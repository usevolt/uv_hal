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


// Signal strength reported while "joined" on the simulator. The host OS owns
// the WiFi link, so there is no AP to measure; this is a plausible good-but-not
// -perfect value that keeps the strength symbol meaningful in the sim.
#define ESP32_SIM_RSSI		(-58)


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


// How long to wait for a connection to complete before giving up on it, and how
// long to wait before retrying afterwards. The embedded driver has equivalents
// (ESP32_MQTT_CONN_TIMEOUT_MS and an exponential backoff); without them here a
// single failed connect would wedge the simulator in CONN or ERROR forever, with
// no way to point it at another broker.
#define ESP32_LINUX_CONN_TIMEOUT_MS		5000
#define ESP32_LINUX_RETRY_MS			3000


/// @brief: Enters the error state with a retry armed.
static void mqtt_enter_error(uv_esp32_st *this) {
	this->mqtt_state = ESP32_MQTT_STATE_ERROR;
	uv_delay_init(&this->mqtt_timeout, ESP32_LINUX_RETRY_MS);
}


/// @brief: Marks every registered subscription as not yet issued, so step()
/// replays the registry. A clean-session reconnect starts with none.
static void mqtt_sub_invalidate(uv_esp32_st *this) {
	for (uint8_t i = 0; i < ESP32_MQTT_SUBSCRIPTION_COUNT; i++) {
		uv_esp32_mqtt_sub_st *s = &this->mqtt_subs[i];
		s->sent = false;
		if (s->unsub) {
			s->in_use = false;
			s->unsub = false;
		}
		else {
		}
	}
}


static void on_connect(struct mosquitto *m, void *userdata, int rc) {
	(void) m;
	uv_esp32_st *this = (uv_esp32_st *) userdata;
	if (rc == 0) {
		this->mqtt_state = ESP32_MQTT_STATE_CONNECTED;
		this->state = ESP32_STATE_JOINED_NETWORK;
		// The host OS owns the WiFi link here, so there is no AP to measure.
		// Report a strong-but-not-perfect value so the signal strength symbol
		// has something plausible to show on the simulator.
		this->rssi = ESP32_SIM_RSSI;
		// clean session: nothing is subscribed on the fresh connection
		mqtt_sub_invalidate(this);
		ESP32_DEBUG(this, "ESP32(linux): MQTT connected\n");
	}
	else {
		mqtt_enter_error(this);
		ESP32_DEBUG(this, "ESP32(linux): MQTT connect rc=%d (%s)\n",
				rc, mosquitto_connack_string(rc));
	}
}


static void on_disconnect(struct mosquitto *m, void *userdata, int rc) {
	(void) m;
	uv_esp32_st *this = (uv_esp32_st *) userdata;
	if (rc == 0) {
		this->mqtt_state = ESP32_MQTT_STATE_DISABLED;
	}
	else {
		mqtt_enter_error(this);
	}
	mqtt_sub_invalidate(this);
	ESP32_DEBUG(this, "ESP32(linux): MQTT disconnected rc=%d\n", rc);
}


/// @brief: The publish this stream is still waiting on, by mosquitto message
/// id, or 0 when it has nothing outstanding.
///
/// The embedded side answers "is this stream still busy?" from its own slot
/// pool. Here mosquitto owns the queue, so the same question is answered by
/// remembering the message id it gave us and clearing it when it reports the
/// message sent. Without this the simulator would have no back-pressure at all
/// and a producer holding one stream could run as far ahead as it liked -
/// which is precisely what the pool exists to stop.
#define ESP32_LINUX_STREAM_MAX	8
static struct {
	uint16_t stream_id;
	int mid;
} s_pending[ESP32_LINUX_STREAM_MAX];

/// @brief: The message id the publish callback last reported.
///
/// For QoS 0 mosquitto calls that callback from inside mosquitto_publish, so a
/// message can be finished before the call that sent it has even returned. Its
/// id is kept here so the caller can tell that case apart from a message still
/// on its way, instead of recording a pending message that has already gone and
/// waiting for it for ever.
static int s_last_pub_mid = -1;


static void on_publish(struct mosquitto *m, void *userdata, int mid) {
	(void) m;
	(void) userdata;
	s_last_pub_mid = mid;
	for (uint8_t i = 0; i < ESP32_LINUX_STREAM_MAX; i++) {
		if (s_pending[i].mid == mid) {
			s_pending[i].stream_id = 0;
			s_pending[i].mid = 0;
		}
		else {
		}
	}
}


