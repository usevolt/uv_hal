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

#define ESP32_DEBUG(esp, ...) do { \
	if ((esp)->wifi_flags != NULL && \
			(*((esp)->wifi_flags) & ESP32_CONF_FLAGS_DEBUG)) { \
		printf_flags(PRINTF_FLAGS_NOTXCALLB, __VA_ARGS__);}} while (0)

#define ESP32_AT_TIMEOUT_MS			5000
#define ESP32_WIFI_TIMEOUT_MS		15000
#define ESP32_RECONNECT_MS			10000
#define ESP32_SCAN_TIMEOUT_MS		15000
#define ESP32_MQTT_AT_TIMEOUT_MS	5000
#define ESP32_MQTT_CONN_TIMEOUT_MS	15000
#define ESP32_MQTT_BACKOFF_MIN_S	2
#define ESP32_MQTT_BACKOFF_MAX_S	60
// How often the signal strength of the joined AP is re-read. Rare on purpose:
// the query shares the AT channel with the MQTT publish drainer, and signal
// strength is a slowly changing, purely informational value.
#define ESP32_RSSI_POLL_MS			10000
// Network time polling. While the clock is unknown the answer is asked for
// often -- a TLS MQTT connect waits for it -- and rarely once it is known,
// only to keep time_str fresh for whatever displays it. The module keeps
// itself in sync on its own after AT+CIPSNTPCFG (lwIP re-syncs hourly), so
// the slow poll is a read, not a resync.
#define ESP32_SNTP_POLL_MS			2000
#define ESP32_SNTP_REFRESH_MS		60000
// How often the SNTP client is restarted while the clock is still unknown. The
// first configuration goes out the moment the network is joined; a restart
// covers that command being lost, answered with ERROR, the module having reset
// quietly -- and, the common case, the first NTP request itself going nowhere.
// That last one is why the first interval is short: a request that is going to
// be answered is answered in well under a second, but lwIP does not retry one
// that was dropped for 15-30 s, and AT+CIPSNTPCFG restarting the client is
// what sends a fresh request instead of waiting out that backoff. The interval
// doubles up to the ceiling so an unreachable server is not hammered.
#define ESP32_SNTP_CFG_RETRY_MIN_MS	5000
#define ESP32_SNTP_CFG_RETRY_MS		30000
// Year from which the module's answer is taken to be a real clock rather than
// the epoch-based one it reports before the first sync.
#define ESP32_SNTP_MIN_YEAR			2024

#define SNTPTIME_PREFIX				"+CIPSNTPTIME:"
#define SNTPTIME_PREFIX_LEN			13
#define SYSTIMESTAMP_PREFIX			"+SYSTIMESTAMP:"
#define SYSTIMESTAMP_PREFIX_LEN		14
// The station address line of AT+CIPSTA?, which also answers with the gateway
// and the netmask; only this one is ours.
#define CIPSTA_IP_PREFIX			"+CIPSTA:ip:"
#define CIPSTA_IP_PREFIX_LEN		11
// Unix time below which the module is considered not to have a clock yet. The
// module answers both time queries from power-on, counting up from the epoch,
// so a plausible date is the only thing separating a set clock from an unset
// one. 2024-01-01, comfortably before any firmware carrying this code ran.
#define ESP32_SNTP_MIN_EPOCH		1704067200u

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

// Subscription drain phases. The registry is written by any task via
// uv_esp32_mqtt_subscribe / _unsubscribe but the AT commands are issued only
// here, in the rxtx task, and only while no publish is mid-transaction — a
// subscribe injected into a publish's data phase would corrupt both.
#define MQTT_SUB_PHASE_IDLE			0
#define MQTT_SUB_PHASE_AT_SENT		1
#define MQTT_SUB_PHASE_BACKOFF		2

#define ESP32_MQTT_SUB_RETRY_MS		5000

// Time given to the answer of a command nobody waits for (a fire-and-forget
// query, a join, a leave) before the next such command or an ESP-NOW exchange
// goes, see uv_esp32_st::at_quiet_delay. They answer in a few milliseconds.
#define ESP32_AT_QUIET_MS			100

// network_join / network_leave requests, see uv_esp32_st::wifi_req
#define ESP32_WIFI_REQ_NONE			0
#define ESP32_WIFI_REQ_JOIN			1
#define ESP32_WIFI_REQ_LEAVE		2

#define SUBRECV_PREFIX				"+MQTTSUBRECV:"
#define SUBRECV_PREFIX_LEN			13

// ESP-NOW exchanges, one at a time, see espnow_step
#define ESPNOW_PHASE_IDLE			0
// a setting (or AT+ENOWDEINIT) sent, waiting for OK / ERROR
#define ESPNOW_PHASE_CMD_SENT		1
// AT+ENOWDISC sent: +ENOWDEV lines, then OK / ERROR
#define ESPNOW_PHASE_DISC_SENT		2
// AT+ENOWSEND sent, waiting for the '>' prompt
#define ESPNOW_PHASE_SEND_AT_SENT	3
// the frame written, waiting for SEND OK / SEND FAIL
#define ESPNOW_PHASE_SEND_DATA_SENT	4

#define ESP32_ESPNOW_AT_TIMEOUT_MS	1000
// The module waits up to 100 ms for the peer's acknowledgement before it
// answers, and the frame has to cross the UART first.
#define ESP32_ESPNOW_SEND_TIMEOUT_MS	1000
#define ESP32_ESPNOW_RETRY_MS		5000
// While frames come from the peer this often, the link is in use: a WiFi
// scan takes the radio off its channel for seconds, so the driver's own scans
// for the configured network are made only every ESP32_ESPNOW_SCAN_SKIPS
// reconnect intervals then, instead of every one.
#define ESP32_ESPNOW_LINK_ALIVE_MS	5000
#define ESP32_ESPNOW_SCAN_SKIPS		6

#define ENOWRECV_PREFIX				"+ENOWRECV:"
#define ENOWRECV_PREFIX_LEN			10
#define ENOWDEV_PREFIX				"+ENOWDEV:"
#define ENOWDEV_PREFIX_LEN			9
#define ENOWINIT_PREFIX				"+ENOWINIT:"
#define ENOWINIT_PREFIX_LEN			10


