/*
 * uv_esp32.c (TARGET_LINUX)
 *
 * Wrapper that mirrors the embedded ESP-AT MQTT client API on top of
 * libmosquitto. The host OS owns the WiFi link, so all WiFi-management
 * functions are no-ops; only the MQTT publish/subscribe paths are wired
 * to a real broker.
 *
 * Install dependency: sudo apt-get install libmosquitto-dev
 */

#include "uv_esp32.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if CONFIG_ESP32 && CONFIG_TARGET_LINUX

#include <mosquitto.h>


#define ESP32_DEBUG(esp, ...) do { \
		if ((esp)->wifi_flags != NULL && \
				(*((esp)->wifi_flags) & ESP32_CONF_FLAGS_DEBUG)) { \
			printf(__VA_ARGS__); \
		} \
	} while (0)


// libmosquitto is process-global (mosquitto_lib_init is reference-counted but
// uv_esp32_st has no platform-specific field for the handle). Keep one instance
// per process — matches the singleton lifecycle of the embedded driver.
static struct mosquitto *s_mosq = NULL;
static uv_esp32_st *s_owner = NULL;


static void on_connect(struct mosquitto *m, void *userdata, int rc) {
	(void) m;
	uv_esp32_st *this = (uv_esp32_st *) userdata;
	if (rc == 0) {
		this->mqtt_state = ESP32_MQTT_STATE_CONNECTED;
		this->state = ESP32_STATE_JOINED_NETWORK;
		ESP32_DEBUG(this, "ESP32(linux): MQTT connected\n");
	}
	else {
		this->mqtt_state = ESP32_MQTT_STATE_ERROR;
		ESP32_DEBUG(this, "ESP32(linux): MQTT connect rc=%d (%s)\n",
				rc, mosquitto_connack_string(rc));
	}
}


static void on_disconnect(struct mosquitto *m, void *userdata, int rc) {
	(void) m;
	uv_esp32_st *this = (uv_esp32_st *) userdata;
	this->mqtt_state = (rc == 0) ?
			ESP32_MQTT_STATE_DISABLED : ESP32_MQTT_STATE_ERROR;
	ESP32_DEBUG(this, "ESP32(linux): MQTT disconnected rc=%d\n", rc);
}


static void on_message(struct mosquitto *m, void *userdata,
		const struct mosquitto_message *msg) {
	(void) m;
	uv_esp32_st *this = (uv_esp32_st *) userdata;
	if (this->mqtt_rx_callb != NULL && msg != NULL) {
		this->mqtt_rx_callb(msg->topic,
				(const uint8_t *) msg->payload,
				(uint16_t) msg->payloadlen);
	}
	else {
		/* no subscriber registered — drop the message */
	}
}


static void apply_tls(uv_esp32_st *this) {
	// `scheme` follows the ESP-AT AT+MQTTUSERCFG spec: 1 = TCP, 2..5 = TLS
	// variants. Schemes 2 and 4 skip server-cert verification; schemes 3
	// and 5 verify the server cert against the provisioned CA.
	if (this->mqtt_scheme >= 2) {
		mosquitto_tls_insecure_set(s_mosq,
				this->mqtt_scheme == 2 ||
				this->mqtt_scheme == 4);
	}
	else {
		/* plain MQTT (scheme 1); no TLS setup */
	}
}


