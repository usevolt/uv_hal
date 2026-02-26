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
#define ESP32_RESP_BUF_SIZE		256


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
		uv_streambuffer_push(&this->rx_streambuffer, &c, 1, 0);
		uv_rtos_task_yield();
	}
}


static void esp32_reset(uv_esp32_st *this) {
	uv_streambuffer_clear(&this->tx_streambuffer);
	uv_streambuffer_clear(&this->rx_streambuffer);
	uv_mutex_unlock(&this->txstream_mutex);
	uv_mutex_unlock(&this->tx_mutex);

	uv_gpio_set(this->reset_io, false);
	uv_rtos_task_delay(20);
	uv_gpio_set(this->reset_io, true);
	uv_rtos_task_delay(1000);
}


/// @brief: Sends an AT command string followed by \r\n via tx_streambuffer
static void send_at_cmd(uv_esp32_st *this, const char *cmd) {
	uv_mutex_lock(&this->txstream_mutex);
	uv_streambuffer_push(&this->tx_streambuffer,
			(char *) cmd, strlen(cmd), 100);
	uv_streambuffer_push(&this->tx_streambuffer,
			"\r\n", 2, 100);
	uv_mutex_unlock(&this->txstream_mutex);
}


/// @brief: Reads available UART data into response buffer
static void read_uart_to_buf(uv_esp32_st *this,
		char *buf, int16_t *index, int16_t buf_size, int32_t wait_ms) {
	char c;
	while (uv_uart_get(this->uart, &c, 1, wait_ms)) {
		if (*index < buf_size - 1) {
			buf[(*index)++] = c;
			buf[*index] = '\0';
		}
		wait_ms = 0;
	}
}


static void rxtx_task(void *me_ptr) {
	uv_esp32_st *this = me_ptr;
	char resp[ESP32_RESP_BUF_SIZE];
	int16_t resp_index = 0;
	uv_delay_st timeout;
	uv_ts_st ts;

	resp[0] = '\0';
	uv_ts_init(&ts);
	uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);

	while (true) {
		uv_ts_step(&ts);
		tx(this);

		switch (this->state) {

		case ESP32_STATE_INIT:
			ESP32_DEBUG(this, "ESP32: resetting module\n");
			esp32_reset(this);
			resp_index = 0;
			resp[0] = '\0';
			uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);
			this->state = ESP32_STATE_WAIT_READY;
			ESP32_DEBUG(this, "ESP32: waiting for ready\n");
			break;

		case ESP32_STATE_WAIT_READY:
			read_uart_to_buf(this, resp, &resp_index,
					ESP32_RESP_BUF_SIZE, 1);
			if (strstr(resp, "ready") != NULL) {
				ESP32_DEBUG(this, "ESP32: module ready, sending AT\n");
				resp_index = 0;
				resp[0] = '\0';
				send_at_cmd(this, "AT");
				uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);
				this->state = ESP32_STATE_TEST_AT;
			}
			else if (uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: ready timeout, resetting\n");
				this->state = ESP32_STATE_INIT;
			}
			break;

		case ESP32_STATE_TEST_AT:
			read_uart_to_buf(this, resp, &resp_index,
					ESP32_RESP_BUF_SIZE, 1);
			if (strstr(resp, "OK") != NULL) {
				ESP32_DEBUG(this, "ESP32: AT OK, disabling echo\n");
				resp_index = 0;
				resp[0] = '\0';
				send_at_cmd(this, "ATE0");
				uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);
				this->state = ESP32_STATE_DISABLE_ECHO;
			}
			else if (uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: AT timeout, resetting\n");
				this->state = ESP32_STATE_INIT;
			}
			break;

		case ESP32_STATE_DISABLE_ECHO:
			read_uart_to_buf(this, resp, &resp_index,
					ESP32_RESP_BUF_SIZE, 1);
			if (strstr(resp, "OK") != NULL) {
				ESP32_DEBUG(this, "ESP32: echo disabled, setting CWMODE\n");
				resp_index = 0;
				resp[0] = '\0';
				send_at_cmd(this, "AT+CWMODE=1");
				uv_delay_init(&timeout, ESP32_AT_TIMEOUT_MS);
				this->state = ESP32_STATE_SET_CWMODE;
			}
			else if (uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				ESP32_DEBUG(this, "ESP32: ATE0 timeout, resetting\n");
				this->state = ESP32_STATE_INIT;
			}
			break;

		case ESP32_STATE_SET_CWMODE:
			read_uart_to_buf(this, resp, &resp_index,
					ESP32_RESP_BUF_SIZE, 1);
			if (strstr(resp, "OK") != NULL) {
				if (strlen(this->conf->ssid) > 0) {
					char cmd[SSID_STR_MAX_LEN + PASSWD_STR_MAX_LEN + 16];
					snprintf(cmd, sizeof(cmd), "AT+CWJAP=\"%s\",\"%s\"",
							this->conf->ssid, this->conf->passwd);
					ESP32_DEBUG(this, "ESP32: CWMODE set, joining '%s'\n",
							this->conf->ssid);
					resp_index = 0;
					resp[0] = '\0';
					send_at_cmd(this, cmd);
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
			break;

		case ESP32_STATE_CONNECT_WIFI:
			read_uart_to_buf(this, resp, &resp_index,
					ESP32_RESP_BUF_SIZE, 1);
			if (strstr(resp, "WIFI GOT IP") != NULL) {
				resp_index = 0;
				resp[0] = '\0';
				uv_rtos_task_delay(100);
				uv_uart_clear_rx_buffer(this->uart);
				this->state = ESP32_STATE_JOINED_NETWORK;
				ESP32_DEBUG(this, "ESP32: WiFi connected\n");
			}
			else if (strstr(resp, "ERROR") != NULL ||
					uv_delay(&timeout, uv_ts_get_step_ms(&ts))) {
				resp_index = 0;
				resp[0] = '\0';
				ESP32_DEBUG(this, "ESP32: WiFi connection failed\n");
				this->state = ESP32_STATE_LEFT_NETWORK;
			}
			break;

		case ESP32_STATE_JOINED_NETWORK:
		case ESP32_STATE_LEFT_NETWORK:
			rx(this,
					uv_streambuffer_get_len(&this->tx_streambuffer) ? 0 : 1);
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

	uv_streambuffer_init_static(&this->rx_streambuffer,
								this->rx_buffer,
								sizeof(this->rx_buffer),
								&this->rx_staticstreambuffer);

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