static void tx(uv_esp32_st *this) {
	uv_mutex_lock(&this->tx_mutex);

	int32_t tx_count = uv_streambuffer_get_len(&this->tx_streambuffer);
	// Only as much as the UART can take: a byte is popped before it is sent,
	// so one refused by a full UART ring would be lost for good, and the
	// module would read whatever follows as part of what it is waiting for.
	int32_t tx_room = uv_uart_get_tx_free_space(this->uart);
	if (tx_count > tx_room) {
		tx_count = tx_room;
	}
	else {
	}
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
		if (this->wifi_flags != NULL &&
				(*(this->wifi_flags) & ESP32_CONF_FLAGS_ECHO)) {
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
	// A line that completed before this command was sent belongs to the
	// previous exchange -- typically the trailing OK of a fire-and-forget
	// query such as AT+CWJAP? or AT+CIPSNTPTIME?. Dropping it here keeps the
	// next rx_at_match from taking it for the answer to the command we are
	// about to send.
	this->at_resp_has_pending = false;
}


/// @brief: at_resp_reset() for a command being sent, which must not cost a
/// line the module is in the middle of sending of its own accord.
///
/// A reply still arriving belongs to the exchange before this one and goes,
/// as with at_resp_reset(), so it cannot be taken for the answer to the new
/// command. But the module's unsolicited lines -- "+..." events, above all the
/// header of a +MQTTSUBRECV, and "WIFI ..." -- arrive whenever they please, and
/// with a publish going out for every forwarded frame, one was often half
/// received when the next AT+MQTTPUBRAW was sent. Cutting a +MQTTSUBRECV
/// header in two left the rest of it unrecognisable, so the payload behind it
/// was read as a garbage line and the frame it carried was lost: one in a few
/// dozen frames to the device while its bus was busy, which is enough to make
/// a firmware flash over the remote link impossible.
static void at_resp_reset_for_cmd(uv_esp32_st *this) {
	this->at_busy = false;
	uint8_t n = (this->at_resp_i < 4U) ? this->at_resp_i : 4U;
	bool unsolicited = ((this->at_resp_i > 0U) &&
			((this->at_resp[0] == '+') ||
			(strncmp(this->at_resp, "WIFI", n) == 0)));
	if (unsolicited) {
		// keep the line, only the completed one waiting for a reader goes
		this->at_resp_has_pending = false;
	}
	else {
		at_resp_reset(this);
	}
}


static void esp32_reset(uv_esp32_st *this) {
	uv_streambuffer_clear(&this->tx_streambuffer);
	uv_streambuffer_clear(&this->rx_datastream);
	at_resp_reset(this);
#if CONFIG_ESP32_MQTT
	// the rx stream is gone, so any half-captured MQTT payload is unrecoverable
	this->mqtt_subrecv.active = false;
	this->mqtt_subrecv.received_len = 0;
	this->mqtt_subrecv.expected_len = 0;
	this->mqtt_subrecv.drop = false;
#endif
#if CONFIG_ESP32_ESPNOW
	// The module forgets every ESP-NOW setting with the reset, so all of them
	// are sent again, and whatever was in flight is gone.
	this->espnow.rx.active = false;
	this->espnow.rx.received_len = 0;
	this->espnow.rx.expected_len = 0;
	this->espnow.rx.drop = false;
	this->espnow.phase = ESPNOW_PHASE_IDLE;
	this->espnow.sending = ESP32_ESPNOW_SET_COUNT;
	this->espnow.state = ESP32_ESPNOW_STATE_DISABLED;
	this->espnow.cur_channel = 0;
	this->espnow.version = 0;
	this->espnow.rx_seen = false;
	this->espnow.disc_req = false;
	for (uint8_t i = 0; i < (uint8_t) ESP32_ESPNOW_SET_COUNT; i++) {
		this->espnow.dirty[i] = true;
	}
	if (this->espnow.tx_pending) {
		this->espnow.diag.tx_fail++;
		this->espnow.tx_last_ok = false;
		this->espnow.tx_pending = false;
	}
	else {
	}
#endif
	this->rx_at_cmd = NULL;
	// a scan asked for is not made by a module starting over. A join is: the
	// module joins the configured network on its own once it is up.
	this->scan_req = false;
	this->wifi_req = ESP32_WIFI_REQ_NONE;
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
	// The wifi password is the second argument of AT+CWJAP and nothing else
	// here carries a secret, so it is the one field worth hiding: a debug log
	// is read over the CAN terminal and pasted into bug reports.
	ESP32_DEBUG(this, "ESP32 -> %s%s%s%s%s\n", cmd,
			(arg1 != NULL) ? "=\"" : "",
			(arg1 != NULL) ? arg1 : "",
			(arg2 != NULL) ? "\",\"" : ((arg1 != NULL) ? "\"" : ""),
			(arg2 != NULL) ?
					((strstr(cmd, "CWJAP") != NULL) ? "***\"" : arg2) : "");
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
	at_resp_reset_for_cmd(this);
}


#if CONFIG_ESP32_MQTT || CONFIG_ESP32_SNTP || CONFIG_ESP32_ESPNOW
/// @brief: Sends a fully-formed AT command line followed by \r\n via
/// tx_streambuffer. The caller is responsible for any escaping inside the
/// composed line. Used for multi-arg AT commands that don't fit the
/// "AT+CMD=\"arg1\",\"arg2\"" shape (e.g. AT+MQTTUSERCFG, AT+MQTTCONN,
/// AT+CIPSNTPCFG).
static void send_at_cmd_raw(uv_esp32_st *this, const char *line) {
	// AT+MQTTUSERCFG carries the broker password inline, so that one is shown
	// only up to its command word. Everything else composed this way is
	// addresses and numbers.
	if (strstr(line, "MQTTUSERCFG") != NULL) {
		ESP32_DEBUG(this, "ESP32 -> AT+MQTTUSERCFG=... (credentials hidden)\n");
	}
	else {
		ESP32_DEBUG(this, "ESP32 -> %s\n", line);
	}
	uv_mutex_lock(&this->txstream_mutex);
	uv_streambuffer_push(&this->tx_streambuffer,
			(char *) line, strlen(line), 100);
	uv_streambuffer_push(&this->tx_streambuffer,
			"\r\n", 2, 100);
	uv_mutex_unlock(&this->txstream_mutex);
	at_resp_reset_for_cmd(this);
}
#endif


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


static void network_scan_start(uv_esp32_st *this, bool store);

#if CONFIG_ESP32_MQTT
// Forward decl — defined below.
static void mqtt_set_state(uv_esp32_st *this, uv_esp32_mqtt_states_e state);
#endif
static void set_state(uv_esp32_st *this, uv_esp32_states_e state);


#if CONFIG_ESP32_ESPNOW

/// @brief: Parses the part after "+ENOWDEV:" of a device found by discovery,
/// "<mac>",<rssi>,<channel>,"<name>", and hands it to the callback.
static void espnow_parse_dev(uv_esp32_st *this, char *p) {
	// the MAC is fixed length: a quote, 17 characters, a quote
	if ((strlen(p) > 20U) && (p[0] == '"') && (p[18] == '"') &&
			(p[19] == ',')) {
		char mac[ESP32_MAC_STR_LEN];
		memcpy(mac, &p[1], ESP32_MAC_STR_LEN - 1);
		mac[ESP32_MAC_STR_LEN - 1] = '\0';
		char *q = &p[20];
		int8_t rssi = (int8_t) strtol(q, &q, 10);
		uint8_t channel = 0;
		char *name = NULL;
		if (*q == ',') {
			channel = (uint8_t) strtoul(q + 1, &q, 10);
			if ((q[0] == ',') && (q[1] == '"')) {
				name = &q[2];
				// the name is the rest of the line up to its last quote, so
				// one with quotes or commas of its own comes through whole
				char *end = strrchr(name, '"');
				if (end != NULL) {
					*end = '\0';
				}
				else {
				}
			}
			else {
			}
		}
		else {
		}
		if (name != NULL) {
			ESP32_DEBUG(this, "ESP32: ESP-NOW found %s '%s', ch %u, %i dBm\n",
					mac, name, (unsigned int) channel, (int) rssi);
			if (this->espnow.dev_callb != NULL) {
				this->espnow.dev_callb(mac, rssi, channel, name);
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


/// @brief: Ends a raw +ENOWRECV payload capture, delivering it unless it was
/// too big for the buffer.
static void enowrecv_finish(uv_esp32_st *this) {
	uv_esp32_espnow_st *e = &this->espnow;
	if (e->rx.drop) {
		e->diag.rx_drop++;
	}
	else {
		e->diag.rx++;
		e->rx_tick = uv_rtos_get_tick_count();
		e->rx_seen = true;
		if (e->rx_callb != NULL) {
			e->rx_callb(e->rx.data, e->rx.expected_len, e->rx.rssi);
		}
		else {
		}
	}
	e->rx.active = false;
	e->rx.received_len = 0;
	e->rx.expected_len = 0;
	e->rx.drop = false;
}


/// @brief: Consumes one raw payload byte of the capture in progress.
static void enowrecv_feed(uv_esp32_st *this, char c) {
	uv_esp32_espnow_st *e = &this->espnow;
	if (!e->rx.drop &&
			(e->rx.received_len < ESP32_ESPNOW_PAYLOAD_MAX_LEN)) {
		e->rx.data[e->rx.received_len] = (uint8_t) c;
	}
	else {
		// oversized payload: still counted, so the stream stays in sync
	}
	e->rx.received_len++;
	if (e->rx.received_len >= e->rx.expected_len) {
		enowrecv_finish(this);
	}
	else {
	}
}


/// @brief: Called right after a ',' has been accumulated into at_resp. If the
/// partial line is now a complete '+ENOWRECV:<rssi>,<len>,' header, switches
/// the receiver into raw payload capture for <len> bytes.
///
/// @return: true if a capture was started.
static bool enowrecv_try_start(uv_esp32_st *this) {
	bool ret = false;
	// see subrecv_try_start
	this->at_resp[this->at_resp_i] = '\0';
	if (strncmp(this->at_resp, ENOWRECV_PREFIX, ENOWRECV_PREFIX_LEN) == 0) {
		char *rssi_start = &this->at_resp[ENOWRECV_PREFIX_LEN];
		char *rssi_end = NULL;
		long rssi = strtol(rssi_start, &rssi_end, 10);
		if ((rssi_end != rssi_start) && (rssi_end[0] == ',')) {
			char *len_start = &rssi_end[1];
			char *len_end = NULL;
			unsigned long datalen = strtoul(len_start, &len_end, 10);
			// the header ends exactly at the comma just accumulated
			if ((len_end != len_start) &&
					(len_end[0] == ',') &&
					(len_end[1] == '\0')) {
				uv_esp32_espnow_st *e = &this->espnow;
				e->rx.rssi = (int8_t) rssi;
				e->rx.expected_len = (uint16_t) datalen;
				e->rx.received_len = 0;
				e->rx.drop = (datalen > ESP32_ESPNOW_PAYLOAD_MAX_LEN);
				e->rx.active = true;
				if (e->rx.drop) {
					ESP32_DEBUG(this, "ESP32: ESP-NOW frame of %u B exceeds "
							"the buffer, dropping\n", (unsigned int) datalen);
				}
				else {
				}
				at_resp_reset(this);
				if (e->rx.expected_len == 0) {
					enowrecv_finish(this);
				}
				else {
				}
				ret = true;
			}
			else {
			}
		}
		else {
		}
	}
	else {
	}
	return ret;
}

#endif


/// @brief: Recognizes async +MQTT* event lines in this->at_resp and reacts.
/// Returns true if the line was an async event (so callers can drop it),
/// false otherwise (so the line remains for AT-cmd response matching).
static bool dispatch_mqtt_line(uv_esp32_st *this) {
	bool dispatched = false;
#if CONFIG_ESP32_MQTT
	if (strstr(this->at_resp, "+MQTTCONNECTED") != NULL) {
		ESP32_DEBUG(this, "ESP32: +MQTTCONNECTED\n");
		if (this->mqtt_state == ESP32_MQTT_STATE_CONN) {
			this->diag.mqtt_connects++;
			mqtt_set_state(this, ESP32_MQTT_STATE_CONNECTED);
		}
		else {
		}
		dispatched = true;
	}
	else if (strstr(this->at_resp, "+MQTTDISCONNECTED") != NULL) {
		ESP32_DEBUG(this, "ESP32: +MQTTDISCONNECTED\n");
		// The close sent from CLEAN may report the link it tears down; that
		// is the close succeeding, not a session lost.
		if (this->mqtt_state != ESP32_MQTT_STATE_DISABLED &&
				this->mqtt_state != ESP32_MQTT_STATE_INIT &&
				this->mqtt_state != ESP32_MQTT_STATE_CLEAN) {
			if (this->mqtt_state == ESP32_MQTT_STATE_CONNECTED) {
				this->diag.mqtt_lost++;
				uv_esp32_diag_err(this, ESP32_ERR_MQTT_LOST);
			}
			else {
				// refused while opening: the broker or the TLS handshake
				this->diag.mqtt_conn_fails++;
				uv_esp32_diag_err(this, ESP32_ERR_MQTT_CONN);
			}
			mqtt_set_state(this, ESP32_MQTT_STATE_ERROR);
		}
		else {
		}
		dispatched = true;
	}
	else
#endif
	if (strstr(this->at_resp, "+CWJAP:") != NULL) {
		// Answer to the periodic signal strength query:
		// +CWJAP:<ssid>,<bssid>,<channel>,<rssi>[,...]. The ssid is quoted and
		// may contain commas and (once the line parser has decoded the escapes)
		// quotes of its own, so counting fields from the start of the line is
		// not safe. The bssid is the last quoted field, so its closing quote is
		// the last quote on the line — count from there.
		char *s = strrchr(this->at_resp, '"');
		if (s != NULL) {
			// skip ,<channel>, to land on <rssi>
			s = strchr(s, ',');
			if (s != NULL) {
				s = strchr(s + 1, ',');
			}
			else {
			}
			if (s != NULL) {
				this->rssi = (int8_t) strtol(s + 1, NULL, 10);
			}
			else {
			}
		}
		else {
		}
		dispatched = true;
	}
	else if (strncmp(this->at_resp, "busy p", 6) == 0) {
		// The command just sent arrived while the module was still processing
		// the one before it, and was dropped. Noted for whoever sent it.
		ESP32_DEBUG(this, "ESP32: module busy, command dropped\n");
		this->at_busy = true;
		dispatched = true;
	}
	else if (strstr(this->at_resp, "WIFI DISCONNECT") != NULL) {
		// Unsolicited: the AP is gone. A join in progress may report the
		// previous AP going the same way, so this is only acted on while
		// joined (see rxtx_task) and cleared when a join completes.
		ESP32_DEBUG(this, "ESP32: WIFI DISCONNECT\n");
		this->wifi_lost = true;
		dispatched = true;
	}
	else if ((this->state == ESP32_STATE_LEFT_NETWORK) &&
			(strstr(this->at_resp, "WIFI GOT IP") != NULL)) {
		// The module rejoined the AP on its own. Only while left: during
		// CONNECT_WIFI this line is the answer the join is waiting for.
		ESP32_DEBUG(this, "ESP32: WiFi rejoined by the module\n");
		this->wifi_lost = false;
		set_state(this, ESP32_STATE_JOINED_NETWORK);
		dispatched = true;
	}
#if CONFIG_ESP32_ESPNOW
	else if (strncmp(this->at_resp, ENOWDEV_PREFIX, ENOWDEV_PREFIX_LEN) == 0) {
		espnow_parse_dev(this, &this->at_resp[ENOWDEV_PREFIX_LEN]);
		dispatched = true;
	}
	else if (strncmp(this->at_resp, ENOWINIT_PREFIX, ENOWINIT_PREFIX_LEN) == 0) {
		// +ENOWINIT:<inited>,<channel>,<version>
		char *p = &this->at_resp[ENOWINIT_PREFIX_LEN];
		(void) strtoul(p, &p, 10);
		if (*p == ',') {
			this->espnow.cur_channel = (uint8_t) strtoul(p + 1, &p, 10);
			if (*p == ',') {
				this->espnow.version = (uint8_t) strtoul(p + 1, NULL, 10);
			}
			else {
			}
			ESP32_DEBUG(this, "ESP32: ESP-NOW v%u on channel %u\n",
					(unsigned int) this->espnow.version,
					(unsigned int) this->espnow.cur_channel);
		}
		else {
		}
		dispatched = true;
	}
#endif
#if CONFIG_ESP32_SNTP
	else if (strstr(this->at_resp, SNTPTIME_PREFIX) != NULL) {
		// Answer to the network time query:
		// +CIPSNTPTIME:Thu Aug 21 14:27:00 2026. The stamp is fixed-shape, so
		// the year is simply the last space-separated field. The module answers
		// this from power-on onwards, reporting an epoch-based date until SNTP
		// has actually reached a server, which is why the year is what decides
		// whether the clock can be trusted.
		char *stamp = strstr(this->at_resp, SNTPTIME_PREFIX) + SNTPTIME_PREFIX_LEN;
		strncpy(this->time_str, stamp, sizeof(this->time_str) - 1);
		this->time_str[sizeof(this->time_str) - 1] = '\0';
		char *year = strrchr(this->time_str, ' ');
		if (year != NULL) {
			this->time_year = (uint16_t) strtol(year + 1, NULL, 10);
		}
		else {
			this->time_year = 0;
		}
		if ((this->time_year >= ESP32_SNTP_MIN_YEAR) &&
				!this->time_synced) {
			this->time_synced = true;
			ESP32_DEBUG(this, "ESP32: network time %s\n", this->time_str);
		}
		else {
		}
		dispatched = true;
	}
	else if (strstr(this->at_resp, CIPSTA_IP_PREFIX) != NULL) {
		// +CIPSTA:ip:"192.168.1.23"
		char *addr = strstr(this->at_resp, CIPSTA_IP_PREFIX) +
				CIPSTA_IP_PREFIX_LEN;
		char *start = strchr(addr, '"');
		if (start != NULL) {
			start++;
			char *end = strchr(start, '"');
			if (end != NULL) {
				uint16_t len = (uint16_t) (end - start);
				if (len >= sizeof(this->ip_str)) {
					len = sizeof(this->ip_str) - 1;
				}
				else {
				}
				memcpy(this->ip_str, start, len);
				this->ip_str[len] = '\0';
				ESP32_DEBUG(this, "ESP32: address %s\n", this->ip_str);
			}
			else {
			}
		}
		else {
		}
		dispatched = true;
	}
	else if (strstr(this->at_resp, SYSTIMESTAMP_PREFIX) != NULL) {
		// +SYSTIMESTAMP:<seconds since the epoch>. Asked for alongside the
		// asctime stamp because callers that compare dates -- certificate
		// validity, most of all -- want a number, and converting the asctime
		// back into one on the MCU is calendar arithmetic for no reason.
		char *stamp = strstr(this->at_resp, SYSTIMESTAMP_PREFIX) +
				SYSTIMESTAMP_PREFIX_LEN;
		uint32_t epoch = (uint32_t) strtoul(stamp, NULL, 10);
		if (epoch >= ESP32_SNTP_MIN_EPOCH) {
			this->time_epoch = epoch;
			if (!this->time_synced) {
				this->time_synced = true;
				ESP32_DEBUG(this, "ESP32: network time set, epoch %u\n",
						(unsigned int) epoch);
			}
			else {
			}
		}
		else {
			// the module is still counting up from the epoch
		}
		dispatched = true;
	}
#endif
	else {
		// Note: +MQTTSUBRECV never reaches here. Its payload is raw binary and
		// is captured off rx_datastream by at_feed_char below, before the line
		// parser can escape-decode it, split it on '\n' or truncate it to the
		// AT line buffer.
	}
	return dispatched;
}


#if CONFIG_ESP32_MQTT

/// @brief: Ends a raw payload capture, delivering it to the subscriber unless
/// it was too big for the buffer.
static void subrecv_finish(uv_esp32_st *this) {
	if (!this->mqtt_subrecv.drop &&
			(this->mqtt_rx_callb != NULL)) {
		this->mqtt_rx_callb(this->mqtt_subrecv.topic,
				this->mqtt_subrecv.data,
				this->mqtt_subrecv.expected_len);
	}
	else {
	}
	this->mqtt_subrecv.active = false;
	this->mqtt_subrecv.received_len = 0;
	this->mqtt_subrecv.expected_len = 0;
	this->mqtt_subrecv.drop = false;
}


/// @brief: Consumes one raw payload byte of the capture in progress.
static void subrecv_feed(uv_esp32_st *this, char c) {
	if (!this->mqtt_subrecv.drop &&
			(this->mqtt_subrecv.received_len < ESP32_MQTT_PAYLOAD_MAX_LEN)) {
		this->mqtt_subrecv.data[this->mqtt_subrecv.received_len] = (uint8_t) c;
	}
	else {
		// oversized payload: still counted, so the stream stays in sync
	}
	this->mqtt_subrecv.received_len++;
	if (this->mqtt_subrecv.received_len >= this->mqtt_subrecv.expected_len) {
		subrecv_finish(this);
	}
	else {
	}
}


/// @brief: Called right after a ',' has been accumulated into at_resp. If the
/// partial line is now a complete '+MQTTSUBRECV:<link>,"<topic>",<len>,'
/// header, switches the receiver into raw payload capture for <len> bytes.
///
/// @return: true if a capture was started.
static bool subrecv_try_start(uv_esp32_st *this) {
	bool ret = false;
	// at_resp is only null-terminated once a line completes, so terminate the
	// partial line in place to parse it. at_resp_accumulate guarantees
	// at_resp_i <= ESP32_AT_RESP_LEN - 1, so this stays in bounds.
	this->at_resp[this->at_resp_i] = '\0';
	if (strncmp(this->at_resp, SUBRECV_PREFIX, SUBRECV_PREFIX_LEN) == 0) {
		char *topic_start = strchr(&this->at_resp[SUBRECV_PREFIX_LEN], '"');
		char *topic_end = (topic_start != NULL) ?
				strchr(topic_start + 1, '"') : NULL;
		if ((topic_end != NULL) &&
				(topic_end[1] == ',')) {
			char *len_start = &topic_end[2];
			char *len_end = NULL;
			unsigned long datalen = strtoul(len_start, &len_end, 10);
			// The header ends exactly at the ',' just accumulated: the length
			// field must be followed by that comma and nothing else yet.
			// Anything else (an escaped ',' inside the topic, the comma after
			// the link id) leaves the line incomplete and is ignored here.
			if ((len_end != len_start) &&
					(len_end[0] == ',') &&
					(len_end[1] == '\0')) {
				topic_start++;
				uint16_t topic_len = (uint16_t) (topic_end - topic_start);
				if (topic_len >= ESP32_MQTT_TOPIC_MAX_LEN) {
					topic_len = ESP32_MQTT_TOPIC_MAX_LEN - 1;
				}
				else {
				}
				memcpy(this->mqtt_subrecv.topic, topic_start, topic_len);
				this->mqtt_subrecv.topic[topic_len] = '\0';
				this->mqtt_subrecv.expected_len = (uint16_t) datalen;
				this->mqtt_subrecv.received_len = 0;
				this->mqtt_subrecv.drop =
						(datalen > ESP32_MQTT_PAYLOAD_MAX_LEN);
				this->mqtt_subrecv.active = true;
				if (this->mqtt_subrecv.drop) {
					ESP32_DEBUG(this,
							"ESP32: SUBRECV %u B exceeds payload buffer, "
							"dropping\n", (unsigned int) datalen);
				}
				else {
				}
				at_resp_reset(this);
				if (this->mqtt_subrecv.expected_len == 0) {
					// zero-length payload completes without any further bytes
					subrecv_finish(this);
				}
				else {
				}
				ret = true;
			}
			else {
			}
		}
		else {
		}
	}
	else {
	}
	return ret;
}

#endif


/// @brief: Feeds one received character into either the raw MQTT payload
/// capture or the AT line parser, starting a capture when a +MQTTSUBRECV
/// header completes.
///
/// @return: true if a complete AT line is now in at_resp.
static bool at_feed_char(uv_esp32_st *this, char c) {
	bool line_complete = false;
#if CONFIG_ESP32_MQTT
	if (this->mqtt_subrecv.active) {
		subrecv_feed(this, c);
	}
	else
#endif
#if CONFIG_ESP32_ESPNOW
	if (this->espnow.rx.active) {
		enowrecv_feed(this, c);
	}
	else
#endif
	if (at_resp_accumulate(this, c)) {
		this->at_resp_i = 0;
		line_complete = true;
	}
	else if (c == ',') {
		bool started = false;
#if CONFIG_ESP32_MQTT
		started = subrecv_try_start(this);
#endif
#if CONFIG_ESP32_ESPNOW
		if (!started) {
			started = enowrecv_try_start(this);
		}
		else {
		}
#endif
		(void) started;
	}
	else {
	}
	return line_complete;
}


#if CONFIG_ESP32_MQTT || CONFIG_ESP32_ESPNOW
/// @brief: True while a binary payload is being captured off the rx stream,
/// in which a '>' is data and not a prompt.
static bool raw_capture_active(uv_esp32_st *this) {
	bool ret = false;
#if CONFIG_ESP32_MQTT
	if (this->mqtt_subrecv.active) {
		ret = true;
	}
	else {
	}
#endif
#if CONFIG_ESP32_ESPNOW
	if (this->espnow.rx.active) {
		ret = true;
	}
	else {
	}
#endif
	return ret;
}
#endif


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
			if (at_feed_char(this, c)) {
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


#if CONFIG_ESP32_MQTT || CONFIG_ESP32_ESPNOW
/// @brief: Reads the rx stream up to the bare '>' prompt of a raw data
/// command (AT+MQTTPUBRAW, AT+ENOWSEND). The prompt is not a line, so the
/// line parser would swallow it; everything before it is fed through the
/// parser instead of being thrown away, so that a message arriving meanwhile
/// is not lost: complete lines go to the async dispatcher or are held for the
/// AT matcher.
///
/// @param error: If not NULL, set when an ERROR line comes instead: the
/// command was refused and no prompt follows. NULL leaves such a line for the
/// AT matcher like any other.
/// @return: true once the prompt was read.
static bool rx_wait_prompt(uv_esp32_st *this, bool *error) {
	char c;
	bool prompt_seen = false;
	while (!prompt_seen &&
			uv_streambuffer_pop(&this->rx_datastream, &c, 1, 0)) {
		if (!raw_capture_active(this) && (c == '>')) {
			prompt_seen = true;
		}
		else if (at_feed_char(this, c)) {
			if (dispatch_mqtt_line(this)) {
				at_consume_line(this);
			}
			else if ((error != NULL) &&
					(strstr(this->at_resp, "ERROR") != NULL)) {
				at_consume_line(this);
				*error = true;
				break;
			}
			else {
				at_save_pending(this);
			}
		}
		else {
		}
	}
	return prompt_seen;
}
#endif


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
	case ESP32_STATE_SET_CWLAPOPT:
		str = "SET_CWLAPOPT";
		break;
	case ESP32_STATE_SET_UART:
		str = "SET_UART";
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
		case ESP32_STATE_INIT:
#if CONFIG_ESP32_SNTP
			// the module is about to be reset and its clock goes with it
			this->time_synced = false;
			this->time_epoch = 0;
			this->time_year = 0;
			this->time_str[0] = '\0';
			this->sntp_cfg_sent = false;
#endif
			break;
		case ESP32_STATE_WAIT_READY:
		case ESP32_STATE_TEST_AT:
		case ESP32_STATE_DISABLE_ECHO:
		case ESP32_STATE_SET_CWMODE:
		case ESP32_STATE_SET_CWLAPOPT:
		case ESP32_STATE_SET_UART:
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
		case ESP32_STATE_JOINED_NETWORK:
#if CONFIG_ESP32_ESPNOW
			// the AP decides the channel now, and a scan just made may have
			// left the radio anywhere
			this->espnow.dirty[ESP32_ESPNOW_SET_CHANNEL] = true;
			this->espnow.dirty[ESP32_ESPNOW_SET_QUERY] = true;
#endif
			// the join's own OK comes after "WIFI GOT IP"
			uv_delay_init(&this->at_quiet_delay, ESP32_AT_QUIET_MS);
			// ask for the signal strength right away rather than after a full
			// poll interval of showing "not known"
			uv_delay_init(&this->rssi_delay, 0);
			// a new join means a new address
			this->ip_str[0] = '\0';
#if CONFIG_ESP32_SNTP
			// (Re)configure SNTP on every join, first thing: it costs one AT
			// command, and nothing else the device does over TLS can work
			// until the clock is set.
			this->sntp_cfg_sent = false;
			this->sntp_query_alt = false;
			this->sntp_cfg_retry_ms = ESP32_SNTP_CFG_RETRY_MIN_MS;
			uv_delay_init(&this->sntp_cfg_retry, this->sntp_cfg_retry_ms);
#endif
			break;
		case ESP32_STATE_LEFT_NETWORK:
#if CONFIG_ESP32_ESPNOW
			// back on our own channel, see ESP32_STATE_JOINED_NETWORK
			this->espnow.dirty[ESP32_ESPNOW_SET_CHANNEL] = true;
			this->espnow.dirty[ESP32_ESPNOW_SET_QUERY] = true;
#endif
			// entered right after AT+CWQAP, whose answer is still to come
			uv_delay_init(&this->at_quiet_delay, ESP32_AT_QUIET_MS);
			uv_delay_init(&this->timeout, ESP32_RECONNECT_MS);
			this->rssi = 0;
			this->ip_str[0] = '\0';
			break;
		default:
			break;
		}
	}
}


const char *uv_esp32_err_to_str(uv_esp32_err_e err) {
	// Short: shown on a single label line with the error's age beside it.
	static const char *const strs[ESP32_ERR_COUNT] = {
			"none",
			"WiFi join",
			"MQTT usercfg",
			"MQTT conncfg",
			"MQTT connect",
			"MQTT lost",
			"MQTT publish",
			"MQTT pub timeout",
			"MQTT subscribe",
			"ESP-NOW command"
	};
	const char *str = "unknown";
	if ((uint32_t) err < (uint32_t) ESP32_ERR_COUNT) {
		str = strs[err];
	}
	else {
	}
	return str;
}


void uv_esp32_diag_err(uv_esp32_st *this, uv_esp32_err_e err) {
	this->diag.last_err = err;
	this->diag.last_err_tick = uv_rtos_get_tick_count();
}


#if CONFIG_ESP32_MQTT

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
	case ESP32_MQTT_STATE_CLEAN:
		str = "CLEAN";
		break;
	default:
		str = "UNKNOWN";
		break;
	}
	return str;
}


/// @brief: Abandons any in-flight (un)subscribe and marks every registered
/// subscription as not yet issued, so the registry is replayed on the next
/// broker connection. A broker session starts with no subscriptions, so this
/// runs on every MQTT state change.
static void mqtt_sub_invalidate(uv_esp32_st *this) {
	uv_mutex_lock(&this->mqtt_sub_mutex);
	for (uint8_t i = 0; i < ESP32_MQTT_SUBSCRIPTION_COUNT; i++) {
		uv_esp32_mqtt_sub_st *s = &this->mqtt_subs[i];
		s->sent = false;
		if (s->unsub) {
			// nothing to unsubscribe from on a connection that no longer
			// exists — just drop the entry
			s->in_use = false;
			s->unsub = false;
		}
		else {
		}
	}
	uv_mutex_unlock(&this->mqtt_sub_mutex);
	this->mqtt_sub_active = NULL;
	this->mqtt_sub_phase = MQTT_SUB_PHASE_IDLE;
}


static void mqtt_set_state(uv_esp32_st *this, uv_esp32_mqtt_states_e state) {
	if (this->mqtt_state != state) {
		ESP32_DEBUG(this, "ESP32: MQTT %s -> %s\n",
				uv_esp32_mqtt_state_to_str(this->mqtt_state),
				uv_esp32_mqtt_state_to_str(state));
		this->mqtt_state = state;
		mqtt_sub_invalidate(this);
		switch (state) {
		case ESP32_MQTT_STATE_CLEAN:
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


/// @brief: Picks which publish slot the drainer should service next.
/// Returns the slot that is currently mid-AT-transaction (phase != IDLE)
/// if any — at most one such slot exists, and we never preempt it. If no
/// slot is mid-flight, picks the highest-priority pending slot (lowest
/// numeric priority value; ties break by smallest seq for FIFO). Returns
/// NULL when the pool is empty.
static uv_esp32_mqtt_slot_st *mqtt_select_slot(uv_esp32_st *this) {
	uv_esp32_mqtt_slot_st *active = NULL;
	uv_esp32_mqtt_slot_st *best_queued = NULL;
	uv_mutex_lock(&this->mqtt_pub_mutex);
	for (uint8_t i = 0; i < ESP32_MQTT_PUBLISH_SLOT_COUNT; i++) {
		uv_esp32_mqtt_slot_st *s = &this->mqtt_pub_slots[i];
		if (!s->in_use) {
			continue;
		}
		else if (s->phase != MQTT_PUB_PHASE_IDLE) {
			active = s;
			break;
		}
		else if (best_queued == NULL ||
				s->priority < best_queued->priority ||
				(s->priority == best_queued->priority &&
						s->seq < best_queued->seq)) {
			best_queued = s;
		}
		else {
		}
	}
	uv_mutex_unlock(&this->mqtt_pub_mutex);
	return (active != NULL) ? active : best_queued;
}


/// @brief: Releases a slot after the AT publish transaction completes
/// (success or terminal failure). Drainer-side only.
static void mqtt_slot_release(uv_esp32_st *this,
		uv_esp32_mqtt_slot_st *slot) {
	uv_mutex_lock(&this->mqtt_pub_mutex);
	slot->in_use = false;
	slot->phase = MQTT_PUB_PHASE_IDLE;
	uv_mutex_unlock(&this->mqtt_pub_mutex);
}


/// @brief: Drops every queued publish. Called when MQTT leaves CONNECTED;
/// any in-flight AT transaction is half-done from the ESP32's perspective
/// so we cannot safely resume it, and queued messages will simply be
/// republished by their producers (periodic streams) or lost (events).
static void mqtt_slot_clear_all(uv_esp32_st *this) {
	uv_mutex_lock(&this->mqtt_pub_mutex);
	for (uint8_t i = 0; i < ESP32_MQTT_PUBLISH_SLOT_COUNT; i++) {
		this->mqtt_pub_slots[i].in_use = false;
		this->mqtt_pub_slots[i].phase = MQTT_PUB_PHASE_IDLE;
	}
	uv_mutex_unlock(&this->mqtt_pub_mutex);
}


/// @brief: Issues the next pending (un)subscribe, if any. Only called with the
/// AT link free — no publish in flight and no subscribe already outstanding.
static void mqtt_sub_start_next(uv_esp32_st *this) {
	uv_esp32_mqtt_sub_st *target = NULL;
	uv_mutex_lock(&this->mqtt_sub_mutex);
	for (uint8_t i = 0; i < ESP32_MQTT_SUBSCRIPTION_COUNT; i++) {
		uv_esp32_mqtt_sub_st *s = &this->mqtt_subs[i];
		if (s->in_use &&
				(s->unsub || !s->sent)) {
			target = s;
			break;
		}
		else {
		}
	}
	uv_mutex_unlock(&this->mqtt_sub_mutex);

	if (target != NULL) {
		char line[ESP32_MQTT_TOPIC_MAX_LEN + 32];
		if (target->unsub) {
			snprintf(line, sizeof(line), "AT+MQTTUNSUB=%d,\"%s\"",
					ESP32_MQTT_LINK_ID, target->topic);
		}
		else {
			snprintf(line, sizeof(line), "AT+MQTTSUB=%d,\"%s\",%u",
					ESP32_MQTT_LINK_ID, target->topic,
					(unsigned int) target->qos);
		}
		ESP32_DEBUG(this, "ESP32: MQTT %s '%s'\n",
				target->unsub ? "UNSUB" : "SUB", target->topic);
		send_at_cmd_raw(this, line);
		uv_delay_init(&this->mqtt_sub_timeout, ESP32_MQTT_AT_TIMEOUT_MS);
		this->mqtt_sub_active = target;
		this->mqtt_sub_phase = MQTT_SUB_PHASE_AT_SENT;
	}
	else {
	}
}


/// @brief: Resolves the in-flight (un)subscribe. A failed subscribe keeps its
/// registry entry with sent == false and backs off, so it is retried rather
/// than lost.
static void mqtt_sub_finish(uv_esp32_st *this, bool ok) {
	uv_esp32_mqtt_sub_st *s = this->mqtt_sub_active;
	if (s != NULL) {
		uv_mutex_lock(&this->mqtt_sub_mutex);
		if (s->unsub) {
			// terminal either way: the caller no longer wants this topic
			s->in_use = false;
			s->unsub = false;
			s->sent = false;
		}
		else if (ok) {
			s->sent = true;
		}
		else {
			ESP32_DEBUG(this, "ESP32: MQTT SUB '%s' failed\n", s->topic);
			this->diag.mqtt_sub_fails++;
			uv_esp32_diag_err(this, ESP32_ERR_MQTT_SUB);
		}
		uv_mutex_unlock(&this->mqtt_sub_mutex);
	}
	else {
	}
	this->mqtt_sub_active = NULL;
	if (ok) {
		this->mqtt_sub_phase = MQTT_SUB_PHASE_IDLE;
	}
	else {
		// don't hammer the AT link retrying a subscription the broker or the
		// module just refused
		uv_delay_init(&this->mqtt_sub_timeout, ESP32_MQTT_SUB_RETRY_MS);
		this->mqtt_sub_phase = MQTT_SUB_PHASE_BACKOFF;
	}
}

#endif


#if CONFIG_ESP32_MQTT
/// @brief: True when the module's clock is good enough for the configured
/// MQTT scheme. Certificate validity dates are compared against that clock, so
/// a TLS scheme (>= 2) connecting before SNTP has set it fails the handshake
/// with the server certificate reported as not yet valid; plain TCP (scheme 1)
/// does not care. Without SNTP support built in nothing can be checked, and
/// the answer is "as good as it gets".
static bool mqtt_tls_time_ok(uv_esp32_st *this) {
	bool ret = true;
#if CONFIG_ESP32_SNTP
	if (this->mqtt_scheme >= 2) {
		ret = this->time_synced;
	}
	else {
	}
#endif
	return ret;
}
#endif


#if CONFIG_ESP32_ESPNOW

const char *uv_esp32_espnow_state_to_str(uv_esp32_espnow_states_e state) {
	static const char *const strs[ESP32_ESPNOW_STATE_COUNT] = {
			"DISABLED",
			"CONFIG",
			"READY",
			"ERROR"
	};
	const char *str = "UNKNOWN";
	if ((uint32_t) state < (uint32_t) ESP32_ESPNOW_STATE_COUNT) {
		str = strs[state];
	}
	else {
	}
	return str;
}


/// @brief: Sends the first setting still to be sent. Returns false when all
/// of them have been.
static bool espnow_config_next(uv_esp32_st *this) {
	uv_esp32_espnow_st *e = &this->espnow;
	uv_esp32_espnow_set_e set = ESP32_ESPNOW_SET_COUNT;
	for (uint8_t i = 0; i < (uint8_t) ESP32_ESPNOW_SET_COUNT; i++) {
		if (e->dirty[i]) {
			set = (uv_esp32_espnow_set_e) i;
			break;
		}
		else {
		}
	}
	if (set != ESP32_ESPNOW_SET_COUNT) {
		// lowered before the value is read: a setter writing meanwhile raises
		// it again, and the new value is sent next
		e->dirty[set] = false;
		char line[64];
		switch (set) {
		case ESP32_ESPNOW_SET_CHANNEL:
			// joined, the AP has the channel and 0 keeps it
			snprintf(line, sizeof(line), "AT+ENOWINIT=%u",
					(this->state == ESP32_STATE_JOINED_NETWORK) ?
							0U : (unsigned int) e->channel);
			break;
		case ESP32_ESPNOW_SET_QUERY:
			snprintf(line, sizeof(line), "AT+ENOWINIT?");
			break;
		case ESP32_ESPNOW_SET_DISCOVERABLE:
			snprintf(line, sizeof(line), "AT+ENOWDISCOVERABLE=%u",
					e->discoverable ? 1U : 0U);
			break;
		default:
			line[0] = '\0';
			break;
		}
		if (set == ESP32_ESPNOW_SET_NAME) {
			send_at_cmd(this, "AT+ENOWNAME", e->name, NULL);
		}
		else if (set == ESP32_ESPNOW_SET_GROUP) {
			send_at_cmd(this, "AT+ENOWGROUP", e->group, NULL);
		}
		else if (set == ESP32_ESPNOW_SET_PEER) {
			send_at_cmd(this, "AT+ENOWPEER", e->peer, NULL);
		}
		else {
			send_at_cmd_raw(this, line);
		}
		e->sending = set;
		e->phase = ESPNOW_PHASE_CMD_SENT;
		uv_delay_init(&e->timeout, ESP32_ESPNOW_AT_TIMEOUT_MS);
	}
	else {
	}
	return (set != ESP32_ESPNOW_SET_COUNT);
}


/// @brief: Ends the frame in flight
static void espnow_send_finish(uv_esp32_st *this, bool ok) {
	uv_esp32_espnow_st *e = &this->espnow;
	if (ok) {
		e->diag.tx_ok++;
	}
	else {
		e->diag.tx_fail++;
	}
	e->tx_last_ok = ok;
	e->phase = ESPNOW_PHASE_IDLE;
	e->tx_pending = false;
	e->yield = true;
}


/// @brief: The ESP-NOW state machine, one AT exchange at a time.
///
/// @param link_free: No other exchange is in flight on the AT link. An
/// exchange of ours only starts then, and while ours is in flight nobody
/// else starts one (see rxtx_task), as the answers -- OK, ERROR -- cannot be
/// told apart.
static void espnow_step(uv_esp32_st *this, uint16_t step_ms, bool link_free) {
	uv_esp32_espnow_st *e = &this->espnow;
	// AT+CWJAP and AT+CWLAP hold the module for seconds, and before it is up
	// there is nothing to talk to
	bool module_up = ((this->state == ESP32_STATE_JOINED_NETWORK) ||
			(this->state == ESP32_STATE_LEFT_NETWORK));

	switch (e->phase) {
	case ESPNOW_PHASE_IDLE:
		if (!module_up || !link_free ||
				!uv_delay_has_ended(&this->at_quiet_delay)) {
			// wait
		}
		else if (!e->enabled) {
			if (e->state != ESP32_ESPNOW_STATE_DISABLED) {
				send_at_cmd(this, "AT+ENOWDEINIT", NULL, NULL);
				e->sending = ESP32_ESPNOW_SET_COUNT;
				e->phase = ESPNOW_PHASE_CMD_SENT;
				uv_delay_init(&e->timeout, ESP32_ESPNOW_AT_TIMEOUT_MS);
			}
			else {
			}
		}
		else if (e->state == ESP32_ESPNOW_STATE_ERROR) {
			if (uv_delay(&e->timeout, step_ms)) {
				e->state = ESP32_ESPNOW_STATE_CONFIG;
			}
			else {
			}
		}
		else if (espnow_config_next(this)) {
			if (e->state != ESP32_ESPNOW_STATE_READY) {
				e->state = ESP32_ESPNOW_STATE_CONFIG;
			}
			else {
				// A setting changing while ready (the channel after a join,
				// a new peer) keeps sends going: they queue behind it.
			}
		}
		else if (e->tx_pending) {
			e->state = ESP32_ESPNOW_STATE_READY;
			char line[32];
			snprintf(line, sizeof(line), "AT+ENOWSEND=%u",
					(unsigned int) e->tx_len);
			send_at_cmd_raw(this, line);
			e->phase = ESPNOW_PHASE_SEND_AT_SENT;
			uv_delay_init(&e->timeout, ESP32_ESPNOW_AT_TIMEOUT_MS);
		}
		else if (e->disc_req) {
			e->state = ESP32_ESPNOW_STATE_READY;
			char line[40];
			if (e->disc_channel != 0U) {
				snprintf(line, sizeof(line), "AT+ENOWDISC=%u,%u",
						(unsigned int) e->disc_ms,
						(unsigned int) e->disc_channel);
			}
			else {
				snprintf(line, sizeof(line), "AT+ENOWDISC=%u",
						(unsigned int) e->disc_ms);
			}
			send_at_cmd_raw(this, line);
			e->phase = ESPNOW_PHASE_DISC_SENT;
			uv_delay_init(&e->timeout,
					(uint16_t) (e->disc_ms + ESP32_ESPNOW_AT_TIMEOUT_MS));
		}
		else {
			e->state = ESP32_ESPNOW_STATE_READY;
		}
		break;

	case ESPNOW_PHASE_CMD_SENT: {
		uint8_t match = rx_at_match(this, "OK", "ERROR");
		if (this->at_busy) {
			// dropped unanswered: sent again once the module is done
			if (e->sending != ESP32_ESPNOW_SET_COUNT) {
				e->dirty[e->sending] = true;
			}
			else {
			}
			e->phase = ESPNOW_PHASE_IDLE;
			uv_delay_init(&this->at_quiet_delay, ESP32_AT_QUIET_MS);
		}
		else if (match == 1) {
			e->phase = ESPNOW_PHASE_IDLE;
			if (!e->enabled) {
				// that was AT+ENOWDEINIT: everything is sent again once
				// enabled
				e->state = ESP32_ESPNOW_STATE_DISABLED;
				for (uint8_t i = 0; i < (uint8_t) ESP32_ESPNOW_SET_COUNT; i++) {
					e->dirty[i] = true;
				}
			}
			else {
			}
		}
		else if ((match == 2) || uv_delay(&e->timeout, step_ms)) {
			ESP32_DEBUG(this, "ESP32: ESP-NOW setting %u %s\n",
					(unsigned int) e->sending,
					(match == 2) ? "refused" : "unanswered");
			e->diag.cmd_fails++;
			uv_esp32_diag_err(this, ESP32_ERR_ESPNOW_CMD);
			if (e->sending != ESP32_ESPNOW_SET_COUNT) {
				e->dirty[e->sending] = true;
			}
			else {
			}
			e->phase = ESPNOW_PHASE_IDLE;
			e->state = ESP32_ESPNOW_STATE_ERROR;
			uv_delay_init(&e->timeout, ESP32_ESPNOW_RETRY_MS);
		}
		else {
		}
		break;
	}

	case ESPNOW_PHASE_DISC_SENT: {
		// the devices found arrive before the OK, see espnow_parse_dev
		uint8_t match = rx_at_match(this, "OK", "ERROR");
		if (this->at_busy) {
			// dropped: disc_req stays, so it is sent again
			e->phase = ESPNOW_PHASE_IDLE;
			uv_delay_init(&this->at_quiet_delay, ESP32_AT_QUIET_MS);
		}
		else if ((match != 0) || uv_delay(&e->timeout, step_ms)) {
			if (match != 1) {
				ESP32_DEBUG(this, "ESP32: ESP-NOW discovery failed\n");
			}
			else {
			}
			e->phase = ESPNOW_PHASE_IDLE;
			e->disc_req = false;
		}
		else {
		}
		break;
	}

	case ESPNOW_PHASE_SEND_AT_SENT: {
		bool error = false;
		bool prompt = rx_wait_prompt(this, &error);
		if (this->at_busy) {
			// dropped before any prompt: tx_pending stays, so it goes again
			e->phase = ESPNOW_PHASE_IDLE;
			uv_delay_init(&this->at_quiet_delay, ESP32_AT_QUIET_MS);
		}
		else if (prompt) {
			uv_mutex_lock(&this->txstream_mutex);
			uint32_t pushed = uv_streambuffer_push(&this->tx_streambuffer,
					(char *) e->tx_data, e->tx_len, 100);
			uv_mutex_unlock(&this->txstream_mutex);
			if (pushed != e->tx_len) {
				ESP32_DEBUG(this, "ESP32: ESP-NOW short write %u / %u\n",
						(unsigned int) pushed, (unsigned int) e->tx_len);
			}
			else {
			}
			e->phase = ESPNOW_PHASE_SEND_DATA_SENT;
			uv_delay_init(&e->timeout, ESP32_ESPNOW_SEND_TIMEOUT_MS);
		}
		else if (error || uv_delay(&e->timeout, step_ms)) {
			// refused: no peer on the module's side, or a length it does not
			// take. Sent again with the peer, should it have been lost.
			ESP32_DEBUG(this, "ESP32: ESP-NOW send %s\n",
					error ? "refused" : "prompt timeout");
			e->dirty[ESP32_ESPNOW_SET_PEER] = true;
			espnow_send_finish(this, false);
		}
		else {
		}
		break;
	}

	case ESPNOW_PHASE_SEND_DATA_SENT: {
		uint8_t match = rx_at_match(this, "SEND OK", "SEND FAIL");
		if ((match != 0) || uv_delay(&e->timeout, step_ms)) {
			espnow_send_finish(this, (match == 1));
		}
		else {
		}
		break;
	}

	default:
		e->phase = ESPNOW_PHASE_IDLE;
		break;
	}
}

#endif


static void rxtx_task(void *me_ptr) {
	uv_esp32_st *this = me_ptr;
	uv_ts_st ts;

	uv_ts_init(&ts);
	uv_delay_init(&this->timeout, ESP32_AT_TIMEOUT_MS);

	while (true) {
		if (this->disabled) {
			// Drop whatever session there was, so that nothing above keeps
			// believing in it, and start over from a reset once resumed.
			set_state(this, ESP32_STATE_INIT);
#if CONFIG_ESP32_MQTT
			mqtt_set_state(this, ESP32_MQTT_STATE_DISABLED);
#endif
			ESP32_DEBUG(this, "ESP32: disabled\n");
			while (this->disabled) {
				// Released, not floating: the module must keep running for
				// whoever is talking to it over USB.
				uv_gpio_set(this->reset_io, true);
				uv_streambuffer_clear(&this->rx_datastream);
				uv_rtos_task_delay(100);
			}
			ESP32_DEBUG(this, "ESP32: enabled\n");
			uv_ts_init(&ts);
		}
		else {
		}

		uv_ts_step(&ts);
		(void) uv_delay(&this->at_quiet_delay, uv_ts_get_step_ms(&ts));
		tx(this);
		rx(this, uv_streambuffer_get_len(&this->rx_datastream) ? 0 : 1);

#if CONFIG_ESP32_MQTT
		// Drain async +MQTT events. Skipped during the publish AT-prompt phase
		// because we need to read the bare ">" byte directly from rx_datastream
		// (it is not a line and would otherwise be swallowed into at_resp).
		uv_esp32_mqtt_slot_st *active_slot = mqtt_select_slot(this);
		bool in_prompt = (active_slot != NULL &&
				active_slot->phase == MQTT_PUB_PHASE_AT_SENT);
#else
		bool in_prompt = false;
#endif
#if CONFIG_ESP32_ESPNOW
		// the same for an ESP-NOW frame
		if (this->espnow.phase == ESPNOW_PHASE_SEND_AT_SENT) {
			in_prompt = true;
		}
		else {
		}
		bool espnow_xchg = (this->espnow.phase != ESPNOW_PHASE_IDLE);
#else
		bool espnow_xchg = false;
#endif
		if (!in_prompt) {
			pump_mqtt_async(this);
		}
		else {
		}

		// An AT exchange of the MQTT client is in flight: a publish or a
		// (un)subscribe past its first command, or a connect in progress.
#if CONFIG_ESP32_MQTT
		bool mqtt_xchg = (((active_slot != NULL) &&
				(active_slot->phase != MQTT_PUB_PHASE_IDLE)) ||
				(this->mqtt_sub_active != NULL) ||
				(this->mqtt_state == ESP32_MQTT_STATE_CLEAN) ||
				(this->mqtt_state == ESP32_MQTT_STATE_USERCFG) ||
				(this->mqtt_state == ESP32_MQTT_STATE_CONNCFG) ||
				(this->mqtt_state == ESP32_MQTT_STATE_CONN));
#else
		bool mqtt_xchg = false;
#endif
		// Nobody waits for an answer on the AT link, so a command of anyone's
		// can go.
		bool link_free = (!mqtt_xchg && !espnow_xchg);

		// True when no AT exchange of ours is in flight, so a fire-and-forget
		// query (signal strength, network time) can be injected without
		// stealing the response another state machine is waiting for.
#if CONFIG_ESP32_MQTT
		// No publish or (un)subscribe in flight: the exchanges that a command
		// arriving mid-transaction would actually corrupt.
		bool at_free = ((active_slot == NULL) &&
				(this->mqtt_sub_active == NULL) && !espnow_xchg);
		// ...and the MQTT machine is somewhere it is not waiting for a reply,
		// so an injected query cannot be mistaken for the answer it wants.
		bool at_idle = (at_free &&
				((this->mqtt_state == ESP32_MQTT_STATE_CONNECTED) ||
				(this->mqtt_state == ESP32_MQTT_STATE_DISABLED)));
#else
		bool at_free = !espnow_xchg;
		bool at_idle = at_free;
#endif

		// The AP was lost while joined: leave, and let LEFT_NETWORK look for
		// it. A scan started from the joined state returns there first and
		// is caught on the next round.
		if (this->wifi_lost && (this->state == ESP32_STATE_JOINED_NETWORK)) {
			ESP32_DEBUG(this, "ESP32: WiFi connection lost\n");
			this->wifi_lost = false;
			this->wifi_ssid_seen = false;
			set_state(this, ESP32_STATE_LEFT_NETWORK);
		}
		else {
		}

		// A join or leave asked for from outside, carried out only between
		// exchanges. Before the module is up the request is dropped: the
		// configured network is what it joins once it is.
		if ((this->wifi_req != ESP32_WIFI_REQ_NONE) && link_free &&
				((this->state == ESP32_STATE_JOINED_NETWORK) ||
				(this->state == ESP32_STATE_LEFT_NETWORK))) {
			if (this->wifi_req == ESP32_WIFI_REQ_JOIN) {
				send_at_cmd(this, "AT+CWJAP", this->wifi_ssid, this->wifi_passwd);
				set_state(this, ESP32_STATE_CONNECT_WIFI);
			}
			else {
				send_at_cmd(this, "AT+CWQAP", NULL, NULL);
				set_state(this, ESP32_STATE_LEFT_NETWORK);
			}
			this->wifi_req = ESP32_WIFI_REQ_NONE;
		}
		// A scan asked for from outside, started only between exchanges
		else if (this->scan_req && link_free &&
				((this->state == ESP32_STATE_JOINED_NETWORK) ||
				(this->state == ESP32_STATE_LEFT_NETWORK))) {
			// started before the request is lowered, so a caller waiting for
			// the scan never sees neither
			network_scan_start(this, this->scan_req_store);
			this->scan_req = false;
		}
		else {
		}

		switch (this->state) {

		case ESP32_STATE_INIT:
			ESP32_DEBUG(this, "ESP32: resetting module\n");
#if ESP32_UART_SETUP
			// back to what the module boots with
#if CONFIG_ESP32_UART_FLOW_CONTROL
			// the module leaves its RTS pin undriven until it is told to
			// use flow control, so its CTS input means nothing until then
			uv_uart_set_cts_enabled(this->uart, false);
#endif
			uv_uart_set_baudrate(this->uart, CONFIG_ESP32_UART_BOOT_BAUDRATE);
#endif
			esp32_reset(this);
			this->diag.module_resets++;
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
#if ESP32_UART_SETUP
				char line[48];
				ESP32_DEBUG(this, "ESP32: echo disabled, setting up UART\n");
				// 8 data bits, 1 stop bit, no parity, and RTS + CTS (3) or no
				// flow control (0)
				snprintf(line, sizeof(line), "AT+UART_CUR=%u,8,1,0,%u",
						 (unsigned int) CONFIG_ESP32_UART_BAUDRATE,
						 CONFIG_ESP32_UART_FLOW_CONTROL ? 3U : 0U);
				send_at_cmd(this, line, NULL, NULL);
				set_state(this, ESP32_STATE_SET_UART);
#else
				ESP32_DEBUG(this, "ESP32: echo disabled, setting CWMODE\n");
				send_at_cmd(this, "AT+CWMODE=1", NULL, NULL);
				set_state(this, ESP32_STATE_SET_CWMODE);
#endif
			}
			else if (uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: ATE0 timeout, resetting\n");
				set_state(this, ESP32_STATE_INIT);
			}
			else {
			}
			break;

#if ESP32_UART_SETUP
		case ESP32_STATE_SET_UART:
			if (rx_at_match(this, "OK", NULL)) {
				ESP32_DEBUG(this, "ESP32: UART set up, setting CWMODE\n");
				// The module switches once its OK has left; give it a moment
				// so that the next command does not arrive mid-switch.
				uv_rtos_task_delay(2);
				uv_uart_set_baudrate(this->uart, CONFIG_ESP32_UART_BAUDRATE);
#if CONFIG_ESP32_UART_FLOW_CONTROL
				uv_uart_set_cts_enabled(this->uart, true);
#endif
				send_at_cmd(this, "AT+CWMODE=1", NULL, NULL);
				set_state(this, ESP32_STATE_SET_CWMODE);
			}
			else if (uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: AT+UART_CUR timeout, resetting\n");
				set_state(this, ESP32_STATE_INIT);
			}
			else {
			}
			break;
#endif

		case ESP32_STATE_SET_CWMODE:
			if (rx_at_match(this, "OK", NULL)) {
				// AT+CWLAP lists networks in the order the scan found them,
				// roughly by channel, and only as many as the scan buffer
				// holds fit in the list. Sorted by RSSI the
				// list keeps the strongest ones instead, so a phone hotspot
				// next to the machine is not pushed out by distant APs on
				// lower channels. Mask 2047 keeps every field, the same
				// line format as the default.
				ESP32_DEBUG(this, "ESP32: CWMODE set, sorting scans by RSSI\n");
				send_at_cmd(this, "AT+CWLAPOPT=1,2047", NULL, NULL);
				set_state(this, ESP32_STATE_SET_CWLAPOPT);
			}
			else if (uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: CWMODE timeout, resetting\n");
				set_state(this, ESP32_STATE_INIT);
			}
			else {
			}
			break;

		case ESP32_STATE_SET_CWLAPOPT: {
			// Sorting only changes which networks make it to the list, so
			// an ERROR or no answer leaves the scans unsorted rather than
			// resetting the module.
			uint8_t match = rx_at_match(this, "OK", "ERROR");
			if (match == 1) {
				ESP32_DEBUG(this, "ESP32: CWLAPOPT set, reading MAC\n");
			}
			else if ((match == 2) ||
					uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: CWLAPOPT failed, scans unsorted\n");
			}
			else {
				break;
			}
			send_at_cmd(this, "AT+CIPSTAMAC?", NULL, NULL);
			set_state(this, ESP32_STATE_GET_MAC);
			break;
		}

		case ESP32_STATE_CONNECT_WIFI: {
			uint8_t match = rx_at_match(this, "WIFI GOT IP", "ERROR");
			if (match == 1) {
				uv_uart_clear_rx_buffer(this->uart);
				uv_streambuffer_clear(&this->rx_datastream);
				// a disconnect reported during the join was the old AP
				this->wifi_lost = false;
				set_state(this, ESP32_STATE_JOINED_NETWORK);
				ESP32_DEBUG(this, "ESP32: WiFi connected\n");
			}
			else if (match == 2 ||
					uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: WiFi connection failed\n");
				this->diag.wifi_join_fails++;
				uv_esp32_diag_err(this, ESP32_ERR_WIFI_JOIN);
				set_state(this, ESP32_STATE_LEFT_NETWORK);
			}
			else {
			}
			break;
		}

		case ESP32_STATE_JOINED_NETWORK:
			// Refresh the signal strength now and then. Fire and forget: the
			// answer is recognized by dispatch_mqtt_line no matter which waiter
			// happens to read the line, so this never occupies the AT matcher
			// and cannot steal a response the MQTT state machine is waiting
			// for. It is skipped entirely while a publish or a (un)subscribe is
			// in flight, so the query is never injected into someone else's
			// exchange; a sample missed that way is simply picked up on the
			// next round.
			// Due once the delay has run out, and sent as soon as the link
			// is idle and the last fire-and-forget command has been answered.
			(void) uv_delay(&this->rssi_delay, uv_ts_get_step_ms(&ts));
			if (uv_delay_has_ended(&this->rssi_delay) && at_idle &&
					uv_delay_has_ended(&this->at_quiet_delay)) {
				uv_delay_init(&this->rssi_delay, ESP32_RSSI_POLL_MS);
				// The address is asked for only until it is known: it does
				// not change while joined, and a rejoin clears it.
				if (this->ip_str[0] == '\0') {
					send_at_cmd(this, "AT+CIPSTA?", NULL, NULL);
				}
				else {
					send_at_cmd(this, "AT+CWJAP?", NULL, NULL);
				}
				uv_delay_init(&this->at_quiet_delay, ESP32_AT_QUIET_MS);
			}
			else {
			}
#if CONFIG_ESP32_SNTP
			// Network time, on the same fire-and-forget terms. The module has
			// no clock of its own until SNTP is pointed at a server, and TLS
			// certificate validity is checked against that clock, so this runs
			// as soon as the network is joined and the MQTT client waits for
			// its result (see mqtt_tls_time_ok).
			// Unlike the signal strength above, this does not wait for the
			// MQTT machine to be in a quiet state: a TLS session cannot come up
			// before the clock is set, so making the clock wait on MQTT is a
			// loop with no way out. It still never injects into a publish or a
			// (un)subscribe, which are the exchanges that would actually be
			// corrupted by a command arriving mid-transaction.
			if (at_free && !this->sntp_cfg_sent &&
					uv_delay_has_ended(&this->at_quiet_delay)) {
				char line[128];
				snprintf(line, sizeof(line), "AT+CIPSNTPCFG=1,%i,%s",
						(int) ESP32_SNTP_TIMEZONE, ESP32_SNTP_SERVERS);
				send_at_cmd_raw(this, line);
				uv_delay_init(&this->at_quiet_delay, ESP32_AT_QUIET_MS);
				ESP32_DEBUG(this, "ESP32: SNTP servers %s\n",
						ESP32_SNTP_SERVERS);
				this->sntp_cfg_sent = true;
				uv_delay_init(&this->sntp_cfg_retry, this->sntp_cfg_retry_ms);
				uv_delay_init(&this->sntp_delay, ESP32_SNTP_POLL_MS);
			}
			else if (at_free && !this->time_synced &&
					uv_delay(&this->sntp_cfg_retry,
							uv_ts_get_step_ms(&ts))) {
				// Nothing has come back in this long, so restart the client:
				// re-arm the setter above and let it re-send the whole
				// configuration on the next round. A restart issues a fresh
				// NTP request there and then, which is the point -- whatever
				// the first request met (a lost packet, DNS that was not up
				// yet when the join completed, SNTP never enabled at all) is
				// answered by asking again rather than by waiting.
				this->sntp_cfg_sent = false;
				if (this->sntp_cfg_retry_ms < (ESP32_SNTP_CFG_RETRY_MS / 2)) {
					this->sntp_cfg_retry_ms =
							(uint16_t) (this->sntp_cfg_retry_ms * 2);
				}
				else {
					this->sntp_cfg_retry_ms = ESP32_SNTP_CFG_RETRY_MS;
				}
			}
			else if (uv_delay(&this->sntp_delay, uv_ts_get_step_ms(&ts))) {
				uv_delay_init(&this->sntp_delay, this->time_synced ?
						ESP32_SNTP_REFRESH_MS : ESP32_SNTP_POLL_MS);
				if (at_free && uv_delay_has_ended(&this->at_quiet_delay)) {
					// ONE query per round: sending both together loses at
					// least one of the answers, because each send resets the
					// AT line buffer and the second command lands while the
					// first reply is still coming in over the UART.
					// While the clock is unknown only the epoch is asked for.
					// It is the one the TLS gate keys on, and alternating with
					// the asctime string -- which is for display and can wait
					// -- would leave the clock up to two poll intervals stale
					// exactly when the wait is being measured. Once the clock
					// is set the two alternate, refreshing both slowly.
					if (!this->time_synced || !this->sntp_query_alt) {
						send_at_cmd(this, "AT+SYSTIMESTAMP?", NULL, NULL);
					}
					else {
						send_at_cmd(this, "AT+CIPSNTPTIME?", NULL, NULL);
					}
					uv_delay_init(&this->at_quiet_delay, ESP32_AT_QUIET_MS);
					this->sntp_query_alt = !this->sntp_query_alt;
				}
				else {
				}
			}
			else {
			}
#endif
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
			// what a join asked for before the module was up is this one
			this->wifi_req = ESP32_WIFI_REQ_NONE;
			if (this->wifi_ssid != NULL && this->wifi_ssid[0] != '\0') {
				ESP32_DEBUG(this, "ESP32: joining '%s'\n",
						this->wifi_ssid);
				send_at_cmd(this, "AT+CWJAP",
						this->wifi_ssid, this->wifi_passwd);
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
				if (strstr(this->at_resp, "+CWLAP:") != NULL) {
					char *ssid_start = strchr(this->at_resp, '"');
					if (ssid_start != NULL) {
						ssid_start++;
						char *ssid_end = strchr(ssid_start, '"');
						if (ssid_end != NULL) {
							uint16_t len = ssid_end - ssid_start;
							// the configured network is looked for in every
							// line, also the ones past a full list
							if ((this->wifi_ssid != NULL) &&
									(len > 0U) &&
									(len == strlen(this->wifi_ssid)) &&
									(strncmp(ssid_start, this->wifi_ssid, len) == 0)) {
								this->wifi_ssid_seen = true;
							}
							else {
							}
							if (len >= SSID_STR_MAX_LEN) {
								len = SSID_STR_MAX_LEN - 1;
							}
							else {
							}
							// A nameless network cannot be joined: AT+CWJAP
							// takes an SSID, so a blank row in the list is one
							// nobody can tap. Hidden networks are reported this
							// way -- the name is simply not broadcast -- and so
							// is a scan line that arrived damaged. Neither
							// belongs on screen.
							if ((len == 0) || !this->scan_store ||
									(this->scan_count >= this->scan_buf_len)) {
								/* nothing to show, nobody to show it to, or
								 * no room to show it */
							}
							else {
								uv_esp32_network_st *net =
										&this->scan_buf[this->scan_count];
								memset(net, 0, sizeof(*net));
								memcpy(net->ssid, ssid_start, len);
								net->ssid[len] = '\0';
								char *rssi_str = ssid_end + 2;
								net->rssi = (int8_t) strtol(rssi_str, NULL, 10);
								this->scan_count++;
							}
						}
						else {
						}
					}
					else {
					}
				}
				else if (strstr(this->at_resp, "OK") != NULL) {
					ESP32_DEBUG(this, "ESP32: scan complete, %u networks stored\n",
							this->scan_count);
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
			// With a network configured, scan for it now and then and join
			// only once it is seen. A blind AT+CWJAP to an AP that is not
			// there ties the module up for the whole join timeout, while a
			// scan is over in a few seconds. A scan started from the UI
			// counts as well.
			if ((this->wifi_ssid == NULL) || (this->wifi_ssid[0] == '\0') ||
					!link_free) {
				// nothing to look for, or the AT link is taken: an AT+CWJAP
				// or AT+CWLAP sent now would land in someone's exchange
			}
			else {
				if (this->wifi_ssid_seen) {
					ESP32_DEBUG(this, "ESP32: '%s' in range, joining\n",
							this->wifi_ssid);
					this->wifi_ssid_seen = false;
					send_at_cmd(this, "AT+CWJAP",
							this->wifi_ssid, this->wifi_passwd);
					set_state(this, ESP32_STATE_CONNECT_WIFI);
				}
				else if (uv_delay(&this->timeout, uv_ts_get_step_ms(&ts))) {
#if CONFIG_ESP32_ESPNOW
					// A scan takes the radio off the ESP-NOW channel for a few
					// seconds, so while the peer is talking to us look for the
					// network only every few rounds.
					if ((uv_esp32_espnow_rx_age_ms(this) <
							ESP32_ESPNOW_LINK_ALIVE_MS) &&
							(this->espnow.scan_skips <
									ESP32_ESPNOW_SCAN_SKIPS)) {
						this->espnow.scan_skips++;
						uv_delay_init(&this->timeout, ESP32_RECONNECT_MS);
					}
					else
#endif
					{
#if CONFIG_ESP32_ESPNOW
						this->espnow.scan_skips = 0;
#endif
						ESP32_DEBUG(this, "ESP32: looking for '%s'\n",
								this->wifi_ssid);
						network_scan_start(this, false);
					}
				}
				else {
				}
			}
			break;

		default:
			set_state(this, ESP32_STATE_INIT);
			break;
		}


#if CONFIG_ESP32_ESPNOW
		// Ahead of the MQTT client, so a frame waiting goes before a queued
		// publish: both only start an exchange with the link free, and
		// whichever starts first has it.
#if CONFIG_ESP32_MQTT
		// a publish waiting gets every other turn, see espnow.yield
		bool espnow_turn = !(this->espnow.yield && (active_slot != NULL) &&
				(this->mqtt_state == ESP32_MQTT_STATE_CONNECTED));
#else
		bool espnow_turn = true;
#endif
		espnow_step(this, uv_ts_get_step_ms(&ts), link_free && espnow_turn);
		espnow_xchg = (this->espnow.phase != ESPNOW_PHASE_IDLE);
#endif


#if CONFIG_ESP32_MQTT
		// MQTT state machine. Runs alongside the main esp32 state machine.
		// Progresses only while Wi-Fi is joined; drops to DISABLED otherwise so
		// it cleanly re-enters INIT on rejoin.
		bool mqtt_no_url = ((this->mqtt_broker_url == NULL) ||
				(this->mqtt_broker_url[0] == '\0'));
		if (espnow_xchg && (this->state == ESP32_STATE_JOINED_NETWORK)) {
			// An ESP-NOW exchange has the AT link. Nothing of the client's is
			// in flight -- it only starts with the link free -- so it simply
			// waits; its timeouts are not run meanwhile.
		}
		else if ((this->state == ESP32_STATE_JOINED_NETWORK) &&
				mqtt_no_url &&
				(((this->mqtt_state == ESP32_MQTT_STATE_CONNECTED) &&
				((active_slot == NULL) ||
						(active_slot->phase == MQTT_PUB_PHASE_IDLE)) &&
				(this->mqtt_sub_active == NULL)) ||
				(this->mqtt_state == ESP32_MQTT_STATE_INIT) ||
				(this->mqtt_state == ESP32_MQTT_STATE_ERROR))) {
			// The broker address was withdrawn (uv_esp32_mqtt_init() with an
			// empty url): close the session and stay closed until one is given
			// again. A session is only closed between AT exchanges, and one
			// still being opened is let finish first, so no reply that a state
			// below waits for is taken by the close.
			if (this->mqtt_state == ESP32_MQTT_STATE_CONNECTED) {
				char line[32];
				snprintf(line, sizeof(line), "AT+MQTTCLEAN=%d",
						ESP32_MQTT_LINK_ID);
				send_at_cmd_raw(this, line);
			}
			else {
			}
			ESP32_DEBUG(this, "ESP32: MQTT broker url withdrawn\n");
			mqtt_slot_clear_all(this);
			mqtt_set_state(this, ESP32_MQTT_STATE_DISABLED);
		}
		else if (this->state == ESP32_STATE_JOINED_NETWORK) {
			switch (this->mqtt_state) {
			case ESP32_MQTT_STATE_DISABLED:
				// after the join's own answer, see at_quiet_delay
				if (this->mqtt_broker_url != NULL &&
						this->mqtt_broker_url[0] != '\0' &&
						mqtt_tls_time_ok(this) &&
						uv_delay_has_ended(&this->at_quiet_delay)) {
					mqtt_set_state(this, ESP32_MQTT_STATE_INIT);
				}
				else {
				}
				break;
			case ESP32_MQTT_STATE_INIT: {
				// Close whatever link the module still holds before
				// configuring a new one. AT+MQTTCONN is sent with
				// reconnect=1, so a link given up on here -- a CONN that
				// timed out, a session dropped with the Wi-Fi -- lives on
				// in ESP-AT, which keeps reconnecting it on its own. Its
				// AT+MQTTUSERCFG then answers ERROR, and a +MQTTCONNECTED
				// arriving outside CONN is ignored, so every retry failed
				// the same way until the module was reset.
				char line[32];
				snprintf(line, sizeof(line), "AT+MQTTCLEAN=%d",
						ESP32_MQTT_LINK_ID);
				send_at_cmd_raw(this, line);
				mqtt_set_state(this, ESP32_MQTT_STATE_CLEAN);
				break;
			}
			case ESP32_MQTT_STATE_CLEAN: {
				// ERROR is the usual answer: there was no link to close.
				// Either way, or with no answer at all, go on configuring.
				uint8_t match = rx_at_match(this, "OK", "ERROR");
				if ((match == 0) &&
						!uv_delay(&this->mqtt_timeout,
								uv_ts_get_step_ms(&ts))) {
					break;
				}
				else {
				}
				char line[256];
				snprintf(line, sizeof(line),
						"AT+MQTTUSERCFG=%d,%u,\"%s\",\"%s\",\"%s\",%u,%u,\"\"",
						ESP32_MQTT_LINK_ID,
						(unsigned int) this->mqtt_scheme,
						this->mqtt_client_id != NULL ?
								this->mqtt_client_id : "",
						this->mqtt_user != NULL ?
								this->mqtt_user : "",
						this->mqtt_passwd != NULL ?
								this->mqtt_passwd : "",
						(unsigned int) this->mqtt_cert_key_id,
						(unsigned int) this->mqtt_ca_id);
				ESP32_DEBUG(this, "ESP32: MQTT USERCFG -> '%s'\n",
						this->mqtt_broker_url);
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
							(unsigned int) this->mqtt_keepalive_s);
					send_at_cmd_raw(this, line);
					mqtt_set_state(this, ESP32_MQTT_STATE_CONNCFG);
				}
				else if (match == 2 ||
						uv_delay(&this->mqtt_timeout,
								uv_ts_get_step_ms(&ts))) {
					ESP32_DEBUG(this, "ESP32: MQTT USERCFG failed\n");
					this->diag.mqtt_conn_fails++;
					uv_esp32_diag_err(this, ESP32_ERR_MQTT_USERCFG);
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
							this->mqtt_broker_url,
							(unsigned int) this->mqtt_broker_port);
					send_at_cmd_raw(this, line);
					mqtt_set_state(this, ESP32_MQTT_STATE_CONN);
				}
				else if (match == 2 ||
						uv_delay(&this->mqtt_timeout,
								uv_ts_get_step_ms(&ts))) {
					ESP32_DEBUG(this, "ESP32: MQTT CONNCFG failed\n");
					this->diag.mqtt_conn_fails++;
					uv_esp32_diag_err(this, ESP32_ERR_MQTT_CONNCFG);
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
					this->diag.mqtt_conn_fails++;
					uv_esp32_diag_err(this, ESP32_ERR_MQTT_CONN);
					mqtt_set_state(this, ESP32_MQTT_STATE_ERROR);
				}
				else {
				}
				break;
			}
			case ESP32_MQTT_STATE_CONNECTED:
				if (this->mqtt_sub_phase == MQTT_SUB_PHASE_AT_SENT) {
					// An in-flight (un)subscribe owns the AT link until it
					// resolves; publishes wait their turn.
					uint8_t match = rx_at_match(this, "OK", "ERROR");
					if (match != 0) {
						mqtt_sub_finish(this, (match == 1));
					}
					else if (uv_delay(&this->mqtt_sub_timeout,
							uv_ts_get_step_ms(&ts))) {
						ESP32_DEBUG(this, "ESP32: MQTT SUB timeout\n");
						mqtt_sub_finish(this, false);
					}
					else {
					}
				}
				else if ((active_slot != NULL) &&
						(active_slot->phase == MQTT_PUB_PHASE_IDLE) &&
						!uv_delay_has_ended(&this->at_quiet_delay)) {
					// A fire-and-forget query was just sent, and the module
					// drops ("busy p...") a command arriving before it has
					// answered: the publish would then wait out its whole
					// prompt timeout. It goes once the answer is in.
				}
				else if (active_slot != NULL) {
					if (active_slot->phase == MQTT_PUB_PHASE_IDLE) {
						char line[200];
						// The length is read and the slot taken out of
						// coalescing in one go: uv_esp32_mqtt_publish()
						// overwrites an idle slot in place, and a payload
						// replaced after its length went out is pushed with
						// a length the module does not expect -- after which
						// it takes the following commands as payload.
						uv_mutex_lock(&this->mqtt_pub_mutex);
						snprintf(line, sizeof(line),
								"AT+MQTTPUBRAW=%d,\"%s\",%u,%u,%u",
								ESP32_MQTT_LINK_ID,
								active_slot->topic,
								(unsigned int) active_slot->datalen,
								(unsigned int) active_slot->qos,
								(unsigned int)
										(active_slot->retain ? 1 : 0));
						active_slot->phase = MQTT_PUB_PHASE_AT_SENT;
						uv_mutex_unlock(&this->mqtt_pub_mutex);
#if CONFIG_ESP32_ESPNOW
						// the turn is taken, ESP-NOW goes first again
						this->espnow.yield = false;
#endif
						send_at_cmd_raw(this, line);
						uv_delay_init(&this->mqtt_timeout,
								ESP32_MQTT_AT_TIMEOUT_MS);
					}
					else if (active_slot->phase ==
							MQTT_PUB_PHASE_AT_SENT) {
						if (rx_wait_prompt(this, NULL)) {
							uv_mutex_lock(&this->txstream_mutex);
							uint32_t pushed = uv_streambuffer_push(
									&this->tx_streambuffer,
									(char *) active_slot->data,
									active_slot->datalen,
									100);
							uv_mutex_unlock(&this->txstream_mutex);
							if (pushed != active_slot->datalen) {
								ESP32_DEBUG(this,
										"ESP32: MQTT publish short write "
										"%u / %u\n",
										(unsigned int) pushed,
										(unsigned int) active_slot->datalen);
							}
							else {
							}
							uv_delay_init(&this->mqtt_timeout,
									ESP32_MQTT_AT_TIMEOUT_MS);
							active_slot->phase = MQTT_PUB_PHASE_DATA_SENT;
						}
						else if (uv_delay(&this->mqtt_timeout,
								uv_ts_get_step_ms(&ts))) {
							ESP32_DEBUG(this,
									"ESP32: MQTT publish prompt timeout\n");
							this->diag.mqtt_pub_fails++;
							uv_esp32_diag_err(this, ESP32_ERR_MQTT_PUB_TIMEOUT);
							mqtt_slot_release(this, active_slot);
						}
						else {
						}
					}
					else if (active_slot->phase ==
							MQTT_PUB_PHASE_DATA_SENT) {
						uint8_t match = rx_at_match(this, "OK", "ERROR");
						if (match != 0 ||
								uv_delay(&this->mqtt_timeout,
										uv_ts_get_step_ms(&ts))) {
							if (match == 1) {
								this->diag.mqtt_pub_ok++;
							}
							else if (match == 2) {
								ESP32_DEBUG(this,
										"ESP32: MQTT publish ERROR\n");
								this->diag.mqtt_pub_fails++;
								uv_esp32_diag_err(this, ESP32_ERR_MQTT_PUB);
							}
							else {
								this->diag.mqtt_pub_fails++;
								uv_esp32_diag_err(this,
										ESP32_ERR_MQTT_PUB_TIMEOUT);
							}
							mqtt_slot_release(this, active_slot);
						}
						else {
						}
					}
					else {
					}
				}
				else {
					// AT link is free: retire the subscribe backoff, then issue
					// whatever the registry still owes the broker.
					if (this->mqtt_sub_phase == MQTT_SUB_PHASE_BACKOFF) {
						if (uv_delay(&this->mqtt_sub_timeout,
								uv_ts_get_step_ms(&ts))) {
							this->mqtt_sub_phase = MQTT_SUB_PHASE_IDLE;
						}
						else {
						}
					}
					else {
					}
					// see at_quiet_delay above
					if ((this->mqtt_sub_phase == MQTT_SUB_PHASE_IDLE) &&
							uv_delay_has_ended(&this->at_quiet_delay)) {
						mqtt_sub_start_next(this);
					}
					else {
					}
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
				mqtt_slot_clear_all(this);
			}
			else {
			}
		}
#endif


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


#if CONFIG_ESP32_MQTT
static void mqtt_slots_bind(uv_esp32_st *this);
#endif


uv_errors_e uv_esp32_init(uv_esp32_st *this,
		uv_gpios_e reset_io,
		uv_uarts_e uart,
		uint16_t *wifi_flags,
		char *wifi_ssid,
		char *wifi_passwd) {
	this->wifi_flags = wifi_flags;
	this->wifi_ssid = wifi_ssid;
	this->wifi_passwd = wifi_passwd;
#if CONFIG_ESP32_MQTT
	this->mqtt_broker_url = NULL;
	this->mqtt_client_id = NULL;
	this->mqtt_user = NULL;
	this->mqtt_passwd = NULL;
	this->mqtt_broker_port = 0;
	this->mqtt_scheme = 0;
	this->mqtt_ca_id = 0;
	this->mqtt_cert_key_id = 0;
	this->mqtt_keepalive_s = 0;
#endif
	this->uart = uart;
	this->reset_io = reset_io;
	this->state = ESP32_STATE_INIT;
	this->wifi_lost = false;
	this->wifi_ssid_seen = false;
	this->mac = 0;
	this->rssi = 0;
	this->ip_str[0] = '\0';
	uv_delay_init(&this->rssi_delay, ESP32_RSSI_POLL_MS);
#if CONFIG_ESP32_SNTP
	this->time_str[0] = '\0';
	this->time_epoch = 0;
	this->time_year = 0;
	this->time_synced = false;
	this->sntp_cfg_sent = false;
	this->sntp_query_alt = false;
	uv_delay_init(&this->sntp_cfg_retry, ESP32_SNTP_CFG_RETRY_MS);
	uv_delay_init(&this->sntp_delay, ESP32_SNTP_POLL_MS);
#endif
	this->written_byte_count = 0;
	this->transmitted_byte_count = 0;
	this->disabled = false;
	memset(&this->diag, 0, sizeof(this->diag));

#if CONFIG_ESP32_MQTT
	this->mqtt_state = ESP32_MQTT_STATE_DISABLED;
	this->mqtt_retry_backoff_s = 0;
	this->mqtt_rx_callb = NULL;
	memset(&this->mqtt_subrecv, 0, sizeof(this->mqtt_subrecv));
	memset(this->mqtt_pub_slots, 0, sizeof(this->mqtt_pub_slots));
	// every slot needs its backing buffer before anything publishes. Bound
	// after the memset above, which would otherwise zero every capacity and
	// have each publish refused.
	mqtt_slots_bind(this);
	this->mqtt_publish_seq = 0;
	uv_mutex_init(&this->mqtt_pub_mutex);
	uv_mutex_unlock(&this->mqtt_pub_mutex);
	memset(this->mqtt_subs, 0, sizeof(this->mqtt_subs));
	this->mqtt_sub_active = NULL;
	this->mqtt_sub_phase = MQTT_SUB_PHASE_IDLE;
	uv_mutex_init(&this->mqtt_sub_mutex);
	uv_mutex_unlock(&this->mqtt_sub_mutex);
#endif
	this->at_resp_has_pending = false;
	this->scan_req = false;
	this->scan_req_store = false;
	this->wifi_req = ESP32_WIFI_REQ_NONE;
	this->at_busy = false;
	uv_delay_end(&this->at_quiet_delay);
#if CONFIG_ESP32_ESPNOW
	memset(&this->espnow, 0, sizeof(this->espnow));
	this->espnow.state = ESP32_ESPNOW_STATE_DISABLED;
	this->espnow.phase = ESPNOW_PHASE_IDLE;
	this->espnow.sending = ESP32_ESPNOW_SET_COUNT;
	for (uint8_t i = 0; i < (uint8_t) ESP32_ESPNOW_SET_COUNT; i++) {
		this->espnow.dirty[i] = true;
	}
#endif

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











#if CONFIG_ESP32_MQTT

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


void uv_esp32_mqtt_print_slots(uv_esp32_st *this) {
	// Snapshot under the lock and print outside it: printf takes a lock of its
	// own and can block for as long as the terminal needs, which is not
	// something to hold the publish pool for.
	struct {
		bool in_use;
		uint8_t phase;
		uint8_t priority;
		uint16_t stream_id;
		uint16_t capacity;
		uint16_t datalen;
		uint32_t seq;
	} snap[ESP32_MQTT_PUBLISH_SLOT_COUNT];
	uv_mutex_lock(&this->mqtt_pub_mutex);
	for (uint8_t i = 0; i < ESP32_MQTT_PUBLISH_SLOT_COUNT; i++) {
		uv_esp32_mqtt_slot_st *s = &this->mqtt_pub_slots[i];
		snap[i].in_use = s->in_use;
		snap[i].phase = s->phase;
		snap[i].priority = (uint8_t) s->priority;
		snap[i].stream_id = s->stream_id;
		snap[i].capacity = s->capacity;
		snap[i].datalen = s->datalen;
		snap[i].seq = s->seq;
	}
	uv_mutex_unlock(&this->mqtt_pub_mutex);

	printf("    publish slots:\n");
	for (uint8_t i = 0; i < ESP32_MQTT_PUBLISH_SLOT_COUNT; i++) {
		if (snap[i].in_use) {
			const char *phase = "idle";
			if (snap[i].phase == MQTT_PUB_PHASE_AT_SENT) {
				phase = "at-sent";
			}
			else if (snap[i].phase == MQTT_PUB_PHASE_DATA_SENT) {
				phase = "data-sent";
			}
			else {
			}
			printf("        %u: %u/%u B, stream %u, prio %u, seq %u, %s\n",
					(unsigned int) i,
					(unsigned int) snap[i].datalen,
					(unsigned int) snap[i].capacity,
					(unsigned int) snap[i].stream_id,
					(unsigned int) snap[i].priority,
					(unsigned int) snap[i].seq,
					phase);
		}
		else {
			printf("        %u: free, %u B\n",
					(unsigned int) i, (unsigned int) snap[i].capacity);
		}
	}

	// The subscribe machine shares the AT link and owns it outright while a
	// subscribe is in flight, so a pool that is not draining is as likely to be
	// waiting on this as on the publish itself.
	const char *sub_phase = "idle";
	if (this->mqtt_sub_phase == MQTT_SUB_PHASE_AT_SENT) {
		sub_phase = "at-sent";
	}
	else if (this->mqtt_sub_phase == MQTT_SUB_PHASE_BACKOFF) {
		sub_phase = "backoff";
	}
	else {
	}
	printf("    subscribe: %s\n", sub_phase);
	for (uint8_t i = 0; i < ESP32_MQTT_SUBSCRIPTION_COUNT; i++) {
		uv_esp32_mqtt_sub_st *s = &this->mqtt_subs[i];
		if (s->in_use) {
			printf("        %u: %s qos %u, %s%s\n",
					(unsigned int) i, s->topic, (unsigned int) s->qos,
					s->sent ? "acked" : "not acked",
					(s == this->mqtt_sub_active) ? ", in flight" : "");
		}
		else {
		}
	}
}


/// @brief: Backing buffers for the publish slots. Slot 0 is the large one, the
/// rest are small: one producer needs to hand over a big payload whole and the
/// others send little and often, so giving them all the big buffer would spend
/// most of the RAM on capacity nothing uses.
static uint8_t mqtt_pub_buf_large[ESP32_MQTT_PAYLOAD_MAX_LEN];
#if ESP32_MQTT_PUBLISH_SLOT_COUNT > 1
static uint8_t mqtt_pub_buf_small[ESP32_MQTT_PUBLISH_SLOT_COUNT - 1]
							  [ESP32_MQTT_PAYLOAD_SMALL_LEN];
#endif


/// @brief: Points every slot at its backing buffer. Called before the pool is
/// used for the first time.
static void mqtt_slots_bind(uv_esp32_st *this) {
	for (uint8_t i = 0; i < ESP32_MQTT_PUBLISH_SLOT_COUNT; i++) {
		if (i == 0) {
			this->mqtt_pub_slots[i].data = mqtt_pub_buf_large;
			this->mqtt_pub_slots[i].capacity = ESP32_MQTT_PAYLOAD_MAX_LEN;
		}
#if ESP32_MQTT_PUBLISH_SLOT_COUNT > 1
		else {
			this->mqtt_pub_slots[i].data = mqtt_pub_buf_small[i - 1];
			this->mqtt_pub_slots[i].capacity = ESP32_MQTT_PAYLOAD_SMALL_LEN;
		}
#else
		else {
		}
#endif
	}
}


static void mqtt_slot_fill(uv_esp32_mqtt_slot_st *slot,
		const char *topic, const uint8_t *data, uint16_t datalen,
		uint8_t qos, bool retain,
		uv_esp32_mqtt_prio_e priority, uint16_t stream_id) {
	strncpy(slot->topic, topic, ESP32_MQTT_TOPIC_MAX_LEN - 1);
	slot->topic[ESP32_MQTT_TOPIC_MAX_LEN - 1] = '\0';
	memcpy(slot->data, data, datalen);
	slot->datalen = datalen;
	slot->qos = qos;
	slot->retain = retain;
	slot->priority = priority;
	slot->stream_id = stream_id;
}


bool uv_esp32_mqtt_publish_pending(uv_esp32_st *this, uint16_t stream_id) {
	bool ret = false;
	if (stream_id != 0) {
		uv_mutex_lock(&this->mqtt_pub_mutex);
		for (uint8_t i = 0; (i < ESP32_MQTT_PUBLISH_SLOT_COUNT) && !ret; i++) {
			if (this->mqtt_pub_slots[i].in_use &&
					(this->mqtt_pub_slots[i].stream_id == stream_id)) {
				ret = true;
			}
			else {
			}
		}
		uv_mutex_unlock(&this->mqtt_pub_mutex);
	}
	else {
	}
	return ret;
}


uv_errors_e uv_esp32_mqtt_publish(uv_esp32_st *this,
		const char *topic, const uint8_t *data, uint16_t datalen,
		uint8_t qos, bool retain,
		uv_esp32_mqtt_prio_e priority,
		uint16_t stream_id) {
	uv_errors_e ret = ERR_NONE;
	// the largest slot bounds what can be published at all
	if (datalen > ESP32_MQTT_PAYLOAD_MAX_LEN) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else if (strlen(topic) >= ESP32_MQTT_TOPIC_MAX_LEN) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {
		uv_mutex_lock(&this->mqtt_pub_mutex);

		// Coalesce: if a queued slot already carries this stream_id,
		// overwrite its payload in place. Skip slots that are mid-AT-flight
		// (phase != IDLE) — those are being read by the drainer; allocate
		// fresh in that case so the in-flight one finishes uninterrupted.
		uv_esp32_mqtt_slot_st *target = NULL;
		if (stream_id != 0) {
			for (uint8_t i = 0; i < ESP32_MQTT_PUBLISH_SLOT_COUNT; i++) {
				uv_esp32_mqtt_slot_st *s = &this->mqtt_pub_slots[i];
				if (s->in_use && s->stream_id == stream_id &&
						(s->phase == MQTT_PUB_PHASE_IDLE) &&
						(datalen <= s->capacity)) {
					target = s;
					break;
				}
				else {
				}
			}
		}
		else {
		}

		if (target != NULL) {
			// Overwrite — keep existing seq so FIFO position doesn't drift.
			mqtt_slot_fill(target, topic, data, datalen, qos, retain,
					priority, stream_id);
		}
		else {
			// Allocate a fresh slot: the smallest free one that can take this
			// payload, so a small message does not sit in the large buffer and
			// leave a big one with nowhere to go.
			uint8_t free_count = 0;
			for (uint8_t i = 0; i < ESP32_MQTT_PUBLISH_SLOT_COUNT; i++) {
				uv_esp32_mqtt_slot_st *s = &this->mqtt_pub_slots[i];
				if (!s->in_use) {
					free_count++;
					if ((datalen <= s->capacity) &&
							((target == NULL) ||
									(s->capacity < target->capacity))) {
						target = s;
					}
					else {
					}
				}
				else {
				}
			}
			// The last free slot is not for the low priority to take.
			// Priorities decide the order the drainer serves the pool in, but
			// that is worth nothing if the pool is already full by the time a
			// higher-priority message is written: the periodic process data
			// streams publish every step and would hold every slot, so an
			// announce or a command -- sent once, seconds apart -- would find
			// the pool full every single time and be refused for good. A
			// stream that already holds a slot is unaffected: it coalesces
			// into its own slot above rather than coming through here.
			if ((target != NULL) && (free_count <= 1) &&
					(priority >= UV_ESP32_MQTT_PRIO_LOW)) {
				target = NULL;
			}
			else {
			}
			if (target == NULL) {
				ret = ERR_BUFFER_OVERFLOW;
			}
			else {
				mqtt_slot_fill(target, topic, data, datalen, qos, retain,
						priority, stream_id);
				target->seq = ++this->mqtt_publish_seq;
				target->phase = MQTT_PUB_PHASE_IDLE;
				target->in_use = true;
			}
		}

		uv_mutex_unlock(&this->mqtt_pub_mutex);
	}
	return ret;
}


/// @brief: Finds the registry entry for *topic*, or NULL. Caller holds
/// mqtt_sub_mutex.
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


uv_errors_e uv_esp32_mqtt_subscribe(uv_esp32_st *this,
		const char *topic, uint8_t qos) {
	uv_errors_e ret = ERR_NONE;
	if (strlen(topic) >= ESP32_MQTT_TOPIC_MAX_LEN) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {
		uv_mutex_lock(&this->mqtt_sub_mutex);
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
			// re-issued on the next drain; harmless if it was already live
			target->sent = false;
			target->in_use = true;
		}
		uv_mutex_unlock(&this->mqtt_sub_mutex);
	}
	return ret;
}


uv_errors_e uv_esp32_mqtt_unsubscribe(uv_esp32_st *this, const char *topic) {
	uv_mutex_lock(&this->mqtt_sub_mutex);
	uv_esp32_mqtt_sub_st *target = mqtt_sub_find(this, topic);
	if (target != NULL) {
		if (target->sent ||
				(target == this->mqtt_sub_active)) {
			// live on the broker (or being issued right now): let the drainer
			// send AT+MQTTUNSUB and free the entry when it resolves
			target->unsub = true;
		}
		else {
			// never issued — nothing to tell the broker about
			target->in_use = false;
		}
	}
	else {
	}
	uv_mutex_unlock(&this->mqtt_sub_mutex);
	return ERR_NONE;
}


void uv_esp32_mqtt_set_rx_callb(uv_esp32_st *this, uv_esp32_mqtt_rx_callb_t cb) {
	this->mqtt_rx_callb = cb;
}

#endif


#if CONFIG_ESP32_ESPNOW

void uv_esp32_espnow_set_enabled(uv_esp32_st *this, bool value) {
	this->espnow.enabled = value;
}


void uv_esp32_espnow_set_channel(uv_esp32_st *this, uint8_t channel) {
	uint8_t ch = (channel > ESP32_ESPNOW_CHANNEL_MAX) ? 0U : channel;
	if (ch != this->espnow.channel) {
		this->espnow.channel = ch;
		this->espnow.dirty[ESP32_ESPNOW_SET_CHANNEL] = true;
		this->espnow.dirty[ESP32_ESPNOW_SET_QUERY] = true;
	}
	else {
	}
}


/// @brief: Copies *src* into a setting of *size* bytes, raising its flag if
/// the value changed. NULL is taken as an empty string.
static void espnow_set_str(uv_esp32_st *this, char *dest, size_t size,
		const char *src, uv_esp32_espnow_set_e set) {
	const char *str = (src != NULL) ? src : "";
	if (strncmp(dest, str, size - 1U) != 0) {
		strncpy(dest, str, size - 1U);
		dest[size - 1U] = '\0';
		this->espnow.dirty[set] = true;
	}
	else {
	}
}


void uv_esp32_espnow_set_name(uv_esp32_st *this, const char *name) {
	espnow_set_str(this, this->espnow.name, sizeof(this->espnow.name),
			name, ESP32_ESPNOW_SET_NAME);
}


void uv_esp32_espnow_set_group(uv_esp32_st *this, const char *group) {
	espnow_set_str(this, this->espnow.group, sizeof(this->espnow.group),
			group, ESP32_ESPNOW_SET_GROUP);
}


void uv_esp32_espnow_set_discoverable(uv_esp32_st *this, bool value) {
	if (value != this->espnow.discoverable) {
		this->espnow.discoverable = value;
		this->espnow.dirty[ESP32_ESPNOW_SET_DISCOVERABLE] = true;
	}
	else {
	}
}


void uv_esp32_espnow_set_peer(uv_esp32_st *this, const char *mac) {
	espnow_set_str(this, this->espnow.peer, sizeof(this->espnow.peer),
			mac, ESP32_ESPNOW_SET_PEER);
}


uv_errors_e uv_esp32_espnow_discover(uv_esp32_st *this,
		uint16_t ms, uint8_t channel) {
	uv_errors_e ret = ERR_NONE;
	uv_esp32_espnow_st *e = &this->espnow;
	if (!e->enabled ||
			((e->state != ESP32_ESPNOW_STATE_READY) &&
			(e->state != ESP32_ESPNOW_STATE_CONFIG))) {
		ret = ERR_NOT_READY;
	}
	else if (e->disc_req) {
		ret = ERR_HW_BUSY;
	}
	else {
		// the firmware's limits, rather than an ERROR for the caller to puzzle
		// over
		e->disc_ms = (ms < 10U) ? 10U : ((ms > 5000U) ? 5000U : ms);
		e->disc_channel = (channel > ESP32_ESPNOW_CHANNEL_MAX) ? 0U : channel;
		e->disc_req = true;
	}
	return ret;
}


uv_errors_e uv_esp32_espnow_send(uv_esp32_st *this,
		const uint8_t *data, uint16_t len) {
	uv_errors_e ret = ERR_NONE;
	uv_esp32_espnow_st *e = &this->espnow;
	if ((len == 0U) || (len > ESP32_ESPNOW_PAYLOAD_MAX_LEN)) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else if (!e->enabled || (e->peer[0] == '\0') ||
			((e->state != ESP32_ESPNOW_STATE_READY) &&
			(e->state != ESP32_ESPNOW_STATE_CONFIG))) {
		ret = ERR_NOT_READY;
	}
	else if (e->tx_pending) {
		ret = ERR_HW_BUSY;
	}
	else {
		memcpy(e->tx_data, data, len);
		e->tx_len = len;
		// last: the driver task takes the frame once this is up
		e->tx_pending = true;
	}
	return ret;
}


void uv_esp32_espnow_set_rx_callb(uv_esp32_st *this,
		uv_esp32_espnow_rx_callb_t cb) {
	this->espnow.rx_callb = cb;
}


void uv_esp32_espnow_set_dev_callb(uv_esp32_st *this,
		uv_esp32_espnow_dev_callb_t cb) {
	this->espnow.dev_callb = cb;
}


uint32_t uv_esp32_espnow_rx_age_ms(uv_esp32_st *this) {
	uint32_t ret = UINT32_MAX;
	if (this->espnow.rx_seen) {
		ret = (uv_rtos_get_tick_count() - this->espnow.rx_tick) *
				UV_RTOS_TICK_PERIOD_MS;
	}
	else {
	}
	return ret;
}

#endif


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


/// @brief: Starts a network scan. Driver task only, with no AT exchange in
/// flight, in JOINED_NETWORK or LEFT_NETWORK.
///
/// @param store: The scan stores the networks it sees in the scan buffer.
/// False for the driver's own scans for the configured network, which must not
/// overwrite a list started from outside and possibly on screen.
static void network_scan_start(uv_esp32_st *this, bool store) {
	this->scan_store = store;
	if (store) {
		this->scan_count = 0;
	}
	else {
	}
	this->wifi_ssid_seen = false;
	this->scan_return_state = this->state;
	send_at_cmd(this, "AT+CWLAP", NULL, NULL);
	ESP32_DEBUG(this, "ESP32: starting network scan\n");
	set_state(this, ESP32_STATE_SCAN_NETWORKS);
}


// Longest a blocking uv_esp32_network_scan() waits: for an exchange in flight
// to end -- a broker connect is the longest -- and then for the scan.
#define ESP32_SCAN_WAIT_MS		(ESP32_MQTT_CONN_TIMEOUT_MS + \
								 ESP32_SCAN_TIMEOUT_MS + ESP32_AT_TIMEOUT_MS)


uv_errors_e uv_esp32_network_scan(uv_esp32_st *this, bool blocking) {
	uv_errors_e ret = ERR_NOT_READY;
	if ((this->state == ESP32_STATE_JOINED_NETWORK) ||
			(this->state == ESP32_STATE_LEFT_NETWORK)) {
		// Only asked for: the driver task starts it between exchanges, as an
		// AT+CWLAP sent from here could land in the middle of a publish or an
		// ESP-NOW send.
		this->scan_req_store = true;
		this->scan_req = true;
		if (blocking) {
			uint32_t waited_ms = 0;
			while ((this->scan_req ||
					(this->state == ESP32_STATE_SCAN_NETWORKS)) &&
					(waited_ms < ESP32_SCAN_WAIT_MS)) {
				uv_rtos_task_delay(10);
				waited_ms += 10U;
			}
		}
		else {
		}
		ret = ERR_NONE;
	}
	else {
	}
	return ret;
}


void uv_esp32_reset(uv_esp32_st *this) {
	this->state = ESP32_STATE_INIT;
}


void uv_esp32_set_disabled(uv_esp32_st *this, bool value) {
	this->disabled = value;
}


void uv_esp32_network_leave(uv_esp32_st *this) {
	if (this->wifi_ssid != NULL) {
		this->wifi_ssid[0] = '\0';
	}
	if (this->wifi_passwd != NULL) {
		this->wifi_passwd[0] = '\0';
	}
	// see wifi_req
	this->wifi_req = ESP32_WIFI_REQ_LEAVE;
}


void uv_esp32_network_join(uv_esp32_st *this, char ssid[32],
						   char passwd[64]) {
	if (this->wifi_ssid != NULL) {
		strncpy(this->wifi_ssid, ssid, SSID_STR_MAX_LEN - 1);
		this->wifi_ssid[SSID_STR_MAX_LEN - 1] = '\0';
	}
	if (this->wifi_passwd != NULL) {
		strncpy(this->wifi_passwd, passwd, PASSWD_STR_MAX_LEN - 1);
		this->wifi_passwd[PASSWD_STR_MAX_LEN - 1] = '\0';
	}
	// see wifi_req
	this->wifi_req = ESP32_WIFI_REQ_JOIN;
}


void uv_esp32_terminal(uv_esp32_st *this,
		unsigned int args, argument_st *argv) {

	if (args && argv[0].type == ARG_STRING) {
		if (strcmp(argv[0].str, "debug") == 0) {
			if (args > 1 && this->wifi_flags != NULL) {
				if (argv[1].number) {
					*(this->wifi_flags) |= ESP32_CONF_FLAGS_DEBUG;
				}
				else {
					*(this->wifi_flags) &= ~ESP32_CONF_FLAGS_DEBUG;
				}
			}
		}
		else if (strcmp(argv[0].str, "echo") == 0) {
			if (args > 1 && this->wifi_flags != NULL) {
				if (argv[1].number) {
					*(this->wifi_flags) |= ESP32_CONF_FLAGS_ECHO;
				}
				else {
					*(this->wifi_flags) &= ~ESP32_CONF_FLAGS_ECHO;
				}
			}
		}
		else if (strcmp(argv[0].str, "comm") == 0) {
			// Lets the module be reflashed over its own USB port while the
			// host runs: otherwise the "ready" timeout resets it every few
			// seconds and the flasher loses its port. Not stored on purpose,
			// so a forgotten "off" is undone by the next reboot.
			if (args > 1) {
				uv_esp32_set_disabled(this, !argv[1].number);
			}
			else {
			}
			printf("ESP32 communication: %s\n",
					this->disabled ? "off" : "on");
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
		uint16_t flags = (this->wifi_flags != NULL) ? *(this->wifi_flags) : 0;
		printf("ESP32:\n"
				"    state: %s\n"
				"    mac: %s\n"
				"    debug: %u\n"
				"    echo: %u\n"
				"    comm: %s\n",
					uv_esp32_state_to_str(this->state),
					mac_str,
					!!(flags & ESP32_CONF_FLAGS_DEBUG),
					!!(flags & ESP32_CONF_FLAGS_ECHO),
					this->disabled ? "off" : "on");
#if CONFIG_ESP32_ESPNOW
		const uv_esp32_espnow_st *e = &this->espnow;
		printf("    espnow: %s%s, ch %u (want %u), v%u\n"
				"        name '%s', group '%s', peer '%s'%s\n"
				"        tx %u ok / %u fail, rx %u / %u dropped, "
				"%u cmd fails\n",
				uv_esp32_espnow_state_to_str(e->state),
				e->enabled ? "" : " (off)",
				(unsigned int) e->cur_channel, (unsigned int) e->channel,
				(unsigned int) e->version,
				e->name, e->group, e->peer,
				e->discoverable ? ", discoverable" : "",
				(unsigned int) e->diag.tx_ok, (unsigned int) e->diag.tx_fail,
				(unsigned int) e->diag.rx, (unsigned int) e->diag.rx_drop,
				(unsigned int) e->diag.cmd_fails);
#endif
	}
}


#endif
