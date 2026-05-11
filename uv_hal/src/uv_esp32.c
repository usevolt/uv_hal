/*
 * uv_esp32.c
 *
 *  Created on: Jan 21, 2026
 *      Author: usevolt
 */

#include "uv_esp32.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if CONFIG_ESP32 && !CONFIG_TARGET_LINUX

#define ESP32_DEBUG(esp, ...) do { if ((esp)->conf->flags & ESP32_CONF_FLAGS_DEBUG) { \
	printf_flags(PRINTF_FLAGS_NOTXCALLB, __VA_ARGS__);}} while (0)

#define ESP32_AT_TIMEOUT_MS			5000
#define ESP32_WIFI_TIMEOUT_MS		15000
#define ESP32_RECONNECT_MS			10000
#define ESP32_SCAN_TIMEOUT_MS		15000
#define ESP32_MQTT_AT_TIMEOUT_MS	5000
#define ESP32_MQTT_CONN_TIMEOUT_MS	15000
#define ESP32_MQTT_BACKOFF_MIN_S	2
#define ESP32_MQTT_BACKOFF_MAX_S	60

// Pending-line buffer state, used to coordinate async-event dispatch and
// AT-response matching without double-consuming lines from rx_datastream.
// We accumulate characters into at_resp via at_resp_accumulate; when a line
// completes, dispatch_mqtt_line checks for +MQTT* events. If the line is not
// an async event, it stays "pending" so the next at_get_line call returns it
// to whichever state machine path consumes it.
//
// We piggy-back on the existing at_resp / at_resp_i fields; the only added
// state is a single "pending" flag plus a copy of the line for safe-keeping
// across multiple at_get_line invocations.

#define MQTT_PUB_PHASE_IDLE			0
#define MQTT_PUB_PHASE_AT_SENT		1
#define MQTT_PUB_PHASE_DATA_SENT	2


static void tx(uv_esp32_st *this) {
	uv_mutex_lock(&this->tx_mutex);

	int32_t tx_count = uv_streambuffer_get_len(&this->tx_streambuffer);
	if (tx_count > 0) {
		char c;
		int32_t i;
		for (i = 0; i < tx_count; i++) {
			uv_streambuffer_pop(&this->tx_streambuffer, &c, 1, 0);
			if (!uv_uart_send(this->uart, &c, 1)) {
				break;
			}
		}
		this->transmitted_byte_count += i;
	}

	uv_mutex_unlock(&this->tx_mutex);
}


static void rx(uv_esp32_st *this, int32_t wait_ms) {
	char c;
	while (uv_streambuffer_get_free_space(&this->rx_datastream) > 0 &&
			uv_uart_get(this->uart, &c, 1, wait_ms)) {
		if (this->conf->flags & ESP32_CONF_FLAGS_ECHO) {
			printf("%c", c);
		}
		uv_streambuffer_push(&this->rx_datastream, &c, 1, 0);
		uv_rtos_task_yield();
	}
}


static void at_resp_reset(uv_esp32_st *this) {
	this->at_resp_i = 0;
	this->at_resp[0] = '\0';
	this->at_resp_escape = false;
}


static void esp32_reset(uv_esp32_st *this) {
	uv_streambuffer_clear(&this->tx_streambuffer);
	uv_streambuffer_clear(&this->rx_datastream);
	at_resp_reset(this);
	this->rx_at_cmd = NULL;
	uv_mutex_unlock(&this->txstream_mutex);
	uv_mutex_unlock(&this->tx_mutex);

	uv_gpio_set(this->reset_io, false);
	uv_rtos_task_delay(20);
	uv_gpio_set(this->reset_io, true);
	uv_rtos_task_delay(1000);
}


/// @brief: Pushes a string into the TX streambuffer, escaping ESP-AT
/// special characters (\, ", ,) with a backslash prefix.
static void tx_push_escaped(uv_esp32_st *this, const char *str) {
	for (uint16_t i = 0; str[i] != '\0'; i++) {
		char c = str[i];
		if (c == '"' || c == ',' || c == '\\') {
			char bs = '\\';
			uv_streambuffer_push(&this->tx_streambuffer, &bs, 1, 100);
		}
		else {
		}
		uv_streambuffer_push(&this->tx_streambuffer, &c, 1, 100);
	}
}


/// @brief: Sends an AT command followed by \r\n via tx_streambuffer.
/// If arg1 is not NULL, appends ="arg1" with arg1 escaped.
/// If arg2 is not NULL, appends ,"arg2" with arg2 escaped.
static void send_at_cmd(uv_esp32_st *this, const char *cmd,
		const char *arg1, const char *arg2) {
	uv_mutex_lock(&this->txstream_mutex);
	uv_streambuffer_push(&this->tx_streambuffer,
			(char *) cmd, strlen(cmd), 100);
	if (arg1 != NULL) {
		uv_streambuffer_push(&this->tx_streambuffer, "=\"", 2, 100);
		tx_push_escaped(this, arg1);
		uv_streambuffer_push(&this->tx_streambuffer, "\"", 1, 100);
	}
	else {
	}
	if (arg2 != NULL) {
		uv_streambuffer_push(&this->tx_streambuffer, ",\"", 2, 100);
		tx_push_escaped(this, arg2);
		uv_streambuffer_push(&this->tx_streambuffer, "\"", 1, 100);
	}
	else {
	}
	uv_streambuffer_push(&this->tx_streambuffer,
			"\r\n", 2, 100);
	uv_mutex_unlock(&this->txstream_mutex);
	at_resp_reset(this);
}


