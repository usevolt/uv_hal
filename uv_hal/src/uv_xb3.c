/*
 * xb3.c
 *
 *  Created on: Dec 19, 2024
 *      Author: usevolt
 */



#include "../inc/uv_xb3.h"
#include <uv_rtos.h>
#include <uv_terminal.h>
#include <uv_wdt.h>
#include "uv_memory.h"

#if CONFIG_XB3 && CONFIG_UART


#define APIFRAME_64TRANSMIT					0x0
#define APIFRAME_16TRANSMIT					0x1
#define APIFRAME_LOCALATCMDREQ				0x8
#define APIFRAME_QUEUELOCALATCMDREQ			0x9
#define APIFRAME_TRANSMITREQ				0x10
#define APIFRAME_EXPLADDCMDREQ				0x11
#define APIFRAME_REMOTEATCMDREQ				0x17
#define APIFRAME_CREATESOURCEROUTE			0x21
#define APIFRAME_REGISTERJOINDEVICE			0x24
#define APIFRAME_BLEUNLOCKREQ				0x2C
#define APIFRAME_USERDATARELAYINPUT			0x2D
#define APIFRAME_SECURESESSIONCONTROL		0x2E
#define APIFRAME_LOCALATCMDRESPONSE			0x88
#define APIFRAME_TRANSMITSTATUS				0x89
#define APIFRAME_MODEMSTATUS				0x8A
#define APIFRAME_EXTTRANSMITSTATUS			0x8B
#define APIFRAME_RECEIVEPACKET				0x90
#define APIFRAME_EXPLRECEIVEIND				0x91
#define APIFRAME_IOSAMPLEIND				0x92
#define APIFRAME_NODEIDENTIFICATIONIND		0x95
#define APIFRAME_REMOTEATCMDRESPONSE		0x97
#define APIFRAME_EXTMODEMSTATUS				0x98
#define APIFRAME_MANYTOONEREQ				0xA3
#define APIFRAME_BLEUNLOCKRESPONSE			0xAC
#define APIFRAME_USERDATARELAYOUTPUT		0xAD
#define APIFRAME_SECURESESSIONRESPONSE		0xAE
#define APIFRAME_START						0x7E



#define JOINWINDOW_DELAY_MS					200


#define XB3_DEBUG(xb3, ...) do { if ((xb3)->conf->flags & XB3_CONF_FLAGS_DEBUG) { \
	printf_set_flags(PRINTF_FLAGS_NOTXCALLB); \
	printf(__VA_ARGS__); \
	printf_clear_flags(PRINTF_FLAGS_NOTXCALLB); }} while (0)



static void tx(uv_xb3_st *this);
static void rx(uv_xb3_st *this, int32_t wait_ms);



/// @brief: Converts uint64_t data from network byte order to local byte order.
/// Use this for data received from XB3. Reads 8 bytes from *srcqueue*.
static uint64_t ntouint64_queue(uv_xb3_st *this, uv_queue_st *srcqueue) {
	uint64_t ret = 0;
	uint64_t data = 0;
	for (uint8_t i = 0; i < 8; i++) {
		uv_queue_pop(srcqueue, &((uint8_t*) &data)[i], 0);
	}

	ret = ntouint64(data);

	return ret;
}


/// @brief: Converts uint64_t data from network byte order to local byte order.
/// Use this for data received from XB3. Reads 8 bytes from *srcqueue*.
static uint32_t ntouint32_queue(uv_xb3_st *this, uv_queue_st *srcqueue) {
	uint32_t ret = 0;
	uint32_t data = 0;
	for (uint8_t i = 0; i < 4; i++) {
		uv_queue_pop(srcqueue, &((uint8_t*) &data)[i], 0);
	}

	ret = ntouint32(data);

	return ret;
}


/// @brief: Converts uint16_t data from network byte order to local byte order.
/// Use this for data received from XB3. Reads 2 bytes from *srcqueue*.
static uint16_t ntouint16_queue(uv_xb3_st *this, uv_queue_st *srcqueue) {
	uint16_t ret = 0;
	uint16_t data = 0;
	for (uint8_t i = 0; i < 2; i++) {
		uv_queue_pop(srcqueue, &((uint8_t*) &data)[i], 0);
	}

	ret = ntouint16(data);

	return ret;
}

/// @brief: Converts uint8_t data from network byte order to local byte order.
/// Use this for data received from XB3. Reads 1 bytes from *srcqueue*.
static uint8_t ntouint8_queue(uv_xb3_st *this, uv_queue_st *srcqueue) {
	uint8_t ret = 0;
	uv_queue_pop(srcqueue, &ret, 0);

	return ret;
}


static void uint64ton(char *dest, uint64_t value) {
	dest[0] = (value >> 56) & 0xFF;
	dest[1] = (value >> 48) & 0xFF;
	dest[2] = (value >> 40) & 0xFF;
	dest[3] = (value >> 32) & 0xFF;
	dest[4] = (value >> 24) & 0xFF;
	dest[5] = (value >> 16) & 0xFF;
	dest[6] = (value >> 8) & 0xFF;
	dest[7] = (value >> 0) & 0xFF;
}