bool uv_esp32_mqtt_publish_pending(uv_esp32_st *this, uint16_t stream_id) {
	(void) this;
	bool ret = false;
	if (stream_id != 0) {
		for (uint8_t i = 0; (i < ESP32_LINUX_STREAM_MAX) && !ret; i++) {
			if (s_pending[i].stream_id == stream_id) {
				ret = true;
			}
			else {
			}
		}
	}
	else {
	}
	return ret;
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


/// @brief: CA the broker's certificate is verified against. Baked in by the
/// makefile (MQTT_CA) and overridable at run time with the UV_MQTT_CA
/// environment variable, because the compiled-in path is relative to the
/// directory the simulator is started from.
#ifndef UV_MQTT_CA_FILE
#define UV_MQTT_CA_FILE		"certs/mqtt_ca.crt"
#endif


static void apply_tls(uv_esp32_st *this) {
	// `scheme` follows the ESP-AT AT+MQTTUSERCFG spec: 1 = TCP, 2..5 = TLS
	// variants. Schemes 2 and 4 skip server-cert verification; schemes 3
	// and 5 verify the server cert against the provisioned CA.
	if (this->mqtt_scheme >= 2) {
		const char *ca = getenv("UV_MQTT_CA");
		if ((ca == NULL) || (ca[0] == '\0')) {
			ca = UV_MQTT_CA_FILE;
		}
		else {
		}
		bool insecure = ((this->mqtt_scheme == 2) ||
				(this->mqtt_scheme == 4));

		// TLS is switched on by mosquitto_tls_set. mosquitto_tls_insecure_set
		// only selects whether the host name is checked and enables nothing on
		// its own — calling it alone leaves a plain TCP connection, which then
		// fails silently against a TLS-only port such as the broker's 8883.
		int rc;
		if (access(ca, R_OK) == 0) {
			rc = mosquitto_tls_set(s_mosq, ca, NULL, NULL, NULL, NULL);
			ESP32_DEBUG(this, "ESP32(linux): TLS with CA '%s'%s\n",
					ca, insecure ? " (host name not checked)" : "");
		}
		else {
			// The private CA is not where we expected. Fall back to the host
			// trust store so a publicly signed broker still works, and say so —
			// against the Usevolt broker this will fail the handshake.
			rc = mosquitto_tls_set(s_mosq, NULL, "/etc/ssl/certs",
					NULL, NULL, NULL);
			ESP32_DEBUG(this, "ESP32(linux): CA '%s' not readable, "
					"falling back to the system trust store\n", ca);
		}
		if (rc != MOSQ_ERR_SUCCESS) {
			ESP32_DEBUG(this, "ESP32(linux): mosquitto_tls_set rc=%d (%s)\n",
					rc, mosquitto_strerror(rc));
		}
		else {
		}
		mosquitto_tls_insecure_set(s_mosq, insecure);
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

	// The handle is created lazily in uv_esp32_step, not here: libmosquitto
	// fixes the client id at mosquitto_new() time and the id is not known yet
	// (uv_esp32_mqtt_init has not been called). Creating it with a NULL id
	// would give us a random one, and the broker ACL scopes an anonymous device
	// by its client id (`pattern ... %c`), so every publish would be denied.
	mosquitto_lib_init();

	return ret;
}


/// @brief: Creates the process-wide mosquitto handle, bound to the configured
/// client id. Returns false if it could not be created.
static bool mosq_ensure(uv_esp32_st *this) {
	bool ret = true;
	if (s_mosq == NULL) {
		const char *id = ((this->mqtt_client_id != NULL) &&
				(this->mqtt_client_id[0] != '\0')) ?
						this->mqtt_client_id : NULL;
		s_mosq = mosquitto_new(id, true, this);
		if (s_mosq == NULL) {
			ret = false;
		}
		else {
			s_owner = this;
			mosquitto_connect_callback_set(s_mosq, on_connect);
			mosquitto_disconnect_callback_set(s_mosq, on_disconnect);
			mosquitto_message_callback_set(s_mosq, on_message);
			mosquitto_publish_callback_set(s_mosq, on_publish);
			ESP32_DEBUG(this, "ESP32(linux): client id '%s'\n",
					(id != NULL) ? id : "(random)");
		}
	}
	else {
		/* already created */
	}
	return ret;
}


/// @brief: Issues any registered subscriptions that the current connection
/// does not have yet, and any pending unsubscribes.
static void mqtt_sub_drain(uv_esp32_st *this) {
	for (uint8_t i = 0; i < ESP32_MQTT_SUBSCRIPTION_COUNT; i++) {
		uv_esp32_mqtt_sub_st *s = &this->mqtt_subs[i];
		if (!s->in_use) {
			continue;
		}
		else if (s->unsub) {
			(void) mosquitto_unsubscribe(s_mosq, NULL, s->topic);
			s->in_use = false;
			s->unsub = false;
			s->sent = false;
		}
		else if (!s->sent) {
			int rc = mosquitto_subscribe(s_mosq, NULL, s->topic, (int) s->qos);
			if (rc == MOSQ_ERR_SUCCESS) {
				s->sent = true;
				ESP32_DEBUG(this, "ESP32(linux): subscribed '%s'\n", s->topic);
			}
			else {
				ESP32_DEBUG(this, "ESP32(linux): subscribe '%s' rc=%d (%s)\n",
						s->topic, rc, mosquitto_strerror(rc));
			}
		}
		else {
			/* already live on this connection */
		}
	}
}


void uv_esp32_step(uv_esp32_st *this, uint16_t step_ms) {

	// Lazy-connect once an MQTT broker URL has been configured.
	if (this->mqtt_state == ESP32_MQTT_STATE_DISABLED &&
			this->mqtt_broker_url != NULL &&
			this->mqtt_broker_url[0] != '\0' &&
			mosq_ensure(this)) {
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
			// mosquitto_loop reports an unfinished connection as NO_CONN, which
			// is indistinguishable from "still connecting", so bound the wait
			uv_delay_init(&this->mqtt_timeout, ESP32_LINUX_CONN_TIMEOUT_MS);
		}
		else {
			mqtt_enter_error(this);
			ESP32_DEBUG(this, "ESP32(linux): mosquitto_connect_async rc=%d\n",
					rc);
		}
	}
	else {
		/* already connecting / connected */
	}

	if (s_mosq == NULL) {
		/* nothing configured yet — no handle to pump */
	}
	else {
		// Pump network I/O. mosquitto_loop is a non-blocking single iteration
		// when timeout=0; it dispatches reads, writes, keepalive, and triggers
		// callbacks (on_connect / on_message / on_disconnect).
		int rc = mosquitto_loop(s_mosq, 0, 1);
		if (rc != MOSQ_ERR_SUCCESS && rc != MOSQ_ERR_NO_CONN) {
			ESP32_DEBUG(this, "ESP32(linux): mosquitto_loop rc=%d (%s)\n",
					rc, mosquitto_strerror(rc));
			mqtt_enter_error(this);
		}
		else {
			/* loop OK or not yet connected */
		}

		// Give up on a connection that never completes, and retry after a
		// failure, so a changed broker address always gets a fresh attempt.
		if ((this->mqtt_state == ESP32_MQTT_STATE_CONN) &&
				uv_delay(&this->mqtt_timeout, step_ms)) {
			ESP32_DEBUG(this, "ESP32(linux): MQTT connect timeout\n");
			(void) mosquitto_disconnect(s_mosq);
			mqtt_enter_error(this);
		}
		else if ((this->mqtt_state == ESP32_MQTT_STATE_ERROR) &&
				uv_delay(&this->mqtt_timeout, step_ms)) {
			(void) mosquitto_disconnect(s_mosq);
			this->mqtt_state = ESP32_MQTT_STATE_DISABLED;
		}
		else {
		}

		if (this->mqtt_state == ESP32_MQTT_STATE_CONNECTED) {
			mqtt_sub_drain(this);
		}
		else {
		}
	}
}


uv_errors_e uv_esp32_mqtt_publish(uv_esp32_st *this,
		const char *topic, const uint8_t *data, uint16_t datalen,
		uint8_t qos, bool retain,
		uv_esp32_mqtt_prio_e priority,
		uint16_t stream_id) {
	(void) priority;
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
		int mid = 0;
		s_last_pub_mid = -1;
		int rc = mosquitto_publish(s_mosq, &mid, topic,
				(int) datalen, data, (int) qos, retain);
		// Only remember it if it has not already gone: at QoS 0 the callback
		// runs inside the call above, and recording it afterwards would mark a
		// finished message as pending with nothing left to clear it.
		if ((rc == MOSQ_ERR_SUCCESS) && (stream_id != 0) &&
				(s_last_pub_mid != mid)) {
			// remember it so this stream can be asked whether it is still busy
			for (uint8_t i = 0; i < ESP32_LINUX_STREAM_MAX; i++) {
				if ((s_pending[i].stream_id == 0) ||
						(s_pending[i].stream_id == stream_id)) {
					s_pending[i].stream_id = stream_id;
					s_pending[i].mid = mid;
					break;
				}
				else {
				}
			}
		}
		else {
		}
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


/// @brief: Finds the registry entry for *topic*, or NULL.
static uv_esp32_mqtt_sub_st *mqtt_sub_find(uv_esp32_st *this,
		const char *topic) {
	uv_esp32_mqtt_sub_st *ret = NULL;
	for (uint8_t i = 0; i < ESP32_MQTT_SUBSCRIPTION_COUNT; i++) {
		uv_esp32_mqtt_sub_st *s = &this->mqtt_subs[i];
		if (s->in_use &&
				(strcmp(s->topic, topic) == 0)) {
			ret = s;
			break;
		}
		else {
		}
	}
	return ret;
}


// Same contract as the embedded driver: the registry is the source of truth
// and is replayed on every (re)connect, so callers subscribe once and never
// have to watch for disconnects.
uv_errors_e uv_esp32_mqtt_subscribe(uv_esp32_st *this,
		const char *topic, uint8_t qos) {
	uv_errors_e ret = ERR_NONE;
	if (strlen(topic) >= ESP32_MQTT_TOPIC_MAX_LEN) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {
		uv_esp32_mqtt_sub_st *target = mqtt_sub_find(this, topic);
		if (target == NULL) {
			for (uint8_t i = 0; i < ESP32_MQTT_SUBSCRIPTION_COUNT; i++) {
				if (!this->mqtt_subs[i].in_use) {
					target = &this->mqtt_subs[i];
					break;
				}
				else {
				}
			}
		}
		else {
		}
		if (target == NULL) {
			ret = ERR_BUFFER_OVERFLOW;
		}
		else {
			strncpy(target->topic, topic, ESP32_MQTT_TOPIC_MAX_LEN - 1);
			target->topic[ESP32_MQTT_TOPIC_MAX_LEN - 1] = '\0';
			target->qos = qos;
			target->unsub = false;
			target->sent = false;
			target->in_use = true;
		}
	}
	return ret;
}


uv_errors_e uv_esp32_mqtt_unsubscribe(uv_esp32_st *this, const char *topic) {
	uv_esp32_mqtt_sub_st *target = mqtt_sub_find(this, topic);
	if (target != NULL) {
		if (target->sent) {
			target->unsub = true;
		}
		else {
			/* never issued — nothing to tell the broker about */
			target->in_use = false;
		}
	}
	else {
	}
	return ERR_NONE;
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
	this->rssi = 0;
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
	this->rssi = ESP32_SIM_RSSI;
}


uv_errors_e uv_esp32_network_scan(uv_esp32_st *this, bool blocking) {
	(void) blocking;
	// The host OS owns the radio, so there is nothing real to scan for. Return
	// a small synthetic list rather than nothing, so the scan command and any
	// network picker can be exercised on the simulator. The names are
	// deliberately obvious, so nobody mistakes these for real access points.
	static const struct {
		const char *ssid;
		int8_t rssi;
	} sim_nets[] = {
			{ "sim-ap-strong", -45 },
			{ "sim-ap-medium", -68 },
			{ "sim-ap-weak",   -84 },
	};
	uint8_t count = (uint8_t) (sizeof(sim_nets) / sizeof(sim_nets[0]));
	if (count > ESP32_SCAN_MAX_NETWORKS) {
		count = ESP32_SCAN_MAX_NETWORKS;
	}
	else {
	}
	for (uint8_t i = 0; i < count; i++) {
		strncpy(this->state_data.scan.networks[i].ssid, sim_nets[i].ssid,
				SSID_STR_MAX_LEN - 1);
		this->state_data.scan.networks[i].ssid[SSID_STR_MAX_LEN - 1] = '\0';
		this->state_data.scan.networks[i].rssi = sim_nets[i].rssi;
	}
	this->state_data.scan.network_count = count;
	return ERR_NONE;
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


// There is no MAC on the host, but callers use this string as the device's
// identity — on MQTT it becomes the client id, which the broker treats as
// unique: a second client connecting with the same id disconnects the first.
// Several simulators are routinely run on one machine, so the pid comes FIRST
// and the user name only fills whatever space is left. The other way round a
// user name of 17+ characters would push the pid out of the buffer entirely and
// every simulator that user starts would share one identity.
void uv_esp32_mac_get_str(uv_esp32_st *this, char *dest) {
	(void) this;
	const char *user = getenv("USER");
	if (user == NULL || user[0] == '\0') {
		user = "unknown";
	}
	else {
	}
	snprintf(dest, ESP32_MAC_STR_LEN, "sim%d:%s", (int) getpid(), user);
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