/// @brief: Sends a fully-formed AT command line followed by \r\n via
/// tx_streambuffer. The caller is responsible for any escaping inside the
/// composed line. Used for multi-arg AT commands that don't fit the
/// "AT+CMD=\"arg1\",\"arg2\"" shape (e.g. AT+MQTTUSERCFG, AT+MQTTCONN).
static void send_at_cmd_raw(uv_esp32_st *this, const char *line) {
	uv_mutex_lock(&this->txstream_mutex);
	uv_streambuffer_push(&this->tx_streambuffer,
			(char *) line, strlen(line), 100);
	uv_streambuffer_push(&this->tx_streambuffer,
			"\r\n", 2, 100);
	uv_mutex_unlock(&this->txstream_mutex);
	at_resp_reset(this);
}


/// @brief: Accumulates a character into the AT response buffer, handling
/// ESP-AT escape sequences for special characters (\", \, \\).
/// Returns true if a complete line was received (c == '\n').
static bool at_resp_accumulate(uv_esp32_st *this, char c) {
	bool line_complete = false;
	if (c == '\n') {
		this->at_resp[this->at_resp_i] = '\0';
		this->at_resp_escape = false;
		line_complete = true;
	}
	else if (c == '\r') {
		/* ignore carriage return */
	}
	else if (this->at_resp_escape) {
		this->at_resp_escape = false;
		if (c == '"' || c == ',' || c == '\\') {
			/* Recognized escape sequence, store only the character */
			if (this->at_resp_i < ESP32_AT_RESP_LEN - 1) {
				this->at_resp[this->at_resp_i++] = c;
			}
			else {
			}
		}
		else {
			/* Not a recognized escape, store backslash and character */
			if (this->at_resp_i < ESP32_AT_RESP_LEN - 2) {
				this->at_resp[this->at_resp_i++] = '\\';
				this->at_resp[this->at_resp_i++] = c;
			}
			else {
			}
		}
	}
	else if (c == '\\') {
		this->at_resp_escape = true;
	}
	else {
		if (this->at_resp_i < ESP32_AT_RESP_LEN - 1) {
			this->at_resp[this->at_resp_i++] = c;
		}
		else {
		}
	}
	return line_complete;
}


// Forward decl — defined below.
static void mqtt_set_state(uv_esp32_st *this, uv_esp32_mqtt_states_e state);


/// @brief: Recognizes async +MQTT* event lines in this->at_resp and reacts.
/// Returns true if the line was an async event (so callers can drop it),
/// false otherwise (so the line remains for AT-cmd response matching).
static bool dispatch_mqtt_line(uv_esp32_st *this) {
	bool dispatched = false;
	if (strstr(this->at_resp, "+MQTTCONNECTED") != NULL) {
		ESP32_DEBUG(this, "ESP32: +MQTTCONNECTED\n");
		if (this->mqtt_state == ESP32_MQTT_STATE_CONN) {
			mqtt_set_state(this, ESP32_MQTT_STATE_CONNECTED);
		}
		else {
		}
		dispatched = true;
	}
	else if (strstr(this->at_resp, "+MQTTDISCONNECTED") != NULL) {
		ESP32_DEBUG(this, "ESP32: +MQTTDISCONNECTED\n");
		if (this->mqtt_state != ESP32_MQTT_STATE_DISABLED &&
				this->mqtt_state != ESP32_MQTT_STATE_INIT) {
			mqtt_set_state(this, ESP32_MQTT_STATE_ERROR);
		}
		else {
		}
		dispatched = true;
	}
	else if (strstr(this->at_resp, "+MQTTSUBRECV") != NULL) {
		// Parse: +MQTTSUBRECV:0,"topic",<len>,<data>
		// topic and data are delivered with ESP-AT escapes already decoded
		// by at_resp_accumulate. We assume topic does not contain ',' or '"'.
		char *p = strchr(this->at_resp, ',');
		if (p != NULL) {
			char *topic_start = strchr(p, '"');
			if (topic_start != NULL) {
				topic_start++;
				char *topic_end = strchr(topic_start, '"');
				if (topic_end != NULL) {
					uint16_t topic_len = (uint16_t) (topic_end - topic_start);
					if (topic_len >= ESP32_MQTT_TOPIC_MAX_LEN) {
						topic_len = ESP32_MQTT_TOPIC_MAX_LEN - 1;
					}
					else {
					}
					memcpy(this->mqtt_subrecv.topic, topic_start, topic_len);
					this->mqtt_subrecv.topic[topic_len] = '\0';
					char *len_start = topic_end + 1;
					if (*len_start == ',') {
						len_start++;
						uint16_t datalen = (uint16_t) strtoul(
								len_start, &p, 10);
						if (datalen > ESP32_MQTT_PAYLOAD_MAX_LEN) {
							datalen = ESP32_MQTT_PAYLOAD_MAX_LEN;
						}
						else {
						}
						if (p != NULL && *p == ',') {
							p++;
							// Remaining chars in at_resp ARE the payload.
							uint16_t got = (uint16_t) strlen(p);
							if (got > datalen) {
								got = datalen;
							}
							else {
							}
							memcpy(this->mqtt_subrecv.data, p, got);
							this->mqtt_subrecv.expected_len = datalen;
							this->mqtt_subrecv.received_len = got;
							if (got == datalen &&
									this->mqtt_rx_callb != NULL) {
								this->mqtt_rx_callb(
										this->mqtt_subrecv.topic,
										this->mqtt_subrecv.data,
										got);
							}
							else {
								// payload spans more than the line buffer; drop
								ESP32_DEBUG(this,
										"ESP32: SUBRECV payload truncated\n");
							}
						}
						else {
						}
					}
					else {
					}
				}
				else {
				}
			}
			else {
			}
		}
		else {
		}
		dispatched = true;
	}
	else {
	}
	return dispatched;
}