const char *uv_xb3_modem_status_to_str(uv_xb3_modem_status_e stat) {
	const char *ret = "";
	switch (stat) {
	case XB3_MODEMSTATUS_ACCESSFAULT: ret = "ACCESSFAULT"; break;
	case XB3_MODEMSTATUS_BLECONNECT: ret = "BLECONNECT"; break;
	case XB3_MODEMSTATUS_BLEDISCONNECT: ret = "BLEDISCONNECT"; break;
	case XB3_MODEMSTATUS_COORDINATORCHANGEDPANID: ret = "COORDINATORCHANGEDPANID"; break;
	case XB3_MODEMSTATUS_COORDINATORDETECTEDPANIDCONFLICT:
		ret = "COORDINATORDETECTEDPANIDCONFLICT"; break;
	case XB3_MODEMSTATUS_COORDINATORSTARTED: ret = "COORDINATORSTARTED"; break;
	case XB3_MODEMSTATUS_FATALERROR: ret = "FATALERROR"; break;
	case XB3_MODEMSTATUS_JOINEDNETWORK: ret = "JOINEDNETWORK"; break;
	case XB3_MODEMSTATUS_LEFTNETWORK: ret = "LEFTNETWORK"; break;
	case XB3_MODEMSTATUS_MODEMCONFCHANGEDWHILEJOIN:
		ret = "MODEMCONFCHANGEDWHILEJOIN"; break;
	case XB3_MODEMSTATUS_NETWORKSECURITYKEYUPDATED:
		ret = "NETWORKSECURITYKEYUPDATED"; break;
	case XB3_MODEMSTATUS_NETWORKWENTTOSLEEP: ret = "NETWORKWENTTOSLEEP"; break;
	case XB3_MODEMSTATUS_NETWORKWOKEUP: ret = "NETWORKWOKEUP"; break;
	case XB3_MODEMSTATUS_POWERUP: ret = "POWERUP"; break;
	case XB3_MODEMSTATUS_REMOTEMANAGEDISCONNECTED: ret = "REMOTEMANAGEDISCONNECTED"; break;
	case XB3_MODEMSTATUS_REMOTEMANAGERCONNECTED: ret = "REMOTEMANAGERCONNECTED"; break;
	case XB3_MODEMSTATUS_VCCEXCEEDED: ret = "VCCEXCEEDED"; break;
	case XB3_MODEMSTATUS_WDT: ret = "WDT"; break;
	case XB3_MODEMSTATUS_ROUTERPANIDCHANGEDBYCOORDINATOR:
		ret = "ROUTERPANIDCHANGEDBYCOORDINATOR"; break;
	case XB3_MODEMSTATUS_NETWORKWDT: ret = "NETWORKWDT TIMEDOUT"; break;
	case XB3_MODEMSTATUS_JOINWINDOWOPEN: ret = "JOINWINDOWOPEN"; break;
	case XB3_MODEMSTATUS_JOINWINDOWCLOSED: ret = "JOINWINDOWCLOSED"; break;
	default:
		ret = "UNKNOWN";
		break;
	}
	return ret;
}


static void tx(uv_xb3_st *this) {
	uv_mutex_lock(&this->tx_mutex);

	int32_t tx_count = MIN(XB3_RF_PACKET_MAX_LEN - 20,
			uv_streambuffer_get_len(&this->tx_streambuffer));

	if (tx_count &&
			uv_uart_get_tx_free_space(this->uart) >=
			MIN(XB3_RF_PACKET_MAX_LEN,
					uv_streambuffer_get_len(&this->tx_streambuffer))) {

		// write to XB3
		uint16_t framedatalen = 14 + tx_count;
		char buffer[17];
		char *d = buffer;
		*d++ = APIFRAME_START;
		*d++ = (framedatalen >> 8); // Length
		*d++ = (framedatalen & 0xFF);
		*d++ = APIFRAME_TRANSMITREQ;
		*d++ = 0x52; // Frame ID. 0x0 doesn't emit response frame
		*d++ = (this->conf->dest_addr >> 56) & 0xFF; // 64-bit address
		*d++ = (this->conf->dest_addr >> 48) & 0xFF;
		*d++ = (this->conf->dest_addr >> 40) & 0xFF;
		*d++ = (this->conf->dest_addr >> 32) & 0xFF;
		*d++ = (this->conf->dest_addr >> 24) & 0xFF;
		*d++ = (this->conf->dest_addr >> 16) & 0xFF;
		*d++ = (this->conf->dest_addr >> 8) & 0xFF;
		*d++ = (this->conf->dest_addr) & 0xFF;
		*d++ = 0xFF; // 16-bit address
		*d++ = 0xFE;
		*d++ = 0; // broadcast radius
		*d++ = 0; // transmit options

		uv_uart_send(this->uart, buffer, 17);

		uint8_t crc = 0;
		// crc doesn't include first 3 bytes
		for (uint8_t i = 3; i < 17; i++) {
			crc += (uint8_t) buffer[i];
		}

		char c;
		for (uint8_t i = 0; i < tx_count; i++) {
			uv_streambuffer_pop(&this->tx_streambuffer, &c, 1, 0);
			uv_uart_send(this->uart, &c, 1);
			crc += (uint8_t) c;
		}

		c = 0xFF - crc;
		uv_uart_send(this->uart, &c, 1);
	}
	uv_mutex_unlock(&this->tx_mutex);
}