uv_errors_e uv_esp32_init(uv_esp32_st *this,
		uv_gpios_e reset_io,
		uv_uarts_e uart,
		uint16_t *wifi_flags,
		char *wifi_ssid,
		char *wifi_passwd) {
	(void) reset_io;
	(void) uart;
	uv_errors_e ret = ERR_NONE;
	memset(this, 0, sizeof(*this));
	this->wifi_flags = wifi_flags;
	this->wifi_ssid = wifi_ssid;
	this->wifi_passwd = wifi_passwd;
	this->state = ESP32_STATE_INIT;
	this->mqtt_state = ESP32_MQTT_STATE_DISABLED;
	this->mqtt_rx_callb = NULL;

	if (s_mosq == NULL) {
		mosquitto_lib_init();
		s_mosq = mosquitto_new(NULL, true, this);
		if (s_mosq == NULL) {
			ret = ERR_NOT_RESPONDING;
		}
		else {
			s_owner = this;
			mosquitto_connect_callback_set(s_mosq, on_connect);
			mosquitto_disconnect_callback_set(s_mosq, on_disconnect);
			mosquitto_message_callback_set(s_mosq, on_message);
		}
	}
	else {
		/* lib already initialized for this process */
	}

	return ret;
}


void uv_esp32_step(uv_esp32_st *this, uint16_t step_ms) {
	(void) step_ms;
	if (s_mosq == NULL) {
		return;
	}

	// Lazy-connect once an MQTT broker URL has been configured.
	if (this->mqtt_state == ESP32_MQTT_STATE_DISABLED &&
			this->mqtt_broker_url != NULL &&
			this->mqtt_broker_url[0] != '\0') {
		this->mqtt_state = ESP32_MQTT_STATE_INIT;

		if (this->mqtt_user != NULL && this->mqtt_user[0] != '\0') {
			mosquitto_username_pw_set(s_mosq,
					this->mqtt_user,
					this->mqtt_passwd);
		}
		else {
			/* anonymous broker */
		}
		apply_tls(this);

		int rc = mosquitto_connect_async(s_mosq,
				this->mqtt_broker_url,
				(int) this->mqtt_broker_port,
				this->mqtt_keepalive_s ?
						this->mqtt_keepalive_s :
						ESP32_MQTT_DEFAULT_KEEPALIVE_S);
		if (rc == MOSQ_ERR_SUCCESS) {
			this->mqtt_state = ESP32_MQTT_STATE_CONN;
		}
		else {
			this->mqtt_state = ESP32_MQTT_STATE_ERROR;
			ESP32_DEBUG(this, "ESP32(linux): mosquitto_connect_async rc=%d\n",
					rc);
		}
	}
	else {
		/* already connecting / connected */
	}

	// Pump network I/O. mosquitto_loop is a non-blocking single iteration
	// when timeout=0; it dispatches reads, writes, keepalive, and triggers
	// callbacks (on_connect / on_message / on_disconnect).
	int rc = mosquitto_loop(s_mosq, 0, 1);
	if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_NO_CONN) {
		ESP32_DEBUG(this, "ESP32(linux): mosquitto_loop rc=%d (%s)\n",
				rc, mosquitto_strerror(rc));
		this->mqtt_state = ESP32_MQTT_STATE_ERROR;
	}
	else {
		/* loop OK or not yet connected */
	}
}


uv_errors_e uv_esp32_mqtt_publish(uv_esp32_st *this,
		const char *topic, const uint8_t *data, uint16_t datalen,
		uint8_t qos, bool retain,
		uv_esp32_mqtt_prio_e priority,
		uint16_t stream_id) {
	(void) priority;
	(void) stream_id;
	// libmosquitto has its own outbound queue and mosquitto_publish never
	// blocks on the broker; the priority/coalescing pool is only meaningful
	// on the embedded side where AT bandwidth is the bottleneck. On the sim
	// we just hand off straight to mosquitto.
	uv_errors_e ret = ERR_NONE;
	if (this->mqtt_state != ESP32_MQTT_STATE_CONNECTED) {
		ret = ERR_NOT_READY;
	}
	else if (datalen > ESP32_MQTT_PAYLOAD_MAX_LEN) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else if (strlen(topic) >= ESP32_MQTT_TOPIC_MAX_LEN) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {
		int rc = mosquitto_publish(s_mosq, NULL, topic,
				(int) datalen, data, (int) qos, retain);
		if (rc != MOSQ_ERR_SUCCESS) {
			ESP32_DEBUG(this, "ESP32(linux): publish rc=%d (%s)\n",
					rc, mosquitto_strerror(rc));
			ret = ERR_NOT_RESPONDING;
		}
		else {
			/* queued for transmission */
		}
	}
	return ret;
}