/// @brief: Returns true with a complete line in this->at_resp.
/// If a line is already pending from a prior pump call, returns it.
/// Otherwise pops bytes from rx_datastream until a line completes (or buffer
/// drains).
static bool at_get_line(uv_esp32_st *this) {
	bool ret = false;
	if (this->at_resp_has_pending) {
		strncpy(this->at_resp,
				this->at_resp_pending,
				ESP32_AT_RESP_LEN);
		this->at_resp[ESP32_AT_RESP_LEN - 1] = '\0';
		ret = true;
	}
	else {
		char c;
		while (uv_streambuffer_pop(&this->rx_datastream, &c, 1, 0)) {
			if (at_resp_accumulate(this, c)) {
				this->at_resp_i = 0;
				ret = true;
				break;
			}
			else {
			}
		}
	}
	return ret;
}


/// @brief: Marks the line currently in at_resp as consumed.
static void at_consume_line(uv_esp32_st *this) {
	this->at_resp_has_pending = false;
}


/// @brief: Saves the line currently in at_resp as pending so the next
/// at_get_line call returns it again (used by pump_mqtt_async to defer
/// non-async lines to the AT-response matcher).
static void at_save_pending(uv_esp32_st *this) {
	strncpy(this->at_resp_pending,
			this->at_resp,
			ESP32_AT_RESP_LEN);
	this->at_resp_pending[ESP32_AT_RESP_LEN - 1] = '\0';
	this->at_resp_has_pending = true;
}


/// @brief: Drains rx_datastream of complete lines, dispatching MQTT async
/// events. Non-async lines are saved as pending for the state machine's
/// rx_at_match / scan-loop to consume.
static void pump_mqtt_async(uv_esp32_st *this) {
	while (at_get_line(this)) {
		if (dispatch_mqtt_line(this)) {
			at_consume_line(this);
		}
		else {
			at_save_pending(this);
			break;
		}
	}
}


/// @brief: Pops characters from rx_datastream and accumulates them into
/// at_resp line buffer. When a complete line is received, checks if
/// it contains str1 or str2 (str2 can be NULL).
/// MQTT async events are dispatched per-line and skipped over.
/// Returns 1 if str1 matched, 2 if str2 matched, 0 if neither.
static uint8_t rx_at_match(uv_esp32_st *this,
		const char *str1, const char *str2) {
	uint8_t ret = 0;
	while (at_get_line(this)) {
		if (dispatch_mqtt_line(this)) {
			at_consume_line(this);
			continue;
		}
		else {
		}
		at_consume_line(this);
		if (strstr(this->at_resp, str1) != NULL) {
			this->rx_at_cmd = NULL;
			ret = 1;
			break;
		}
		else if (str2 != NULL &&
				strstr(this->at_resp, str2) != NULL) {
			this->rx_at_cmd = NULL;
			ret = 2;
			break;
		}
		else {
		}
	}
	return ret;
}


const char *uv_esp32_state_to_str(uv_esp32_states_e state) {
	const char *str;
	switch (state) {
	case ESP32_STATE_INIT:
		str = "INIT";
		break;
	case ESP32_STATE_WAIT_READY:
		str = "WAIT_READY";
		break;
	case ESP32_STATE_TEST_AT:
		str = "TEST_AT";
		break;
	case ESP32_STATE_DISABLE_ECHO:
		str = "DISABLE_ECHO";
		break;
	case ESP32_STATE_SET_CWMODE:
		str = "SET_CWMODE";
		break;
	case ESP32_STATE_CONNECT_WIFI:
		str = "CONNECT_WIFI";
		break;
	case ESP32_STATE_JOINED_NETWORK:
		str = "JOINED_NETWORK";
		break;
	case ESP32_STATE_LEFT_NETWORK:
		str = "LEFT_NETWORK";
		break;
	case ESP32_STATE_SCAN_NETWORKS:
		str = "SCAN_NETWORKS";
		break;
	case ESP32_STATE_GET_MAC:
		str = "GET_MAC";
		break;
	default:
		str = "UNKNOWN";
		break;
	}
	return str;
}


static void set_state(uv_esp32_st *this, uv_esp32_states_e state) {
	if (this->state != state) {
		this->state = state;
		switch (state) {
		case ESP32_STATE_WAIT_READY:
		case ESP32_STATE_TEST_AT:
		case ESP32_STATE_DISABLE_ECHO:
		case ESP32_STATE_SET_CWMODE:
			uv_delay_init(&this->timeout, ESP32_AT_TIMEOUT_MS);
			break;
		case ESP32_STATE_CONNECT_WIFI:
			uv_delay_init(&this->timeout, ESP32_WIFI_TIMEOUT_MS);
			break;
		case ESP32_STATE_SCAN_NETWORKS:
			uv_delay_init(&this->timeout, ESP32_SCAN_TIMEOUT_MS);
			break;
		case ESP32_STATE_GET_MAC:
			uv_delay_init(&this->timeout, ESP32_AT_TIMEOUT_MS);
			break;
		case ESP32_STATE_LEFT_NETWORK:
			uv_delay_init(&this->timeout, ESP32_RECONNECT_MS);
			break;
		default:
			break;
		}
	}
}