static void rx(uv_xb3_st *this, int32_t wait_ms) {
	char rx;

	while (uv_uart_get(this->uart, &rx, 1, wait_ms)) {
		if (this->rx_index == 0) {
			if (rx == APIFRAME_START) {
				this->rx_index = 1;
				this->rx_size = 0;
			}
			else {
				printf("0x%x ", rx);
			}
		}
		else {
			this->rx_index++;

			uint16_t offset = this->rx_index - 1;

			switch(this->rx_index) {
			// API package length MSB byte
			case 2:
				this->rx_size += (rx << 8);
				break;
			// API package length LSB byte
			case 3:
				this->rx_size += rx;
				break;
			// API Frame type
			case 4:
				this->rx_frame_type = rx;
				break;
			// API data
			default:
				if (this->rx_index - 4 < this->rx_size) {
					switch (this->rx_frame_type) {
					case APIFRAME_LOCALATCMDRESPONSE: {
						if (this->rx_index == 8) {
							this->at_response_req = rx;
						}
						else if (this->rx_index > 8) {
							if (this->conf->flags & XB3_CONF_FLAGS_AT_ECHO) {
								printf("0x%02x ", rx);
							}
							if (uv_queue_push(&this->rx_at_queue, &rx, 0) != ERR_NONE) {
								XB3_DEBUG(this, "XB3: AT rx queue full\n");
							}
						}
						else {

						}
						if (this->rx_index - 4 == this->rx_size - 1) {
							// last byte of this response, update AT response status
							if (this->conf->flags & XB3_CONF_FLAGS_AT_ECHO ||
									this->conf->flags & XB3_CONF_FLAGS_DEBUG) {
								switch(this->at_response_req) {
								case XB3_AT_RESPONSE_OK:
									printf("OK\n");
									break;
								case XB3_AT_RESPONSE_ERROR:
									printf("ERROR\n");
									break;
								case XB3_AT_RESPONSE_INVALID_COMMAND:
									printf("Invalid command\n");
									break;
								case XB3_AT_RESPONSE_INVALID_PARAMETER:
									printf("Invalid parameter\n");
									break;
								default:
									break;
								}
							}
							this->at_response = this->at_response_req;
						}
						break;
					}
					case APIFRAME_MODEMSTATUS:
						if (offset == 4) {
							XB3_DEBUG(this, "MODEMSTATUS 0x%x '%s'\n", rx,
									uv_xb3_modem_status_to_str(rx));
							// Joinwindow is always open,
							// it just messes up state machine
							if (rx != XB3_MODEMSTATUS_JOINWINDOWOPEN &&
									rx != XB3_MODEMSTATUS_JOINWINDOWCLOSED) {
								this->modem_status = rx;
								this->modem_status_changed = rx;
							}
						}
						break;
					case APIFRAME_RECEIVEPACKET:
						if (offset >= 15) {
							if (!uv_streambuffer_push_isr(&this->rx_data_streambuffer,
									&rx, 1)) {
								XB3_DEBUG(this, "XB3 receive buffer overflow\n");
							}
							this->rx_max = MAX(this->rx_max,
									uv_streambuffer_get_len(&this->rx_data_streambuffer));
							if (this->conf->flags & XB3_CONF_FLAGS_RX_ECHO) {
								printf("%c", rx);
							}
						}
						break;
					case APIFRAME_EXTTRANSMITSTATUS:
						if (offset == 8) {
							if (rx) {
								XB3_DEBUG(this, "XB3 TRANSMIT fail 0x%x", rx);
							}
						}
						else if (offset == 7) {
							this->max_retransmit = MAX(this->max_retransmit, rx);
						}
						else if (offset == 9) {

						}
						else {

						}
						break;
					default:
						break;
					}
				}
				else {
					// last byte is CRC, which marks the end of transmission
					this->rx_index = 0;
				}
				break;
			}
		}
	}
}

static inline void tx_rx(uv_xb3_st *this, int32_t wait_ms) {
	tx(this);
	rx(this, uv_streambuffer_get_len(&this->tx_streambuffer) ? 0 : 1);
}





static uv_xb3_at_response_e at_wait_for_reply(uv_xb3_st *this, int32_t wait_ms,
		bool xb3_task) {
	while (this->at_response == XB3_AT_RESPONSE_COUNT &&
			wait_ms > 0) {
		if (xb3_task) {
			// if we are in xb3_step_task, handle tx and rx data
			tx_rx(this, 1);
		}
		else {
			// otherwise just wait, xb3 task handles all serial communication
			uv_rtos_task_delay(1);
		}
		wait_ms -= 1;
	}
	if (wait_ms <= 0) {
		this->at_response = XB3_AT_RESPONSE_TIMEOUT;
		if (this->conf->flags & XB3_CONF_FLAGS_AT_ECHO) {
			printf("TIMEOUT\n");
		}
	}
	return this->at_response;
}


/// @brief: Writes a local AT command request to device
/// according to frame type LOCALATCMDREQ
void uv_xb3_local_at_cmd_req(uv_xb3_st *this, char *atcmd,
		char *data, uint16_t data_len) {
	uv_mutex_lock(&this->tx_mutex);

	this->at_response = XB3_AT_RESPONSE_COUNT;
	uv_queue_clear(&this->rx_at_queue);
	uint8_t crc = 0;
	uint16_t paramvaluelen = data_len;
	if (data != NULL &&
			paramvaluelen == 0) {
		paramvaluelen = strlen(data);
	}

	while (uv_uart_get_tx_free_space(this->uart) < 8 + paramvaluelen) {
		uv_rtos_task_delay(1);
	}

	int32_t len = 0;
	char buffer[7];
	char *d = buffer;
	*d++ = APIFRAME_START;
	uint16_t framedatalen = 4 + paramvaluelen;
	*d++ = (framedatalen >> 8); // length MSB
	*d++ = framedatalen & 0xFF; // length LSB
	*d = APIFRAME_LOCALATCMDREQ;
	crc += (uint8_t) *d++;
	// Frame ID, has to be something else than 0
	*d = 0x17;
	crc += (uint8_t) *d++;
	*d = atcmd[0];
	crc += (uint8_t) *d++;
	*d = atcmd[1];
	crc += (uint8_t) *d++;
	len += uv_uart_send(this->uart, buffer, 7);
	len += uv_uart_send(this->uart, data, paramvaluelen);
	for (uint16_t i = 0; i < paramvaluelen; i++) {
		crc += (uint8_t) data[i];
	}
	buffer[0] = (uint8_t) 0xFF - (crc & 0xFF);
	len += uv_uart_send(this->uart, buffer, 1);

	if (len != 8 + paramvaluelen) {
		printf("AT error: UART RX buffer full\n");
	}

	if (this->conf->flags & XB3_CONF_FLAGS_AT_ECHO) {
		printf("AT %s ", atcmd);
		for (uint16_t i = 0; i < data_len; i++) {
			printf("0x%02x ", data[i]);
		}
		printf("\n");
	}

	uv_mutex_unlock(&this->tx_mutex);
}

#define TX_DEBUG(this, ...)	if (this->conf->flags & XB3_CONF_FLAGS_TX_ECHO) \
	XB3_DEBUG(this, __VA_ARGS__);