uv_errors_e uv_esp32_mqtt_subscribe(uv_esp32_st *this,
		const char *topic, uint8_t qos) {
	uv_errors_e ret = ERR_NONE;
	if (this->mqtt_state != ESP32_MQTT_STATE_CONNECTED) {
		ret = ERR_NOT_READY;
	}
	else {
		int rc = mosquitto_subscribe(s_mosq, NULL, topic, (int) qos);
		if (rc != MOSQ_ERR_SUCCESS) {
			ESP32_DEBUG(this, "ESP32(linux): subscribe rc=%d (%s)\n",
					rc, mosquitto_strerror(rc));
			ret = ERR_NOT_RESPONDING;
		}
		else {
			/* subscription queued */
		}
	}
	return ret;
}


uv_errors_e uv_esp32_mqtt_unsubscribe(uv_esp32_st *this, const char *topic) {
	uv_errors_e ret = ERR_NONE;
	if (this->mqtt_state != ESP32_MQTT_STATE_CONNECTED) {
		ret = ERR_NOT_READY;
	}
	else {
		int rc = mosquitto_unsubscribe(s_mosq, NULL, topic);
		if (rc != MOSQ_ERR_SUCCESS) {
			ESP32_DEBUG(this, "ESP32(linux): unsubscribe rc=%d (%s)\n",
					rc, mosquitto_strerror(rc));
			ret = ERR_NOT_RESPONDING;
		}
		else {
			/* unsubscription queued */
		}
	}
	return ret;
}


void uv_esp32_mqtt_set_rx_callb(uv_esp32_st *this, uv_esp32_mqtt_rx_callb_t cb) {
	this->mqtt_rx_callb = cb;
}


/* --- WiFi side: host OS owns the link --- */

void uv_esp32_reset(uv_esp32_st *this) {
	if (s_mosq != NULL) {
		mosquitto_disconnect(s_mosq);
	}
	else {
		/* nothing to disconnect */
	}
	this->mqtt_state = ESP32_MQTT_STATE_DISABLED;
	this->state = ESP32_STATE_INIT;
}


void uv_esp32_network_leave(uv_esp32_st *this) {
	if (this->wifi_ssid != NULL) {
		this->wifi_ssid[0] = '\0';
	}
	if (this->wifi_passwd != NULL) {
		this->wifi_passwd[0] = '\0';
	}
	this->state = ESP32_STATE_LEFT_NETWORK;
}


void uv_esp32_network_join(uv_esp32_st *this, char ssid[32], char passwd[64]) {
	if (this->wifi_ssid != NULL) {
		strncpy(this->wifi_ssid, ssid, SSID_STR_MAX_LEN - 1);
		this->wifi_ssid[SSID_STR_MAX_LEN - 1] = '\0';
	}
	if (this->wifi_passwd != NULL) {
		strncpy(this->wifi_passwd, passwd, PASSWD_STR_MAX_LEN - 1);
		this->wifi_passwd[PASSWD_STR_MAX_LEN - 1] = '\0';
	}
	this->state = ESP32_STATE_JOINED_NETWORK;
}


uv_errors_e uv_esp32_network_scan(uv_esp32_st *this, bool blocking) {
	(void) this;
	(void) blocking;
	// Scanning is the host OS's job; the simulator wifi tab does not call
	// this path on TARGET_LINUX.
	return ERR_NOT_IMPLEMENTED;
}


uv_errors_e uv_esp32_get_data(uv_esp32_st *this, char *dest) {
	(void) this;
	(void) dest;
	return ERR_NOT_IMPLEMENTED;
}