const char *uv_esp32_mqtt_state_to_str(uv_esp32_mqtt_states_e state) {
	const char *str;
	switch (state) {
	case ESP32_MQTT_STATE_DISABLED:
		str = "DISABLED";
		break;
	case ESP32_MQTT_STATE_INIT:
		str = "INIT";
		break;
	case ESP32_MQTT_STATE_USERCFG:
		str = "USERCFG";
		break;
	case ESP32_MQTT_STATE_CONNCFG:
		str = "CONNCFG";
		break;
	case ESP32_MQTT_STATE_CONN:
		str = "CONN";
		break;
	case ESP32_MQTT_STATE_CONNECTED:
		str = "CONNECTED";
		break;
	case ESP32_MQTT_STATE_ERROR:
		str = "ERROR";
		break;
	default:
		str = "UNKNOWN";
		break;
	}
	return str;
}


static void mqtt_set_state(uv_esp32_st *this, uv_esp32_mqtt_states_e state) {
	if (this->mqtt_state != state) {
		ESP32_DEBUG(this, "ESP32: MQTT %s -> %s\n",
				uv_esp32_mqtt_state_to_str(this->mqtt_state),
				uv_esp32_mqtt_state_to_str(state));
		this->mqtt_state = state;
		switch (state) {
		case ESP32_MQTT_STATE_USERCFG:
		case ESP32_MQTT_STATE_CONNCFG:
			uv_delay_init(&this->mqtt_timeout, ESP32_MQTT_AT_TIMEOUT_MS);
			break;
		case ESP32_MQTT_STATE_CONN:
			uv_delay_init(&this->mqtt_timeout, ESP32_MQTT_CONN_TIMEOUT_MS);
			break;
		case ESP32_MQTT_STATE_ERROR:
			if (this->mqtt_retry_backoff_s == 0) {
				this->mqtt_retry_backoff_s = ESP32_MQTT_BACKOFF_MIN_S;
			}
			else {
				uint16_t doubled = (uint16_t) this->mqtt_retry_backoff_s * 2;
				if (doubled > ESP32_MQTT_BACKOFF_MAX_S) {
					doubled = ESP32_MQTT_BACKOFF_MAX_S;
				}
				else {
				}
				this->mqtt_retry_backoff_s = (uint8_t) doubled;
			}
			uv_delay_init(&this->mqtt_timeout,
					(uint32_t) this->mqtt_retry_backoff_s * 1000U);
			break;
		case ESP32_MQTT_STATE_CONNECTED:
			this->mqtt_retry_backoff_s = 0;
			break;
		default:
			break;
		}
	}
}