#define TX_BUF_SIZE		(700)
#define RX_BUF_SIZE		300



uv_errors_e uv_xb3_generic_write(uv_xb3_st *this, char *data,
		uint16_t datalen, bool isr) {
	uv_errors_e ret = ERR_NONE;
	if (this->initialized) {
		if (uv_streambuffer_get_free_space(&this->tx_streambuffer) < datalen) {
			XB3_DEBUG(this, "XB3 Write: buffer full %i / %i, required %i\n",
					(int) uv_streambuffer_get_len(&this->tx_streambuffer),
					TX_BUF_SIZE,
					(int) datalen);
			ret = ERR_NOT_ENOUGH_MEMORY;
		}
		else if (!this->initialized) {
			XB3_DEBUG(this, "XB3 write: Not initialized\n");
			ret = ERR_NOT_INITIALIZED;
		}
		else {
			isr ? uv_enter_critical_isr() : uv_enter_critical();

			isr ? uv_streambuffer_push_isr(&this->tx_streambuffer, data, datalen) :
					uv_streambuffer_push(&this->tx_streambuffer, data, datalen, 0);
			this->tx_max = MAX(this->tx_max,
					uv_streambuffer_get_len(&this->tx_streambuffer));

			isr ? uv_exit_critical_isr() : uv_exit_critical();
		}
	}
	else {
		ret = ERR_NOT_INITIALIZED;
	}
	return ret;
}



uv_errors_e uv_xb3_write_sync(uv_xb3_st *this, char *data,
		uint16_t datalen) {
	uv_errors_e ret = ERR_NONE;
	// poll to clear transmit buffer
	ret = uv_xb3_generic_write(this, data, datalen, false);
	// poll to send message

	return ret;
}



void uv_xb3_conf_reset(uv_xb3_conf_st *conf, uint16_t flags_def, uv_uarts_e uart) {
	memset(conf, 0, sizeof(uv_xb3_conf_st));
	conf->flags = flags_def;

	uv_terminal_enable(TERMINAL_CAN);

	// break condition enters command mode in 9600 baud
	printf("XB3 entering to command mode via serial break\n");
	uv_uart_set_baudrate(uart, 9600);
	uv_uart_clear_rx_buffer(uart);

	uv_uart_break_start(uart);
	uv_rtos_task_delay(6000);

	if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
		printf("OK\n");
	}
	else {
		printf("Couldn't put XB3 to command mode\n");
	}
	uv_uart_break_stop(uart);
	uv_rtos_task_delay(2000);

	char dest[11] = { };
	uv_uart_clear_rx_buffer(uart);

	if (flags_def & XB3_CONF_FLAGS_OPERATE_AS_COORDINATOR) {
		// clear ID
		uv_uart_send(uart, "ATID0\r", 6);
		printf("%s\n", dest);
		if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
			printf("OK\n");
		}
		else {
			printf("ERROR\n");
		}

		// operate as COORDINATOR
		uv_uart_send(uart, "ATCE1\r", 6);
		printf("%s\n", dest);
		if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
			printf("OK\n");
		}
		else {
			printf("ERROR\n");
		}

		// Node join time is infinite
		uv_uart_send(uart, "ATNJFF\r", 7);
		printf("%s\n", dest);
		if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
			printf("OK\n");
		}
		else {
			printf("ERROR\n");
		}
	}
	else {
		// write join device controls
		uv_uart_send(uart, "ATDC48\r", 7);
		printf("%s\n", dest);
		if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
			printf("OK\n");
		}
		else {
			printf("ERROR\n");
		}

		// clear ID
		uv_uart_send(uart, "ATID0\r", 6);
		printf("%s\n", dest);
		if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
			printf("OK\n");
		}
		else {
			printf("ERROR\n");
		}
	}


	printf("Set baudrate to 921600\n");
	sprintf(dest, "ATBDA\r");
	uv_uart_send(uart, dest, strlen(dest));
	printf("%s\n", dest);
	if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
		printf("OK\n");
	}
	else {
		printf("ERROR\n");
	}

	strcpy(dest, "ATAP1\r");
	printf("Putting XB3 to API mode\n");
	uv_uart_clear_rx_buffer(uart);
	uv_uart_send(uart, dest, strlen(dest));
	printf("%s\n", dest);
	if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
		printf("OK\n");
	}
	else {
		printf("ERROR\n");
	}

	printf("Write changes... ");
	strcpy(dest, "ATWR\r");
	uv_uart_clear_rx_buffer(uart);
	uv_uart_send(uart, dest, strlen(dest));
	printf("%s\n", dest);
	if (uv_uart_receive_cmp(uart, "OK\r", 3, 1000)) {
		printf("OK\n");
	}
	else {
		printf("ERROR\n");
	}

	uv_uart_set_baudrate(uart, 921600);
}




static bool xb3_reset(uv_xb3_st *this) {
	bool ret = true;

	uv_queue_clear(&this->rx_at_queue);
	uv_streambuffer_clear(&this->rx_data_streambuffer);
	uv_streambuffer_clear(&this->tx_streambuffer);

	uv_gpio_set(this->reset_gpio, false);
	uv_rtos_task_delay(20);
	uv_gpio_set(this->reset_gpio, true);
	uv_rtos_task_delay(1000);

	return ret;
}


static uv_xb3_at_response_e set_nodename(
		uv_xb3_st *this, const char *name, bool xb3_task) {
	uv_xb3_at_response_e ret = XB3_AT_RESPONSE_COUNT;
	uv_mutex_lock(&this->atreq_mutex);

	char str[20] = {};
	strncpy(str, name, 19);
	uv_xb3_local_at_cmd_req(this, "NI", (char*) str, strlen(str) + 1);
	ret = at_wait_for_reply(this, 1000, xb3_task);

	uv_mutex_unlock(&this->atreq_mutex);

	return ret;
}