uv_errors_e uv_esp32_write(uv_esp32_st *this,
		char *data, uint16_t datalen, int32_t wait_ms,
		uint32_t *transmitting_index) {
	(void) this;
	(void) data;
	(void) datalen;
	(void) wait_ms;
	(void) transmitting_index;
	return ERR_NOT_IMPLEMENTED;
}


uv_errors_e uv_esp32_write_isr(uv_esp32_st *this,
		char *data, uint16_t datalen, uint32_t *transmitting_index) {
	(void) this;
	(void) data;
	(void) datalen;
	(void) transmitting_index;
	return ERR_NOT_IMPLEMENTED;
}


void uv_esp32_mac_get_str(uv_esp32_st *this, char *dest) {
	(void) this;
	const char *user = getenv("USER");
	if (user == NULL || user[0] == '\0') {
		user = "unknown";
	}
	snprintf(dest, ESP32_MAC_STR_LEN, "%s:%d", user, (int) getpid());
	dest[ESP32_MAC_STR_LEN - 1] = '\0';
}


char *uv_esp32_get_connected_ssid(uv_esp32_st *this) {
	return (this->wifi_ssid != NULL) ? this->wifi_ssid : "";
}


void uv_esp32_terminal(uv_esp32_st *this,
		unsigned int args, argument_st *argv) {
	(void) this;
	(void) args;
	(void) argv;
}


/* --- shared helpers (state strings + conf reset) --- */

const char *uv_esp32_state_to_str(uv_esp32_states_e state) {
	const char *str = "UNKNOWN";
	switch (state) {
	case ESP32_STATE_INIT:				str = "INIT"; break;
	case ESP32_STATE_WAIT_READY:		str = "WAIT_READY"; break;
	case ESP32_STATE_TEST_AT:			str = "TEST_AT"; break;
	case ESP32_STATE_DISABLE_ECHO:		str = "DISABLE_ECHO"; break;
	case ESP32_STATE_SET_CWMODE:		str = "SET_CWMODE"; break;
	case ESP32_STATE_CONNECT_WIFI:		str = "CONNECT_WIFI"; break;
	case ESP32_STATE_JOINED_NETWORK:	str = "JOINED"; break;
	case ESP32_STATE_LEFT_NETWORK:		str = "LEFT"; break;
	case ESP32_STATE_SCAN_NETWORKS:		str = "SCAN"; break;
	case ESP32_STATE_GET_MAC:			str = "GET_MAC"; break;
	default:							break;
	}
	return str;
}


const char *uv_esp32_mqtt_state_to_str(uv_esp32_mqtt_states_e state) {
	const char *str = "UNKNOWN";
	switch (state) {
	case ESP32_MQTT_STATE_DISABLED:		str = "DISABLED"; break;
	case ESP32_MQTT_STATE_INIT:			str = "INIT"; break;
	case ESP32_MQTT_STATE_USERCFG:		str = "USERCFG"; break;
	case ESP32_MQTT_STATE_CONNCFG:		str = "CONNCFG"; break;
	case ESP32_MQTT_STATE_CONN:			str = "CONN"; break;
	case ESP32_MQTT_STATE_CONNECTED:	str = "CONNECTED"; break;
	case ESP32_MQTT_STATE_ERROR:		str = "ERROR"; break;
	default:							break;
	}
	return str;
}


void uv_esp32_mqtt_init(uv_esp32_st *this,
		const char *broker_url,
		uint16_t broker_port,
		const char *client_id,
		const char *user,
		const char *passwd,
		uint16_t scheme,
		uint16_t ca_id,
		uint16_t cert_key_id,
		uint16_t keepalive_s) {
	this->mqtt_broker_url = broker_url;
	this->mqtt_broker_port = broker_port;
	this->mqtt_client_id = client_id;
	this->mqtt_user = user;
	this->mqtt_passwd = passwd;
	this->mqtt_scheme = scheme;
	this->mqtt_ca_id = ca_id;
	this->mqtt_cert_key_id = cert_key_id;
	this->mqtt_keepalive_s = keepalive_s;
}


#endif