static void rxtx_task(void *me_ptr) {
	uv_esp32_st *this = me_ptr;
	uv_ts_st ts;

	uv_ts_init(&ts);
	uv_delay_init(&this->timeout, ESP32_AT_TIMEOUT_MS);

	while (true) {
		uv_ts_step(&ts);
		tx(this);
		rx(this, uv_streambuffer_get_len(&this->rx_datastream) ? 0 : 1);

		// Drain async +MQTT events. Skipped during the publish AT-prompt phase
		// because we need to read the bare ">" byte directly from rx_datastream
		// (it is not a line and would otherwise be swallowed into at_resp).
		bool in_pub_at_sent = (this->mqtt_publish_req.pending &&
				this->mqtt_publish_req.phase == MQTT_PUB_PHASE_AT_SENT);
		if (!in_pub_at_sent) {
			pump_mqtt_async(this);
		}
		else {
		}

		switch (this->state) {

		case ESP32_STATE_INIT:
			ESP32_DEBUG(this, "ESP32: resetting module\n");
			esp32_reset(this);
			set_state(this, ESP32_STATE_WAIT_READY);
			ESP32_DEBUG(this, "ESP32: waiting for ready\n");
			break;

		case ESP32_STATE_WAIT_READY:
			if (rx_at_match(this, "ready", NULL)) {
				ESP32_DEBUG(this, "ESP32: module ready, sending AT\n");
				send_at_cmd(this, "AT", NULL, NULL);
				set_state(this, ESP32_STATE_TEST_AT);
			}
			else if (uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: ready timeout, resetting\n");
				set_state(this, ESP32_STATE_INIT);
			}
			else {
			}
			break;

		case ESP32_STATE_TEST_AT:
			if (rx_at_match(this, "OK", NULL)) {
				ESP32_DEBUG(this, "ESP32: AT OK, disabling echo\n");
				send_at_cmd(this, "ATE0", NULL, NULL);
				set_state(this, ESP32_STATE_DISABLE_ECHO);
			}
			else if (uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: AT timeout, resetting\n");
				set_state(this, ESP32_STATE_INIT);
			}
			else {
			}
			break;

		case ESP32_STATE_DISABLE_ECHO:
			if (rx_at_match(this, "OK", NULL)) {
				ESP32_DEBUG(this, "ESP32: echo disabled, setting CWMODE\n");
				send_at_cmd(this, "AT+CWMODE=1", NULL, NULL);
				set_state(this, ESP32_STATE_SET_CWMODE);
			}
			else if (uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: ATE0 timeout, resetting\n");
				set_state(this, ESP32_STATE_INIT);
			}
			else {
			}
			break;

		case ESP32_STATE_SET_CWMODE:
			if (rx_at_match(this, "OK", NULL)) {
				ESP32_DEBUG(this, "ESP32: CWMODE set, reading MAC\n");
				send_at_cmd(this, "AT+CIPSTAMAC?", NULL, NULL);
				set_state(this, ESP32_STATE_GET_MAC);
			}
			else if (uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: CWMODE timeout, resetting\n");
				set_state(this, ESP32_STATE_INIT);
			}
			else {
			}
			break;

		case ESP32_STATE_CONNECT_WIFI: {
			uint8_t match = rx_at_match(this, "WIFI GOT IP", "ERROR");
			if (match == 1) {
				uv_uart_clear_rx_buffer(this->uart);
				uv_streambuffer_clear(&this->rx_datastream);
				set_state(this, ESP32_STATE_JOINED_NETWORK);
				ESP32_DEBUG(this, "ESP32: WiFi connected\n");
			}
			else if (match == 2 ||
					uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: WiFi connection failed\n");
				set_state(this, ESP32_STATE_LEFT_NETWORK);
			}
			else {
			}
			break;
		}

		case ESP32_STATE_JOINED_NETWORK:
			break;

		case ESP32_STATE_GET_MAC: {
			uint8_t match = rx_at_match(this, "+CIPSTAMAC:", "ERROR");
			if (match == 1) {
				char *start = strchr(this->at_resp, '"');
				if (start != NULL) {
					start++;
					this->mac = 0;
					for (uint8_t i = 0; i < 6; i++) {
						this->mac = (this->mac << 8) |
								strtol(start, &start, 16);
						if (*start == ':') {
							start++;
						}
					}
				}
				ESP32_DEBUG(this, "ESP32: MAC read OK\n");
			}
			else if (match == 2 ||
					uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: get MAC failed\n");
			}
			else {
				break;
			}
			if (strlen(this->conf->ssid) > 0) {
				ESP32_DEBUG(this, "ESP32: joining '%s'\n",
						this->conf->ssid);
				send_at_cmd(this, "AT+CWJAP",
						this->conf->ssid, this->conf->passwd);
				set_state(this, ESP32_STATE_CONNECT_WIFI);
			}
			else {
				ESP32_DEBUG(this, "ESP32: no SSID configured\n");
				set_state(this, ESP32_STATE_LEFT_NETWORK);
			}
			break;
		}

		case ESP32_STATE_SCAN_NETWORKS: {
			while (at_get_line(this)) {
				if (dispatch_mqtt_line(this)) {
					at_consume_line(this);
					continue;
				}
				else {
				}
				at_consume_line(this);
				if (strstr(this->at_resp, "+CWLAP:") != NULL &&
						this->state_data.scan.network_count < ESP32_SCAN_MAX_NETWORKS) {
					char *ssid_start = strchr(this->at_resp, '"');
					if (ssid_start != NULL) {
						ssid_start++;
						char *ssid_end = strchr(ssid_start, '"');
						if (ssid_end != NULL) {
							uint16_t len = ssid_end - ssid_start;
							if (len >= SSID_STR_MAX_LEN) {
								len = SSID_STR_MAX_LEN - 1;
							}
							else {
							}
							memcpy(this->state_data.scan.networks[this->state_data.scan.network_count].ssid,
									ssid_start, len);
							this->state_data.scan.networks[this->state_data.scan.network_count].ssid[len] = '\0';
							char *rssi_str = ssid_end + 2;
							this->state_data.scan.networks[this->state_data.scan.network_count].rssi = strtol(rssi_str, NULL, 10);
							this->state_data.scan.network_count++;
						}
						else {
						}
					}
					else {
					}
				}
				else if (strstr(this->at_resp, "OK") != NULL) {
					ESP32_DEBUG(this, "ESP32: scan complete, %u networks found\n",
							this->state_data.scan.network_count);
					set_state(this, this->scan_return_state);
					break;
				}
				else if (strstr(this->at_resp, "ERROR") != NULL) {
					ESP32_DEBUG(this, "ESP32: scan failed\n");
					set_state(this, this->scan_return_state);
					break;
				}
				else {
				}
			}
			if (this->state == ESP32_STATE_SCAN_NETWORKS &&
					uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: scan timeout\n");
				set_state(this, this->scan_return_state);
			}
			else {
			}
			break;
		}

		case ESP32_STATE_LEFT_NETWORK:
			if (strlen(this->conf->ssid) > 0 &&
					uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: reconnecting to '%s'\n",
						this->conf->ssid);
				send_at_cmd(this, "AT+CWJAP",
						this->conf->ssid, this->conf->passwd);
				set_state(this, ESP32_STATE_CONNECT_WIFI);
			}
			break;

		default:
			set_state(this, ESP32_STATE_INIT);
			break;
		}


		// MQTT state machine. Runs alongside the main esp32 state machine.
		// Progresses only while Wi-Fi is joined; drops to DISABLED otherwise so
		// it cleanly re-enters INIT on rejoin.
		if (this->state == ESP32_STATE_JOINED_NETWORK) {
			switch (this->mqtt_state) {
			case ESP32_MQTT_STATE_DISABLED:
				if (this->conf->mqtt.broker_url[0] != '\0') {
					mqtt_set_state(this, ESP32_MQTT_STATE_INIT);
				}
				else {
				}
				break;
			case ESP32_MQTT_STATE_INIT: {
				char line[256];
				snprintf(line, sizeof(line),
						"AT+MQTTUSERCFG=%d,%u,\"%s\",\"%s\",\"%s\",%u,%u,\"\"",
						ESP32_MQTT_LINK_ID,
						(unsigned int) this->conf->mqtt.scheme,
						this->conf->mqtt.client_id,
						this->conf->mqtt.user,
						this->conf->mqtt.passwd,
						(unsigned int) this->conf->mqtt.cert_key_id,
						(unsigned int) this->conf->mqtt.ca_id);
				ESP32_DEBUG(this, "ESP32: MQTT USERCFG -> '%s'\n",
						this->conf->mqtt.broker_url);
				send_at_cmd_raw(this, line);
				mqtt_set_state(this, ESP32_MQTT_STATE_USERCFG);
				break;
			}
			case ESP32_MQTT_STATE_USERCFG: {
				uint8_t match = rx_at_match(this, "OK", "ERROR");
				if (match == 1) {
					char line[128];
					snprintf(line, sizeof(line),
							"AT+MQTTCONNCFG=%d,%u,0,\"\",\"\",0,0",
							ESP32_MQTT_LINK_ID,
							(unsigned int) this->conf->mqtt.keepalive_s);
					send_at_cmd_raw(this, line);
					mqtt_set_state(this, ESP32_MQTT_STATE_CONNCFG);
				}
				else if (match == 2 ||
						uv_delay(&this->mqtt_timeout,
								uv_ts_get_step_ms(&ts))) {
					ESP32_DEBUG(this, "ESP32: MQTT USERCFG failed\n");
					mqtt_set_state(this, ESP32_MQTT_STATE_ERROR);
				}
				else {
				}
				break;
			}
			case ESP32_MQTT_STATE_CONNCFG: {
				uint8_t match = rx_at_match(this, "OK", "ERROR");
				if (match == 1) {
					char line[128];
					snprintf(line, sizeof(line),
							"AT+MQTTCONN=%d,\"%s\",%u,1",
							ESP32_MQTT_LINK_ID,
							this->conf->mqtt.broker_url,
							(unsigned int) this->conf->mqtt.broker_port);
					send_at_cmd_raw(this, line);
					mqtt_set_state(this, ESP32_MQTT_STATE_CONN);
				}
				else if (match == 2 ||
						uv_delay(&this->mqtt_timeout,
								uv_ts_get_step_ms(&ts))) {
					ESP32_DEBUG(this, "ESP32: MQTT CONNCFG failed\n");
					mqtt_set_state(this, ESP32_MQTT_STATE_ERROR);
				}
				else {
				}
				break;
			}
			case ESP32_MQTT_STATE_CONN: {
				// dispatch_mqtt_line in pump/rx_at_match advances us to
				// CONNECTED on +MQTTCONNECTED. Here we only watch for ERROR
				// or timeout. AT-cmd OK is silently consumed.
				uint8_t match = rx_at_match(this, "ERROR", NULL);
				if (match == 1 ||
						uv_delay(&this->mqtt_timeout,
								uv_ts_get_step_ms(&ts))) {
					ESP32_DEBUG(this, "ESP32: MQTT CONN failed\n");
					mqtt_set_state(this, ESP32_MQTT_STATE_ERROR);
				}
				else {
				}
				break;
			}
			case ESP32_MQTT_STATE_CONNECTED:
				if (this->mqtt_publish_req.pending) {
					if (this->mqtt_publish_req.phase ==
							MQTT_PUB_PHASE_IDLE) {
						char line[200];
						snprintf(line, sizeof(line),
								"AT+MQTTPUBRAW=%d,\"%s\",%u,%u,%u",
								ESP32_MQTT_LINK_ID,
								this->mqtt_publish_req.topic,
								(unsigned int) this->mqtt_publish_req.datalen,
								(unsigned int) this->mqtt_publish_req.qos,
								(unsigned int)
										(this->mqtt_publish_req.retain ? 1 : 0));
						send_at_cmd_raw(this, line);
						uv_delay_init(&this->mqtt_timeout,
								ESP32_MQTT_AT_TIMEOUT_MS);
						this->mqtt_publish_req.phase =
								MQTT_PUB_PHASE_AT_SENT;
					}
					else if (this->mqtt_publish_req.phase ==
							MQTT_PUB_PHASE_AT_SENT) {
						char c;
						bool prompt_seen = false;
						while (uv_streambuffer_pop(
								&this->rx_datastream, &c, 1, 0)) {
							if (c == '>') {
								prompt_seen = true;
								break;
							}
							else {
								// discard pre-prompt chatter
							}
						}
						if (prompt_seen) {
							uv_mutex_lock(&this->txstream_mutex);
							uv_streambuffer_push(&this->tx_streambuffer,
									(char *) this->mqtt_publish_req.data,
									this->mqtt_publish_req.datalen,
									100);
							uv_mutex_unlock(&this->txstream_mutex);
							uv_delay_init(&this->mqtt_timeout,
									ESP32_MQTT_AT_TIMEOUT_MS);
							this->mqtt_publish_req.phase =
									MQTT_PUB_PHASE_DATA_SENT;
						}
						else if (uv_delay(&this->mqtt_timeout,
								uv_ts_get_step_ms(&ts))) {
							ESP32_DEBUG(this,
									"ESP32: MQTT publish prompt timeout\n");
							this->mqtt_publish_req.pending = false;
							this->mqtt_publish_req.phase =
									MQTT_PUB_PHASE_IDLE;
						}
						else {
						}
					}
					else if (this->mqtt_publish_req.phase ==
							MQTT_PUB_PHASE_DATA_SENT) {
						uint8_t match = rx_at_match(this, "OK", "ERROR");
						if (match != 0 ||
								uv_delay(&this->mqtt_timeout,
										uv_ts_get_step_ms(&ts))) {
							if (match == 2) {
								ESP32_DEBUG(this,
										"ESP32: MQTT publish ERROR\n");
							}
							else {
							}
							this->mqtt_publish_req.pending = false;
							this->mqtt_publish_req.phase =
									MQTT_PUB_PHASE_IDLE;
						}
						else {
						}
					}
					else {
					}
				}
				else {
				}
				break;
			case ESP32_MQTT_STATE_ERROR:
				if (uv_delay(&this->mqtt_timeout, uv_ts_get_step_ms(&ts))) {
					mqtt_set_state(this, ESP32_MQTT_STATE_INIT);
				}
				else {
				}
				break;
			default:
				break;
			}
		}
		else {
			if (this->mqtt_state != ESP32_MQTT_STATE_DISABLED) {
				mqtt_set_state(this, ESP32_MQTT_STATE_DISABLED);
				this->mqtt_publish_req.pending = false;
				this->mqtt_publish_req.phase = MQTT_PUB_PHASE_IDLE;
			}
			else {
			}
		}


		// Discard any pending non-async line that no state consumed this
		// iteration (e.g., the OK from a fire-and-forget AT+MQTTSUB). Without
		// this cleanup such lines wedge pump_mqtt_async on subsequent
		// iterations.
		if (this->at_resp_has_pending) {
			this->at_resp_has_pending = false;
		}
		else {
		}

		uv_rtos_task_yield();
	}
}