uv_errors_e uv_xb3_init(uv_xb3_st *this,
		uv_xb3_conf_st *conf,
		uv_gpios_e reset_gpio,
		uv_uarts_e uart,
		const char *nodeid) {
	uv_errors_e ret = ERR_NONE;
	this->conf = conf;
	this->initialized = false;
	this->uart = uart;
	this->reset_gpio = reset_gpio;
	this->at_response = XB3_AT_RESPONSE_COUNT;
	this->rx_index = 0;
	this->rx_size = 0;
	this->modem_status = XB3_MODEMSTATUS_NONE;
	this->modem_status_changed = XB3_MODEMSTATUS_NONE;
	this->max_retransmit = 0;
	memset(&this->network, 0, sizeof(this->network));

	if (uv_streambuffer_init(&this->tx_streambuffer, TX_BUF_SIZE) != ERR_NONE) {
		uv_terminal_enable(TERMINAL_CAN);
		printf("XB3: Creating TX streambuffer failed, not enough memory\n");
		ret = ERR_NOT_ENOUGH_MEMORY;
	}
	this->tx_max = 0;
	if (uv_streambuffer_init(&this->rx_data_streambuffer, RX_BUF_SIZE) != ERR_NONE) {
		uv_terminal_enable(TERMINAL_CAN);
		printf("XB3: Creating RX streambuffer failed, not enough memory\n");
		ret = ERR_NOT_ENOUGH_MEMORY;
	}
	this->rx_max = 0;
	if (uv_queue_init(&this->rx_at_queue, 100, sizeof(char)) == NULL) {
		uv_terminal_enable(TERMINAL_CAN);
		printf("XB3: Creating RX AT queue failed\n");
		ret = ERR_NOT_ENOUGH_MEMORY;
	}


	uv_mutex_init(&this->atreq_mutex);
	uv_mutex_unlock(&this->atreq_mutex);

	uv_mutex_init(&this->tx_mutex);
	uv_mutex_unlock(&this->tx_mutex);

	uv_gpio_init_output(this->reset_gpio, false);

	xb3_reset(this);

	XB3_DEBUG(this, "Setting device identification string to '%s'\n", nodeid);
	uv_xb3_at_response_e res = set_nodename(this, nodeid, true);

	uv_xb3_local_at_cmd_req(this, "SH", "", 0);
	res |= at_wait_for_reply(this, 1000, true);
	this->ieee_serial = ntouint32_queue(this, &this->rx_at_queue);
	this->ieee_serial = this->ieee_serial << 32;
	uv_xb3_local_at_cmd_req(this, "SL", "", 0);
	res |= at_wait_for_reply(this, 1000, true);
	this->ieee_serial += ntouint32_queue(this, &this->rx_at_queue);

	if (res == XB3_AT_RESPONSE_OK) {
		this->initialized = true;
	}
	else {
		uv_terminal_enable(TERMINAL_CAN);
		printf("XB3 init error. To reverting all settings to defaults.\n");
		ret = ERR_HARDWARE_NOT_SUPPORTED;
	}

	return ret;
}




uv_xb3_at_response_e uv_xb3_set_nodename(uv_xb3_st *this, const char *name) {
	return set_nodename(this, name, false);
}



void uv_xb3_step(uv_xb3_st *this, uint16_t step_ms) {
	if (this->initialized) {
		uv_enter_critical();
		uv_xb3_modem_status_e modem_status_changed = this->modem_status_changed;
		this->modem_status_changed = XB3_MODEMSTATUS_NONE;
		uv_exit_critical();

		// wait until network is created or device is booted
		if (modem_status_changed == XB3_MODEMSTATUS_COORDINATORSTARTED) {
			uv_mutex_lock(&this->atreq_mutex);
			if (this->conf->epanid == 0) {
				// "OP" contains random EPID that we copy to ID to keep
				// constant EPID in future
				uv_xb3_local_at_cmd_req(this, "OP", "", 0);
				at_wait_for_reply(this, 500, true);
				this->conf->epanid = ntouint64_queue(this, &this->rx_at_queue);

				this->network.op = this->conf->epanid;
				uv_xb3_local_at_cmd_req(this, "OI", "", 0);
				at_wait_for_reply(this, 500, true);
				this->network.oi = ntouint16_queue(this, &this->rx_at_queue);
				uv_xb3_local_at_cmd_req(this, "CH", "", 0);
				at_wait_for_reply(this, 500, true);
				this->network.ch = ntouint8_queue(this, &this->rx_at_queue);
				char data[8];
				uint64ton(data, this->conf->epanid);
				uv_xb3_local_at_cmd_req(this, "ID", data, sizeof(data));
				at_wait_for_reply(this, 500, true);

				if (this->conf->epanid) {
					// write modifications to nonvol memory on XB3
					uv_xb3_local_at_cmd_req(this, "WR", "", 0);
					if (at_wait_for_reply(this, 500, true) == XB3_AT_RESPONSE_OK) {
						XB3_DEBUG(this, "XB3: Wrote configuration to nonvol memory\n");
						uv_memory_save();
					}
					else {
						XB3_DEBUG(this, "XB3: Writing changes failed\n");
					}
				}
			}
			uv_mutex_unlock(&this->atreq_mutex);
		}
		else if (modem_status_changed == XB3_MODEMSTATUS_JOINEDNETWORK) {
			uv_mutex_lock(&this->atreq_mutex);
			// routers by default connect to first network they find.
			// This is OK for new devices, but we dont want to accidentally
			// connect to wrong networks. Thus disable autojoining by
			// only joining to networks that we connect to.
			uv_xb3_local_at_cmd_req(this, "OP", "", 0);
			at_wait_for_reply(this, 500, true);
			uint64_t id = ntouint64_queue(this, &this->rx_at_queue);

			if (id != this->conf->epanid) {
				char data[8];
				this->conf->epanid = id;
				uint64ton(data, this->conf->epanid);
				uv_xb3_local_at_cmd_req(this, "ID", data, 8);
				at_wait_for_reply(this, 500, true);

				uv_xb3_local_at_cmd_req(this, "WR", "", 0);
				at_wait_for_reply(this, 500, true);
				XB3_DEBUG(this, "XB3: Wrote configuration to nonvol memory\n");
				uv_memory_save();
			}
			this->network.op = id;
			uv_xb3_local_at_cmd_req(this, "OI", "", 0);
			at_wait_for_reply(this, 500, true);
			this->network.oi = ntouint16_queue(this, &this->rx_at_queue);
			uv_xb3_local_at_cmd_req(this, "CH", "", 0);
			at_wait_for_reply(this, 500, true);
			this->network.ch = ntouint8_queue(this, &this->rx_at_queue);

			uv_mutex_unlock(&this->atreq_mutex);
		}
		else {

		}

		tx_rx(this, 1);
	}
}







