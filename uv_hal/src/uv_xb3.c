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

#if CONFIG_SPI


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
#define APIFRAME_BLEUNLOCKRESPONSE			0xAC
#define APIFRAME_USERDATARELAYOUTPUT		0xAD
#define APIFRAME_SECURESESSIONRESPONSE		0xAE
#define APIFRAME_START						0x7E




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




/// @brief: Writes a local AT command request to device
/// according to frame type LOCALATCMDREQ
void uv_xb3_local_at_cmd_req(uv_xb3_st *this, char *atcmd, char *data, uint16_t data_len) {
	uv_mutex_lock(&this->tx_mutex);
	this->at_response = XB3_AT_RESPONSE_COUNT;
	uv_queue_clear(&this->rx_at_queue);
	uint32_t crc = 0;
	uint16_t paramvaluelen = data_len;
	if (data != NULL &&
			paramvaluelen == 0) {
		paramvaluelen = strlen(data);
	}
	uint8_t d = APIFRAME_START;
	uv_queue_push(&this->tx_queue, &d, 0);
	uint16_t framedatalen = 4 + paramvaluelen;
	d = (framedatalen >> 8); // length MSB
	uv_queue_push(&this->tx_queue, &d, 0);
	d = framedatalen & 0xFF; // length LSB
	uv_queue_push(&this->tx_queue, &d, 0);
	d = APIFRAME_LOCALATCMDREQ;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	// Frame ID, has to be something else than 0
	d = 0x17;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = atcmd[0];
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = atcmd[1];
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	for (uint16_t i = 0; i < paramvaluelen; i++) {
		uv_queue_push(&this->tx_queue, &data[i], 0);
		crc += (uint8_t) data[i];
	}
	d = 0xFF - (crc & 0xFF);
	uv_queue_push(&this->tx_queue, &d, 0);

	uv_mutex_unlock(&this->tx_mutex);
}


void uv_xb3_write_data(uv_xb3_st *this, uint64_t destaddr,
		char *data, uint16_t datalen) {
	uv_mutex_lock(&this->tx_mutex);
	uint32_t crc = 0;
	uint8_t d = APIFRAME_START;
	uv_queue_push(&this->tx_queue, &d, 0);
	uint16_t framedatalen = 14 + datalen;
	// Length
	d = (framedatalen >> 8);
	uv_queue_push(&this->tx_queue, &d, 0);
	d = framedatalen & 0xFF;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = APIFRAME_TRANSMITREQ;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	// 64-bit address
	d = (destaddr >> 56) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = (destaddr >> 48) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = (destaddr >> 40) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = (destaddr >> 32) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = (destaddr >> 24) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = (destaddr >> 16) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = (destaddr >> 8) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = (destaddr) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	// 16-bit adddress
	d = 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	d = 0xFE;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	// broadcast radius
	d = 0;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	// transmit options
	d = 0;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	for (uint16_t i = 0; i < datalen; i++) {
		d = data[i];
		crc += d;
		uv_queue_push(&this->tx_queue, &d, 0);
	}
	d = 0xFF - crc;
	uv_queue_push(&this->tx_queue, &d, 0);

	uv_mutex_unlock(&this->tx_mutex);
}





uv_errors_e uv_xb3_init(uv_xb3_st *this,
		uv_xb3_conf_st *conf,
		spi_e spi,
		uv_gpios_e ssel_gpio,
		uv_gpios_e spi_attn_gpio,
		uv_gpios_e reset_gpio,
		const char *nodeid) {
	uv_errors_e ret = ERR_NONE;
	this->conf = conf;
	this->initialized = false;
	this->spi = spi;
	this->attn_gpio = spi_attn_gpio;
	this->reset_gpio = reset_gpio;
	this->ssel_gpio = ssel_gpio;
	this->at_echo = false;
	this->at_response = XB3_AT_RESPONSE_COUNT;
	this->rx_index = 0;
	this->rx_size = 0;
	this->modem_status = 0;
	if (uv_queue_init(&this->tx_queue, 100, sizeof(char)) == NULL) {
		uv_terminal_enable(TERMINAL_CAN);
		printf("XB3: Creating TX queue failed\n");
		ret = ERR_NOT_ENOUGH_MEMORY;
	}
	if (uv_queue_init(&this->rx_data_queue, 100, sizeof(char)) == NULL) {
		uv_terminal_enable(TERMINAL_CAN);
		printf("XB3: Creating RX data queue failed\n");
		ret = ERR_NOT_ENOUGH_MEMORY;
	}
	if (uv_queue_init(&this->rx_at_queue, 50, sizeof(char)) == NULL) {
		uv_terminal_enable(TERMINAL_CAN);
		printf("XB3: Creating RX AT queue failed\n");
		ret = ERR_NOT_ENOUGH_MEMORY;
	}

	uv_mutex_init(&this->tx_mutex);
	uv_mutex_unlock(&this->tx_mutex);

	uv_gpio_init_input(this->attn_gpio, PULL_UP_ENABLED);

	uv_terminal_enable(TERMINAL_CAN);
	printf("Resetting XB3\n");

	// reset the XB3
	uv_gpio_init_output(this->reset_gpio, false);
	uv_rtos_task_delay(20);
	uv_gpio_set(this->reset_gpio, true);
	uv_rtos_task_delay(20);

	uv_gpio_init_output(this->ssel_gpio, false);
	uint16_t ms = 0;
	while (uv_gpio_get(this->attn_gpio)) {
		uv_wdt_update();
		uv_rtos_task_delay(10);
		ms += 10;
		if (ms > 5000) {
			ret = ERR_NACK;
			break;
		}
	}
	uv_gpio_set(this->ssel_gpio, true);

	uv_rtos_task_delay(1);

	// SMD devices enter SPI mode by asserting SSEL.
	// We do this by reading from XB3

	// set AO to 0, zigee data format
	uv_xb3_local_at_cmd_req(this, "AO", "0", 1);

	printf("XB3 initialized, took %i ms\n", ms);

	if (ret == ERR_NONE) {
		this->initialized = true;
	}

	printf("Setting device identification string to '%s'\n", nodeid);
	uv_xb3_set_nodename(this, nodeid);


	return ret;
}