uv_errors_e uv_esp32_init(uv_esp32_st *this,
		uv_esp32_conf_st *conf,
		uv_gpios_e reset_io,
		uv_uarts_e uart) {
	this->conf = conf;
	this->uart = uart;
	this->reset_io = reset_io;
	this->state = ESP32_STATE_INIT;
	this->mac = 0;
	this->written_byte_count = 0;
	this->transmitted_byte_count = 0;

	this->mqtt_state = ESP32_MQTT_STATE_DISABLED;
	this->mqtt_retry_backoff_s = 0;
	this->mqtt_rx_callb = NULL;
	memset(&this->mqtt_subrecv, 0, sizeof(this->mqtt_subrecv));
	memset(&this->mqtt_publish_req, 0, sizeof(this->mqtt_publish_req));
	this->at_resp_has_pending = false;

	uv_streambuffer_init_static(&this->tx_streambuffer,
								this->tx_buffer,
								sizeof(this->tx_buffer),
								&this->tx_staticstreambuffer);
	uv_mutex_init(&this->txstream_mutex);
	uv_mutex_unlock(&this->txstream_mutex);

	uv_streambuffer_init_static(&this->rx_datastream,
								this->rx_datastream_buffer,
								sizeof(this->rx_datastream_buffer),
								&this->rx_static_datastream);
	at_resp_reset(this);
	this->rx_at_cmd = NULL;

	uv_mutex_init(&this->tx_mutex);
	uv_mutex_unlock(&this->tx_mutex);

	uv_gpio_init_output(this->reset_io, false);

	uv_rtos_task_create(&rxtx_task, "ESP",
						UV_RTOS_MIN_STACK_SIZE * 2,
						this,
						UV_RTOS_IDLE_PRIORITY + 1,
						NULL);

	return ERR_NONE;
}