uv_xb3_at_response_e uv_xb3_scan_networks(uv_xb3_st *this,
		uint8_t *network_count,
		uv_xb3_network_st *dest,
		uint8_t network_max_count,
		bool xb3_step_task) {

	uv_mutex_lock(&this->atreq_mutex);

	uv_xb3_local_at_cmd_req(this, "AS", "", 0);
	if (network_count) {
		*network_count = 0;
	}
	for (uint8_t i = 0; i < network_max_count; i++) {
		// wait until AT response is received
		at_wait_for_reply(this, 6000, xb3_step_task);

		bool br = false;
		uint8_t data = 0;
		uv_xb3_network_st d;
		if (uv_xb3_get_at_response(this) != XB3_AT_RESPONSE_OK ||
				uv_queue_pop(&this->rx_at_queue, &data, 0) != ERR_NONE) {
			// no data received, it means scan has ended
			br = true;
		}
		else if (data == 0x2) {
			this->at_response = XB3_AT_RESPONSE_COUNT;

			uv_queue_pop(&this->rx_at_queue, &d.channel, 0);
			d.pan16 = 0;
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan16 |= (((uint16_t) data) << 8);
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan16 |= (((uint16_t) data) << 0);
			d.pan64 = 0;
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan64 |= ((uint64_t) data << 56);
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan64 |= ((uint64_t) data << 48);
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan64 |= ((uint64_t) data << 40);
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan64 |= ((uint64_t) data << 32);
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan64 |= ((uint64_t) data << 24);
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan64 |= ((uint64_t) data << 16);
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan64 |= ((uint64_t) data << 8);
			uv_queue_pop(&this->rx_at_queue, &data, 0);
			d.pan64 |= (data << 0);
			uv_queue_pop(&this->rx_at_queue, &d.allowjoin, 0);
			uv_queue_pop(&this->rx_at_queue, &d.stackprofile, 0);
			uv_queue_pop(&this->rx_at_queue, &d.lqi, 0);
			uv_queue_pop(&this->rx_at_queue, &d.rssi, 0);
			memcpy(&dest[i], &d, sizeof(d));
			(*network_count)++;
		}
		else {
			br = true;
		}
		if (br) {
			break;
		}
	}

	uv_xb3_at_response_e ret = uv_xb3_get_at_response(this);

	uv_mutex_unlock(&this->atreq_mutex);

	return ret;
}



uv_xb3_at_response_e uv_xb3_network_discovery(uv_xb3_st *this,
		uint8_t *dev_count,
		uv_xb3_nddev_st *dest,
		uint8_t dev_max_count,
		void (*found_dev_callb)(uint8_t index, uv_xb3_nddev_st *dev),
		bool xb3_step_task) {
	uv_mutex_lock(&this->atreq_mutex);

	// get discovery time
	uv_xb3_local_at_cmd_req(this, "NT", "", 0);
	at_wait_for_reply(this, 500, xb3_step_task);
	uint16_t discovery_time_ms = ntouint16_queue(this, &this->rx_at_queue) * 100;
	XB3_DEBUG(this, "discovery time: %i\n", discovery_time_ms);

	uint16_t ms = 0;

	uv_queue_clear(&this->rx_at_queue);
	uv_xb3_local_at_cmd_req(this, "ND", "", 0);
	if (dev_count &&
			dev_max_count) {
		*dev_count = 0;
		// init first dev
		memset(dest, 0, sizeof(uv_xb3_nddev_st));

		uint16_t rx_count = 0;
		while (ms < discovery_time_ms) {
			uv_wdt_update();
			uv_xb3_at_response_e ret = uv_xb3_get_at_response(this);
			uint8_t r = 0;
			if (uv_queue_pop(&this->rx_at_queue, &r, 0) == ERR_NONE) {
				uv_xb3_nddev_st *nd = &dest[*dev_count];
				// copy data blindly in network byte order to dest
				((uint8_t*) nd)[rx_count] = r;
				rx_count++;
				if (rx_count >= 11 &&
						rx_count < 11 + 20 &&
						r == '\0') {
					// device name null termination received, jump to end of name
					rx_count = 11 + 21;
				}
				if (rx_count == 40) {
					// whole dev received, swap network byte order to host
					rx_count = 0;
					nd->my = ntouint16(nd->my);
					nd->serial = ntouint64(nd->serial);
					nd->parent_panid = ntouint16(nd->parent_panid);
					nd->profile_id = ntouint16(nd->profile_id);
					nd->manufacture_id = ntouint16(nd->manufacture_id);
					if (found_dev_callb) {
						found_dev_callb(*dev_count, nd);
					}
					(*dev_count)++;

					XB3_DEBUG(this, "    '%s' 16-bit id: 0x%x serial: 0x%08x%08x\n",
							nd->ni,
							(unsigned int) nd->my,
							(unsigned int) nd->sh,
							(unsigned int) nd->sl);

					if (*dev_count == dev_max_count) {
						break;
					}
					else {
						// initialize next dev
						memset(&dest[*dev_count], 0, sizeof(uv_xb3_nddev_st));
					}
				}
			}
			if (ret == XB3_AT_RESPONSE_OK) {
				this->at_response = XB3_AT_RESPONSE_COUNT;
			}
			else if (ret == XB3_AT_RESPONSE_ERROR) {
				XB3_DEBUG(this, "XB3 Network discovery ERROR\n");
				*dev_count = 0;
				break;
			}
			else {

			}
			ms += 1;
			if (xb3_step_task) {
				tx_rx(this, 1);
			}
			else {
				uv_rtos_task_delay(1);
			}
		}

		XB3_DEBUG(this, "Found %i devices\n",
				(int) *dev_count);
	}

	uv_mutex_unlock(&this->atreq_mutex);

	return this->at_response;
}



