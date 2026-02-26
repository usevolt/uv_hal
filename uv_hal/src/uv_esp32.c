/*
 * uv_esp32.c
 *
 *  Created on: Jan 21, 2026
 *      Author: usevolt
 */

#include "uv_esp32.h"
#include <string.h>
#include <stdio.h>

#if CONFIG_ESP32

#define ESP32_DEBUG(esp, ...) do { if ((esp)->conf->flags & ESP32_CONF_FLAGS_DEBUG) { \
	printf_flags(PRINTF_FLAGS_NOTXCALLB, __VA_ARGS__);}} while (0)

#define ESP32_AT_TIMEOUT_MS		5000
#define ESP32_WIFI_TIMEOUT_MS	15000


static void tx(uv_esp32_st *this) {
	uv_mutex_lock(&this->tx_mutex);

	int32_t tx_count = uv_streambuffer_get_len(&this->tx_streambuffer);
	if (tx_count > 0) {
		char c;
		for (int32_t i = 0; i < tx_count; i++) {
			uv_streambuffer_pop(&this->tx_streambuffer, &c, 1, 0);
			if (!uv_uart_send(this->uart, &c, 1)) {
				break;
			}
		}
	}

	uv_mutex_unlock(&this->tx_mutex);
}


static void rx(uv_esp32_st *this, int32_t wait_ms) {
	char c;
	while (uv_uart_get(this->uart, &c, 1, wait_ms)) {
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


/// @brief: Pops characters from rx_datastream and accumulates them into
/// at_resp line buffer. When a complete line is received, checks if
/// it contains str1 or str2 (str2 can be NULL).
/// Returns 1 if str1 matched, 2 if str2 matched, 0 if neither.
static uint8_t rx_at_match(uv_esp32_st *this,
		const char *str1, const char *str2) {
	uint8_t ret = 0;
	char c;
	while (uv_streambuffer_pop(&this->rx_datastream, &c, 1, 0)) {
		if (at_resp_accumulate(this, c)) {
			this->at_resp_i = 0;
			if (strstr(this->at_resp, str1) != NULL) {
				this->rx_at_cmd = NULL;
				ret = 1;
			}
			else if (str2 != NULL &&
					strstr(this->at_resp, str2) != NULL) {
				this->rx_at_cmd = NULL;
				ret = 2;
			}
			else {
			}
			if (ret != 0) {
				break;
			}
		}
		else {
		}
	}
	return ret;
}


static void rxtx_task(void *me_ptr) {
	uv_esp32_st *this = me_ptr;
	uv_delay_st timeout;
	uv_ts_st ts;

	uv_ts_init(&ts);
	uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);

	while (true) {
		uv_ts_step(&ts);
		tx(this);
		rx(this, uv_streambuffer_get_len(&this->rx_datastream) ? 0 : 1);

		switch (this->state) {

		case ESP32_STATE_INIT:
			ESP32_DEBUG(this, "ESP32: resetting module\n");
			esp32_reset(this);
			uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);
			this->state = ESP32_STATE_WAIT_READY;
			ESP32_DEBUG(this, "ESP32: waiting for ready\n");
			break;

		case ESP32_STATE_WAIT_READY:
			if (rx_at_match(this, "ready", NULL)) {
				ESP32_DEBUG(this, "ESP32: module ready, sending AT\n");
				send_at_cmd(this, "AT", NULL, NULL);
				uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);
				this->state = ESP32_STATE_TEST_AT;
			}
			else if (uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: ready timeout, resetting\n");
				this->state = ESP32_STATE_INIT;
			}
			else {
			}
			break;

		case ESP32_STATE_TEST_AT:
			if (rx_at_match(this, "OK", NULL)) {
				ESP32_DEBUG(this, "ESP32: AT OK, disabling echo\n");
				send_at_cmd(this, "ATE0", NULL, NULL);
				uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);
				this->state = ESP32_STATE_DISABLE_ECHO;
			}
			else if (uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: AT timeout, resetting\n");
				this->state = ESP32_STATE_INIT;
			}
			else {
			}
			break;

		case ESP32_STATE_DISABLE_ECHO:
			if (rx_at_match(this, "OK", NULL)) {
				ESP32_DEBUG(this, "ESP32: echo disabled, setting CWMODE\n");
				send_at_cmd(this, "AT+CWMODE=1", NULL, NULL);
				uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);
				this->state = ESP32_STATE_SET_CWMODE;
			}
			else if (uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: ATE0 timeout, resetting\n");
				this->state = ESP32_STATE_INIT;
			}
			else {
			}
			break;

		case ESP32_STATE_SET_CWMODE:
			if (rx_at_match(this, "OK", NULL)) {
				if (strlen(this->conf->ssid) > 0) {
					ESP32_DEBUG(this, "ESP32: CWMODE set, joining '%s'\n",
							this->conf->ssid);
					send_at_cmd(this, "AT+CWJAP",
							this->conf->ssid, this->conf->passwd);
					uv_delay_init(&timeout, ESP32_WIFI_TIMEOUT_MS);
					this->state = ESP32_STATE_CONNECT_WIFI;
				}
				else {
					ESP32_DEBUG(this, "ESP32: CWMODE set, no SSID configured\n");
					this->state = ESP32_STATE_LEFT_NETWORK;
				}
			}
			else if (uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: CWMODE timeout, resetting\n");
				this->state = ESP32_STATE_INIT;
			}
			else {
			}
			break;

		case ESP32_STATE_CONNECT_WIFI: {
			uint8_t match = rx_at_match(this, "WIFI GOT IP", "ERROR");
			if (match == 1) {
				uv_uart_clear_rx_buffer(this->uart);
				uv_streambuffer_clear(&this->rx_datastream);
				this->state = ESP32_STATE_JOINED_NETWORK;
				ESP32_DEBUG(this, "ESP32: WiFi connected\n");
			}
			else if (match == 2 ||
					uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: WiFi connection failed\n");
				this->state = ESP32_STATE_LEFT_NETWORK;
			}
			else {
			}
			break;
		}

		case ESP32_STATE_JOINED_NETWORK:
		case ESP32_STATE_LEFT_NETWORK:
			break;

		default:
			this->state = ESP32_STATE_INIT;
			break;
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
}


uv_errors_e uv_esp32_get_data(uv_esp32_st *this, char *dest) {
	return ERR_HARDWARE_NOT_SUPPORTED;
}


uv_errors_e uv_esp32_write(uv_esp32_st *this,
		char *data, uint16_t datalen, int32_t wait_ms) {
	return ERR_HARDWARE_NOT_SUPPORTED;
}


uv_errors_e uv_esp32_write_isr(uv_esp32_st *this,
		char *data, uint16_t datalen) {
	return ERR_HARDWARE_NOT_SUPPORTED;
}


uint64_t uv_esp32_get_mac(uv_esp32_st *this) {
	return 0;
}


char *uv_esp32_get_connected_ssid(uv_esp32_st *this) {
	return "";
}


void uv_esp32_network_reset(uv_esp32_st *this) {
}


void uv_esp32_network_leave(uv_esp32_st *this) {
}


void uv_esp32_network_join(uv_esp32_st *this, char ssid[32],
						   char passwd[64]) {
}


void uv_esp32_terminal(uv_esp32_st *this,
		unsigned int args, argument_st *argv) {
	printf("ESP32:\n");
}


#endif