/// @brief: Step function
void uv_esp32_step(uv_esp32_st *this, uint16_t step_ms) {

}











void uv_esp32_conf_reset(uv_esp32_conf_st *conf) {
	memset(conf, 0, sizeof(*conf));
	uv_esp32_mqtt_conf_reset(&conf->mqtt);
}


void uv_esp32_mqtt_conf_reset(uv_esp32_mqtt_conf_st *conf) {
	memset(conf, 0, sizeof(*conf));
	conf->broker_port = 8883;
	conf->scheme = 4;
	conf->keepalive_s = ESP32_MQTT_DEFAULT_KEEPALIVE_S;
}


uv_errors_e uv_esp32_mqtt_publish(uv_esp32_st *this,
		const char *topic, const uint8_t *data, uint16_t datalen,
		uint8_t qos, bool retain) {
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
		// Wait for any prior publish to complete.
		while (this->mqtt_publish_req.pending) {
			uv_rtos_task_delay(5);
		}
		strncpy(this->mqtt_publish_req.topic, topic,
				ESP32_MQTT_TOPIC_MAX_LEN - 1);
		this->mqtt_publish_req.topic[ESP32_MQTT_TOPIC_MAX_LEN - 1] = '\0';
		memcpy(this->mqtt_publish_req.data, data, datalen);
		this->mqtt_publish_req.datalen = datalen;
		this->mqtt_publish_req.qos = qos;
		this->mqtt_publish_req.retain = retain;
		this->mqtt_publish_req.phase = MQTT_PUB_PHASE_IDLE;
		this->mqtt_publish_req.pending = true;
		// Block until rxtx task completes the publish.
		while (this->mqtt_publish_req.pending) {
			uv_rtos_task_delay(5);
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
		char line[128];
		snprintf(line, sizeof(line),
				"AT+MQTTSUB=%d,\"%s\",%u",
				ESP32_MQTT_LINK_ID, topic, (unsigned int) qos);
		send_at_cmd_raw(this, line);
	}
	return ret;
}


uv_errors_e uv_esp32_mqtt_unsubscribe(uv_esp32_st *this, const char *topic) {
	uv_errors_e ret = ERR_NONE;
	if (this->mqtt_state != ESP32_MQTT_STATE_CONNECTED) {
		ret = ERR_NOT_READY;
	}
	else {
		char line[128];
		snprintf(line, sizeof(line),
				"AT+MQTTUNSUB=%d,\"%s\"",
				ESP32_MQTT_LINK_ID, topic);
		send_at_cmd_raw(this, line);
	}
	return ret;
}