uint64_t uv_xb3_get_epid(uv_xb3_st *this, bool xb3_step_task) {
	uint64_t ret = 0;
	uv_mutex_lock(&this->atreq_mutex);
	uv_xb3_local_at_cmd_req(this, "ID", "", 0);
	if (at_wait_for_reply(this, 500, xb3_step_task) == XB3_AT_RESPONSE_OK) {
		ret = ntouint64_queue(this, &this->rx_at_queue);
	}
	else {

	}
	uv_mutex_unlock(&this->atreq_mutex);

	return ret;
}



void uv_xb3_network_reset(uv_xb3_st *this, bool xb3_step_task) {
	memset(&this->network, 0, sizeof(this->network));

	uv_mutex_lock(&this->atreq_mutex);

	char data[8] = {};
	// clear default extended PAN ID
	uv_xb3_local_at_cmd_req(this, "ID", data, 8);
	at_wait_for_reply(this, 500, xb3_step_task);
	// save to flash
	uv_xb3_local_at_cmd_req(this, "WR", "", 0);
	at_wait_for_reply(this, 500, xb3_step_task);
	// reset network
	uv_xb3_local_at_cmd_req(this, "NR", data, 1);
	at_wait_for_reply(this, 500, xb3_step_task);

	uv_mutex_unlock(&this->atreq_mutex);

	this->conf->epanid = 0;
	uv_memory_save();

	XB3_DEBUG(this, "XB3 network reset\n");
}


void uv_xb3_join_network(uv_xb3_st *this, uint64_t pan64, uint16_t pan16, uint8_t chn,
		bool xb3_step_task) {

}


void uv_xb3_leave_network(uv_xb3_st *this, bool xb3_step_task) {

}