uv_errors_e uv_xb3_set_nodename(uv_xb3_st *this, const char *name) {
	uv_errors_e ret = ERR_NONE;
	uv_xb3_local_at_cmd_req(this, "NI", (char*) name, strlen(name));
	while (uv_xb3_get_at_response(this) == XB3_AT_RESPONSE_COUNT) {
		uv_xb3_poll(this);
	}
	if (uv_xb3_get_at_response(this) != XB3_AT_RESPONSE_OK) {
		ret = ERR_ABORTED;
	}
	return ret;
}



void uv_xb3_poll(uv_xb3_st *this) {
	if (this->initialized) {
		// read and write to XB3
		spi_data_t tx = 0;

		if ((uv_queue_pop(&this->tx_queue, &tx, 0) == ERR_NONE) ||
				!uv_gpio_get(this->attn_gpio)) {
			spi_data_t rx = 0;
			uv_gpio_set(this->ssel_gpio, false);
			uv_spi_readwrite_sync(this->spi, SPI_SLAVE_NONE, &tx, &rx, 8, 1);

			if (this->rx_index == 0) {
				if (rx == APIFRAME_START) {
					this->rx_index++;
					this->rx_size = 0;
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
								if (this->at_echo) {
									if (this->at_echo_hex) {
										printf("0x%02x ", rx);
									}
									else {
										printf("%c", rx);
									}
								}
								if (uv_queue_push(&this->rx_at_queue, &rx, 0) != ERR_NONE) {
									printf("XB3: AT rx queue full\n");
								}
							}
							else {

							}
							if (this->rx_index - 4 == this->rx_size - 1) {
								// last byte of this response, update AT response status
								if (this->at_echo) {
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
//						case APIFRAME_16RECEIVE: {
//							if (uv_queue_push(&this->rx_data_queue, &rx, 0) != ERR_NONE) {
//								printf("XB3: RX data queue full\n");
//							}
//							break;
//						}
						case APIFRAME_MODEMSTATUS:
							if (offset == 4) {
								this->modem_status = rx;
								printf("MODEMSTATUS 0x%x '%s'\n", rx,
										uv_xb3_modem_status_to_str(rx));
							}
							break;
						default:
							printf("APIFRAME 0x%x\n", this->rx_frame_type);
							break;
						}
					}
					else {
						// last byte is CRC, which marks the end of transmission
						this->rx_index = 0;
						if (this->at_echo &&
								this->rx_frame_type == APIFRAME_LOCALATCMDRESPONSE) {
						}
					}
					break;
				}
			}
		}
		else {
			uv_gpio_set(this->ssel_gpio, true);
		}
	}
}




uv_xb3_at_response_e uv_xb3_scan_devs(uv_xb3_st *this,
		uint8_t *dev_count,
		uv_xb3_dev_st *dest,
		uint8_t dev_max_count) {

	uv_xb3_local_at_cmd_req(this, "AS", "", 0);
	if (dev_count) {
		*dev_count = 0;
	}
	for (uint8_t i = 0; i < dev_max_count; i++) {
		// wait until AT response is received
		while (uv_xb3_get_at_response(this) == XB3_AT_RESPONSE_COUNT) {
			uv_rtos_task_delay(1);
		}

		uv_enter_critical();

		bool br = false;

		uint8_t data = 0;
		uv_xb3_dev_st d;
		if (uv_xb3_get_at_response(this) != XB3_AT_RESPONSE_OK ||
				uv_queue_pop(&this->rx_at_queue, &data, 0) != ERR_NONE) {
			// no data received, it means scan has ended
			br = true;
		}
		else if (data == 0x2) {

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
			(*dev_count)++;
		}
		else {

		}
		uv_exit_critical();
		if (br) {
			break;
		}
	}


	return uv_xb3_get_at_response(this);
}


#endif