void uv_esp32_mqtt_set_rx_callb(uv_esp32_st *this, uv_esp32_mqtt_rx_callb_t cb) {
	this->mqtt_rx_callb = cb;
}


uv_errors_e uv_esp32_get_data(uv_esp32_st *this, char *dest) {
	return ERR_NONE;
}


uv_errors_e uv_esp32_write(uv_esp32_st *this,
		char *data, uint16_t datalen, int32_t wait_ms,
		uint32_t *transmitting_index) {
	return ERR_HARDWARE_NOT_SUPPORTED;
}


uv_errors_e uv_esp32_write_isr(uv_esp32_st *this,
		char *data, uint16_t datalen, uint32_t *transmitting_index) {
	return ERR_HARDWARE_NOT_SUPPORTED;
}


void uv_esp32_mac_get_str(uv_esp32_st *this, char *dest) {
	snprintf(dest, ESP32_MAC_STR_LEN, "%02x:%02x:%02x:%02x:%02x:%02x",
			(unsigned int) ((this->mac >> 40) & 0xFF),
			(unsigned int) ((this->mac >> 32) & 0xFF),
			(unsigned int) ((this->mac >> 24) & 0xFF),
			(unsigned int) ((this->mac >> 16) & 0xFF),
			(unsigned int) ((this->mac >> 8) & 0xFF),
			(unsigned int) (this->mac & 0xFF));
}


char *uv_esp32_get_connected_ssid(uv_esp32_st *this) {
	return "";
}


uv_errors_e uv_esp32_network_scan(uv_esp32_st *this, bool blocking) {
	uv_errors_e ret = ERR_NOT_READY;
	if (this->state == ESP32_STATE_JOINED_NETWORK ||
			this->state == ESP32_STATE_LEFT_NETWORK) {
		this->state_data.scan.network_count = 0;
		this->scan_return_state = this->state;
		send_at_cmd(this, "AT+CWLAP", NULL, NULL);
		ESP32_DEBUG(this, "ESP32: starting network scan\n");
		set_state(this, ESP32_STATE_SCAN_NETWORKS);
		if (blocking) {
			while (this->state == ESP32_STATE_SCAN_NETWORKS) {
				uv_rtos_task_delay(10);
			}
		}
		ret = ERR_NONE;
	}
	return ret;
}


void uv_esp32_reset(uv_esp32_st *this) {
	this->state = ESP32_STATE_INIT;
}


void uv_esp32_network_leave(uv_esp32_st *this) {
	this->conf->ssid[0] = '\0';
	this->conf->passwd[0] = '\0';
	send_at_cmd(this, "AT+CWQAP", NULL, NULL);
	set_state(this, ESP32_STATE_LEFT_NETWORK);
}


void uv_esp32_network_join(uv_esp32_st *this, char ssid[32],
						   char passwd[64]) {
	strncpy(this->conf->ssid, ssid, SSID_STR_MAX_LEN - 1);
	this->conf->ssid[SSID_STR_MAX_LEN - 1] = '\0';
	strncpy(this->conf->passwd, passwd, PASSWD_STR_MAX_LEN - 1);
	this->conf->passwd[PASSWD_STR_MAX_LEN - 1] = '\0';
	send_at_cmd(this, "AT+CWJAP", this->conf->ssid, this->conf->passwd);
	set_state(this, ESP32_STATE_CONNECT_WIFI);
}


void uv_esp32_terminal(uv_esp32_st *this,
		unsigned int args, argument_st *argv) {

	if (args && argv[0].type == ARG_STRING) {
		if (strcmp(argv[0].str, "debug") == 0) {
			if (args > 1) {
				if (argv[1].number) {
					this->conf->flags |= ESP32_CONF_FLAGS_DEBUG;
				}
				else {
					this->conf->flags &= ~ESP32_CONF_FLAGS_DEBUG;
				}
			}
		}
		else if (strcmp(argv[0].str, "echo") == 0) {
			if (args > 1) {
				if (argv[1].number) {
					this->conf->flags |= ESP32_CONF_FLAGS_ECHO;
				}
				else {
					this->conf->flags &= ~ESP32_CONF_FLAGS_ECHO;
				}
			}
		}
		else if (strcmp(argv[0].str, "at") == 0) {
			if (args > 1 &&
					argv[1].type == ARG_STRING) {
				char cmd[64];
				if (strlen(argv[1].str) > 0) {
					snprintf(cmd, sizeof(cmd), "AT+%s", argv[1].str);
				}
				else {
					snprintf(cmd, sizeof(cmd), "AT");
				}
				const char *arg1 = (args > 2 && argv[2].type == ARG_STRING) ?
						argv[2].str : NULL;
				const char *arg2 = (args > 3 && argv[3].type == ARG_STRING) ?
						argv[3].str : NULL;
				send_at_cmd(this, cmd, arg1, arg2);
			}
			else {
				printf("Give command as second string argument\n");
			}
		}
		else {
			printf("Unknown command '%s'\n",
				   argv[0].str);
		}
	}
	else {
		char mac_str[ESP32_MAC_STR_LEN];
		uv_esp32_mac_get_str(this, mac_str);
		printf("ESP32:\n"
				"    state: %s\n"
				"    mac: %s\n"
				"    debug: %u\n"
				"    echo: %u\n",
					uv_esp32_state_to_str(this->state),
					mac_str,
					!!(this->conf->flags & ESP32_CONF_FLAGS_DEBUG),
					!!(this->conf->flags & ESP32_CONF_FLAGS_ECHO));
	}
}


#endif