void uv_xb3_terminal(uv_xb3_st *this,
		unsigned int args, argument_st *argv) {
	if (args && argv[0].type == ARG_STRING) {
		if (strcmp(argv[0].str, "scan") == 0) {
			printf("Starting XB3 active scan\n");
			uv_xb3_network_st networks[3] = {};
			uint8_t network_count = 0;
			uv_xb3_at_response_e ret =
					uv_xb3_scan_networks(this, &network_count, networks, 3, false);
			printf("Scan returned %i, Found %i networks\n",
					ret, network_count);
			for (uint8_t i = 0; i < network_count; i++) {
				printf("Network %i:\n"
						"    channel: %u\n"
						"    PAN ID16: 0x%x\n"
						"    PAN ID64: 0x%08x%08x\n"
						"    Allow join: %i\n"
						"    Stack profile: %u\n"
						"    LQI: %u\n"
						"    RSSI: %i\n",
						i + 1,
						networks[i].channel,
						networks[i].pan16,
						(unsigned int) (networks[i].pan64 >> 32),
						(unsigned int) (networks[i].pan64 & 0xFFFFFFFF),
						networks[i].allowjoin,
						networks[i].stackprofile,
						networks[i].lqi,
						networks[i].rssi);
			}
		}
		else if (strcmp(argv[0].str, "hex") == 0) {
			if (args > 1) {
				uv_xb3_set_at_echo_as_hex(this, argv[1].number);
			}
			printf("Display as hex: %u\n", !!(this->conf->flags & XB3_CONF_FLAGS_AT_HEX));
		}
		else if (strcmp(argv[0].str, "at") == 0 ||
				strcmp(argv[0].str, "AT") == 0) {
			if (args > 1 && argv[1].type == ARG_STRING) {
				char data[64] = "";
				uint16_t datalen = 0;
				if (args > 2) {
					if (argv[2].type == ARG_INTEGER) {
						// convert integer into string
						data[0] = argv[2].number & 0xFF;
						data[1] = (argv[2].number >> 8) & 0xFF;
						datalen = 2;
						if (args > 3) {
							data[0] = (argv[2].number >> 24) & 0xFF;
							data[1] = (argv[2].number >> 16) & 0xFF;
							data[2] = (argv[2].number >> 8) & 0xFF;
							data[3] = (argv[2].number >> 0) & 0xFF;
							data[4] = (argv[3].number >> 24) & 0xFF;
							data[5] = (argv[3].number >> 16) & 0xFF;
							data[6] = (argv[3].number >> 8) & 0xFF;
							data[7] = (argv[3].number >> 0) & 0xFF;
							datalen = 8;
						}
						else if (argv[2].number > UINT16_MAX) {
							data[0] = (argv[2].number >> 24) & 0xFF;
							data[1] = (argv[2].number >> 16) & 0xFF;
							data[2] = (argv[2].number >> 8) & 0xFF;
							data[3] = (argv[2].number >> 0) & 0xFF;
							datalen = 4;
						}
						else if (argv[2].number > UINT8_MAX) {
							// convert integer into string
							data[0] = (argv[2].number >> 8) & 0xFF;
							data[1] = (argv[2].number >> 0) & 0xFF;
							datalen = 2;
						}
						else {
							// convert integer into string
							data[0] = argv[2].number;
							datalen = 1;
						}
					}
					else {
						strcpy(data, argv[2].str);
						datalen = strlen(data);
					}
				}
				uv_xb3_local_at_cmd_req(this, argv[1].str, data, datalen);
			}
		}
		else if (strcmp(argv[0].str, "rxecho") == 0) {
			if (args > 1) {
				bool val = 0;
				if (argv[1].type == ARG_STRING) {
					if (strcmp(argv[1].str, "true") == 0 ||
							strcmp(argv[1].str, "1") == 0) {
						val = 1;
					}
				}
				else {
					val = argv[1].number;
				}
				if (val) {
					this->conf->flags |= XB3_CONF_FLAGS_RX_ECHO;
				}
				else {
					this->conf->flags &= ~XB3_CONF_FLAGS_RX_ECHO;
				}
			}
			printf("RX echo: %u\n", !!(this->conf->flags & XB3_CONF_FLAGS_RX_ECHO));
		}
		else if (strcmp(argv[0].str, "txecho") == 0) {
			if (args > 1) {
				bool val = 0;
				if (argv[1].type == ARG_STRING) {
					if (strcmp(argv[1].str, "true") == 0 ||
							strcmp(argv[1].str, "1") == 0) {
						val = 1;
					}
				}
				else {
					val = argv[1].number;
				}
				if (val) {
					this->conf->flags |= XB3_CONF_FLAGS_TX_ECHO;
				}
				else {
					this->conf->flags &= ~XB3_CONF_FLAGS_TX_ECHO;
				}
			}
			printf("TX echo: %u\n", !!(this->conf->flags & XB3_CONF_FLAGS_TX_ECHO));
		}
		else if (strcmp(argv[0].str, "atecho") == 0) {
			if (args > 1) {
				bool val = 0;
				if (argv[1].type == ARG_STRING) {
					if (strcmp(argv[1].str, "true") == 0 ||
							strcmp(argv[1].str, "1") == 0) {
						val = 1;
					}
				}
				else {
					val = argv[1].number;
				}
				if (val) {
					this->conf->flags |= XB3_CONF_FLAGS_AT_ECHO;
				}
				else {
					this->conf->flags &= ~XB3_CONF_FLAGS_AT_ECHO;
				}
			}
			printf("AT echo: %u\n", !!(this->conf->flags & XB3_CONF_FLAGS_AT_ECHO));
		}
		else if (strcmp(argv[0].str, "debug") == 0) {
			if (args > 1) {
				if (argv[1].number) {
					this->conf->flags |= XB3_CONF_FLAGS_DEBUG;
				}
				else {
					this->conf->flags &= ~XB3_CONF_FLAGS_DEBUG;
				}
			}
			printf("XB3 debug messages: %u\n",
					!!(this->conf->flags & XB3_CONF_FLAGS_DEBUG));
		}
		else if (strcmp(argv[0].str, "write") == 0) {
			if (args > 1) {
				printf("Writing to dev: 0x%08x%08x, data:'%s'\n",
						(unsigned int) (this->conf->dest_addr >> 32),
						(unsigned int) (this->conf->dest_addr & 0xFFFFFFFF),
						argv[1].str);
				uv_xb3_write(this, argv[1].str,
						strlen(argv[1].str));
			}
			else {
				printf("Give data to write as a second argument\n");
			}
		}
		else if (strcmp(argv[0].str, "destaddr") == 0) {
			if (args > 1) {
				uint64_t panid = strtol(argv[1].str, NULL, 0);
				if (panid == 0x7FFFFFFF) {
					char str1[11] = {};
					strncpy(str1, argv[1].str, 10);
					panid = ((uint64_t) strtol(str1, NULL, 0)) << 32;
					panid += strtol(&argv[1].str[10], NULL, 16);
				}
				this->conf->dest_addr = panid;
			}
			printf("Destination address: 0x%08x%08x\n",
					(unsigned int) (this->conf->dest_addr >> 32),
					(unsigned int) (this->conf->dest_addr & 0xFFFFFFFF));
		}
		else if (strcmp(argv[0].str, "reset") == 0) {
			uv_xb3_network_reset(this, false);
		}
		else if (strcmp(argv[0].str, "netdisc") == 0) {
			printf("Executing network discovery...\n");
			uv_xb3_nddev_st devs[3];
			uint8_t dev_count;
			uv_xb3_network_discovery(this, &dev_count, devs, 3, NULL, false);
		}
		else {
			printf("Unknown command '%s'\n", argv[0].str);
		}
	}
	else {
		printf("XB3 stat:\n"
				"   initialized: %i\n"
				"   network id: 0x%08x%08x\n"
				"   dest addr: 0x%08x%08x\n"
				"   debug: %u\n"
				"   RX echo: %u\n"
				"    TX echo: %u\n"
				"   AT echo: %u\n"
				"   AT as hex: %u\n"
				"    TX buf: %i/%i (max %i)\n"
				"    RX buf: %i/%i (max %i)\n"
				"    Max retransmit: %u\n",
				(int) this->initialized,
				(unsigned int) ((uint64_t) this->conf->epanid >> 32),
				(unsigned int) ((uint32_t) this->conf->epanid & 0xFFFFFFFF),
				(unsigned int) ((uint64_t) this->conf->dest_addr >> 32),
				(unsigned int) ((uint32_t) this->conf->dest_addr & 0xFFFFFFFF),
				!!(this->conf->flags & XB3_CONF_FLAGS_DEBUG),
				!!(this->conf->flags & XB3_CONF_FLAGS_RX_ECHO),
				!!(this->conf->flags & XB3_CONF_FLAGS_TX_ECHO),
				!!(this->conf->flags & XB3_CONF_FLAGS_AT_ECHO),
				!!(this->conf->flags & XB3_CONF_FLAGS_AT_HEX),
				(int) uv_streambuffer_get_len(&this->tx_streambuffer),
				(int) TX_BUF_SIZE,
				(int) this->tx_max,
				(int) uv_streambuffer_get_len(&this->rx_data_streambuffer),
				(int) RX_BUF_SIZE,
				(int) this->rx_max,
				this->max_retransmit);
		this->tx_max = 0;
		this->rx_max = 0;
		this->max_retransmit = 0;
	}
}



#endif
