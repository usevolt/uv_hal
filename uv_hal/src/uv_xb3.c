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

#if CONFIG_SPI && CONFIG_XB3


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



#define JOINWINDOW_DELAY_MS					200



/// @brief: Converts uint64_t data from network byte order to local byte order.
/// Use this for data received from XB3. Reads 8 bytes from *srcqueue*.
static uint64_t ntouint64(uv_xb3_st *this, uv_queue_st *srcqueue) {
	uint64_t ret = 0;
	uint8_t data = 0;
	while (this->at_response != XB3_AT_RESPONSE_OK) {
		uv_rtos_task_delay(1);
	}
	uv_queue_pop(srcqueue, &data, 0);
	ret |= ((uint64_t) data << 56);
	uv_queue_pop(srcqueue, &data, 0);
	ret |= ((uint64_t) data << 48);
	uv_queue_pop(srcqueue, &data, 0);
	ret |= ((uint64_t) data << 40);
	uv_queue_pop(srcqueue, &data, 0);
	ret |= ((uint64_t) data << 32);
	uv_queue_pop(srcqueue, &data, 0);
	ret |= ((uint64_t) data << 24);
	uv_queue_pop(srcqueue, &data, 0);
	ret |= ((uint64_t) data << 16);
	uv_queue_pop(srcqueue, &data, 0);
	ret |= ((uint64_t) data << 8);
	uv_queue_pop(srcqueue, &data, 0);
	ret |= ((uint64_t) data << 0);

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

	if (this->conf->flags & XB3_CONF_FLAGS_AT_ECHO) {
		printf("AT %s ", atcmd);
		for (uint16_t i = 0; i < data_len; i++) {
			if (this->conf->flags & XB3_CONF_FLAGS_AT_HEX) {
				printf("0x%x ", data[i]);
			}
			else {
				printf("%c", data[i]);
			}
		}
		printf("\n");
	}

	uv_mutex_unlock(&this->tx_mutex);
}



void uv_xb3_write(uv_xb3_st *this, char *data, uint16_t datalen) {
	if (this->conf->flags & XB3_CONF_FLAGS_OPERATE_AS_COORDINATOR) {
		// todo: write data to all end-devices connected to us
	}
	else {
		// writing to address 0 writes data to coordinator
		uv_xb3_write_data_to_addr(this, 0, data, datalen);
	}
}


void uv_xb3_write_data_to_addr(uv_xb3_st *this, uint64_t destaddr,
		char *data, uint16_t datalen) {
	uv_mutex_lock(&this->tx_mutex);
	uint32_t crc = 0;
	uint8_t d = APIFRAME_START;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	uint16_t framedatalen = 14 + datalen;
	// Length
	d = (framedatalen >> 8);
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = framedatalen & 0xFF;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = APIFRAME_TRANSMITREQ;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	// Frame ID. 0x0 doesn't emit response frame
	d = 0x52;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	// 64-bit address
	d = (destaddr >> 56) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = (destaddr >> 48) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = (destaddr >> 40) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = (destaddr >> 32) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = (destaddr >> 24) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = (destaddr >> 16) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = (destaddr >> 8) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = (destaddr) & 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	// 16-bit adddress
	d = 0xFF;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	d = 0xFE;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	// broadcast radius
	d = 0;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	// transmit options
	d = 0;
	crc += d;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x ", d);
	for (uint16_t i = 0; i < datalen; i++) {
		d = data[i];
		crc += d;
		uv_queue_push(&this->tx_queue, &d, 0);
		printf("0x%x ", d);
	}
	d = 0xFF - crc;
	uv_queue_push(&this->tx_queue, &d, 0);
	printf("0x%x \n", d);

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
	this->at_response = XB3_AT_RESPONSE_COUNT;
	this->rx_index = 0;
	this->rx_size = 0;
	this->epanid = 0;
	this->modem_status = XB3_MODEMSTATUS_NONE;
	this->modem_status_changed = XB3_MODEMSTATUS_NONE;
	// at ehco defaults to false, sending commands via terminal enables AT echo
	uv_xb3_set_at_echo(this, false);

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

	uv_mutex_init(&this->atreq_mutex);
	uv_mutex_unlock(&this->atreq_mutex);

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
		if (ms > 1000) {
			ret = ERR_NACK;
			break;
		}
	}
	if (ret != ERR_NACK) {
		uv_gpio_set(this->ssel_gpio, true);

		uv_rtos_task_delay(1);

		// SMD devices enter SPI mode by asserting SSEL.
		// We do this by reading from XB3

		// set AO to 0, zigee data format
		uv_xb3_local_at_cmd_req(this, "AO", "0", 1);

		printf("XB3 initialized, took %i ms\n", ms);

		this->initialized = true;

		printf("Setting device identification string to '%s'\n", nodeid);
		uv_xb3_set_nodename(this, nodeid);
	}

	return ret;
}




uv_errors_e uv_xb3_set_nodename(uv_xb3_st *this, const char *name) {
	uv_errors_e ret = ERR_NONE;
	uv_mutex_lock(&this->atreq_mutex);
	uv_xb3_local_at_cmd_req(this, "NI", (char*) name, strlen(name));
	while (uv_xb3_get_at_response(this) == XB3_AT_RESPONSE_COUNT) {
		uv_xb3_poll(this);
	}
	if (uv_xb3_get_at_response(this) != XB3_AT_RESPONSE_OK) {
		ret = ERR_ABORTED;
	}
	uv_mutex_unlock(&this->atreq_mutex);
	return ret;
}



void uv_xb3_step(uv_xb3_st *this, uint16_t step_ms) {
	if (this->initialized) {
		uv_enter_critical();
		uv_xb3_modem_status_e modem_status_changed = this->modem_status_changed;
		this->modem_status_changed = XB3_MODEMSTATUS_NONE;
		uv_exit_critical();

		// wait until network is created or device is booted
		if (modem_status_changed == XB3_MODEMSTATUS_POWERUP) {
			this->epanid = uv_xb3_get_epid(this);
			uv_mutex_lock(&this->atreq_mutex);

			if (this->conf->flags & XB3_CONF_FLAGS_OPERATE_AS_COORDINATOR) {
				if (this->epanid == 0) {
					// operate as COORDINATOR
					char data[8] = {};
					data[0] = 1;
					uv_xb3_local_at_cmd_req(this, "CE", data, 1);
					while (this->at_response == XB3_AT_RESPONSE_COUNT) {
						uv_rtos_task_delay(1);
					}

					// Node join time is infinite
					data[0] = 0xFF;
					uv_xb3_local_at_cmd_req(this, "NJ", data, 1);
					while (this->at_response == XB3_AT_RESPONSE_COUNT) {
						uv_rtos_task_delay(1);
					}
				}
			}
			uv_mutex_unlock(&this->atreq_mutex);
		}
		else if (modem_status_changed == XB3_MODEMSTATUS_COORDINATORSTARTED) {
			uv_mutex_lock(&this->atreq_mutex);
			if (this->epanid == 0) {
				// "OP" contains random EPID that we copy to ID to keep
				// constant EPID in future
				uv_xb3_local_at_cmd_req(this, "OP", "", 0);
				this->epanid = ntouint64(this, &this->rx_at_queue);
				if (this->epanid) {
					// write modifications to nonvol memory on XB3
					uv_xb3_local_at_cmd_req(this, "WR", "", 0);
					while (this->at_response == XB3_AT_RESPONSE_COUNT) {
						uv_rtos_task_delay(1);
					}
					printf("XB3: Wrote configuration to nonvol memory\n");
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
			if (!(this->conf->flags & XB3_CONF_FLAGS_OPERATE_AS_COORDINATOR)) {
				uv_xb3_local_at_cmd_req(this, "OP", "", 0);
				while (this->at_response == XB3_AT_RESPONSE_COUNT) {
					uv_rtos_task_delay(1);
				}
				uint64_t id = ntouint64(this, &this->rx_at_queue);
				if (id != this->epanid) {
					char data[8];
					this->epanid = id;
					uint64ton(data, this->epanid);
					uv_xb3_local_at_cmd_req(this, "ID", data, 8);
					while (this->at_response == XB3_AT_RESPONSE_COUNT) {
						uv_rtos_task_delay(1);
					}
					uv_xb3_local_at_cmd_req(this, "WR", "", 0);
					while (this->at_response == XB3_AT_RESPONSE_COUNT) {
						uv_rtos_task_delay(1);
					}
					printf("XB3: Wrote configuration to nonvol memory\n");
				}
			}
			uv_mutex_unlock(&this->atreq_mutex);
		}
		else {

		}
	}
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
								if (this->conf->flags & XB3_CONF_FLAGS_AT_ECHO) {
									if (this->conf->flags & XB3_CONF_FLAGS_AT_HEX) {
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
								if (this->conf->flags & XB3_CONF_FLAGS_AT_ECHO) {
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
								printf("MODEMSTATUS 0x%x '%s'\n", rx,
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
								if (uv_queue_push(&this->rx_data_queue, &rx, 0)
										!= ERR_NONE) {
									printf("XB3: RX data queue full\n");
								}
								if (this->conf->flags & XB3_CONF_FLAGS_RX_ECHO) {
									printf("%c", rx);
								}
							}
							break;
						case APIFRAME_EXTTRANSMITSTATUS:
							if (offset == 8) {
								if (this->conf->flags & XB3_CONF_FLAGS_RX_ECHO) {
									if (rx) {
										printf("XB3 TRANSMIT ");
										if (rx == 1) {
											printf("MAC ACK fail\n");
										}
										else if (rx == 2) {
											printf("CCA/LBT fail\n");
										}
										else {
											printf("Indirect message requested\n");
										}
									}
								}
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



uv_xb3_at_response_e uv_xb3_scan_networks(uv_xb3_st *this,
		uint8_t *network_count,
		uv_xb3_network_st *dest,
		uint8_t network_max_count) {

	uv_mutex_lock(&this->atreq_mutex);

	uv_xb3_local_at_cmd_req(this, "AS", "", 0);
	if (network_count) {
		*network_count = 0;
	}
	for (uint8_t i = 0; i < network_max_count; i++) {
		// wait until AT response is received
		while (uv_xb3_get_at_response(this) == XB3_AT_RESPONSE_COUNT) {
			uv_rtos_task_delay(1);
		}

		uv_enter_critical();

		bool br = false;

		uint8_t data = 0;
		uv_xb3_network_st d;
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
			(*network_count)++;
		}
		else {

		}
		uv_exit_critical();
		if (br) {
			break;
		}
	}

	uv_xb3_at_response_e ret = uv_xb3_get_at_response(this);

	uv_mutex_unlock(&this->atreq_mutex);

	return ret;
}



uv_xb3_at_response_e uv_xb3_node_discovery(uv_xb3_st *this,
		uint8_t *dev_count,
		uv_xb3_dev_st *dest,
		uint8_t dev_max_count) {
	uv_mutex_lock(&this->atreq_mutex);

	uv_xb3_local_at_cmd_req(this, "ND", "", 0);
	if (dev_count) {
		*dev_count = 0;
	}

	uv_xb3_at_response_e ret = uv_xb3_get_at_response(this);
	uv_mutex_unlock(&this->atreq_mutex);

	return ret;
}



uint64_t uv_xb3_get_epid(uv_xb3_st *this) {
	uint64_t ret = 0;
	uv_mutex_lock(&this->atreq_mutex);
	uv_xb3_local_at_cmd_req(this, "ID", "", 0);
	while (uv_xb3_get_at_response(this) == XB3_AT_RESPONSE_COUNT) {
		uv_rtos_task_delay(1);
	}
	if (uv_xb3_get_at_response(this) == XB3_AT_RESPONSE_OK) {
		ret = ntouint64(this, &this->rx_at_queue);
	}
	else {

	}
	uv_mutex_unlock(&this->atreq_mutex);

	return ret;
}


void uv_xb3_terminal(uv_xb3_st *this,
		unsigned int args, argument_st *argv) {
	if (args && argv[0].type == ARG_STRING) {
		if (strcmp(argv[0].str, "scan") == 0) {
			printf("Starting XB3 active scan\n");
			uv_xb3_network_st networks[3] = {};
			uint8_t network_count = 0;
			uv_xb3_at_response_e ret =
					uv_xb3_scan_networks(this, &network_count, networks, 3);
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
				uv_xb3_set_at_echo(this, true);
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
		else if (strcmp(argv[0].str, "write") == 0) {
			if (args > 1) {
				if (args > 2) {
					if (argv[1].type != ARG_STRING) {
						printf("Argument 2 should be string "
								"defining the extended PAN ID\n");
					}
					else if (strncmp(argv[1].str, "0x", 2) != 0) {
						printf("Argument 2 should be hexadecimal string, "
								"starting with '0x'\n");
					}
					else {
						uint64_t panid = strtol(argv[1].str, NULL, 0);
						if (panid == 0x7FFFFFFF) {
							char str1[11] = {};
							strncpy(str1, argv[1].str, 10);
							panid = ((uint64_t) strtol(str1, NULL, 0)) << 32;
							panid += strtol(&argv[1].str[10], NULL, 16);
						}
						printf("Writing to dev: 0x%08x%08x, data:'%s'\n",
								(unsigned int) ((uint64_t) panid >> 32),
								(unsigned int) (panid & 0xFFFFFFFF),
								argv[2].str);
						uv_xb3_write_data_to_addr(this, panid, argv[2].str,
								strlen(argv[2].str));
					}
				}
			}
			else {
				printf("Give data to write as a second argument\n");
			}
		}
		else if (strcmp(argv[0].str, "stat") == 0) {
			printf("XB3 stat:\n"
					"   id: 0x%04x%04x\n"
					"   RX echo: %u\n"
					"   AT echo: %u\n"
					"   AT as hex: %u\n",
					(unsigned int) ((uint64_t) this->epanid >> 32),
					(unsigned int) ((uint32_t) this->epanid & 0xFFFFFFFF),
					!!(this->conf->flags & XB3_CONF_FLAGS_RX_ECHO),
					!!(this->conf->flags & XB3_CONF_FLAGS_AT_ECHO),
					!!(this->conf->flags & XB3_CONF_FLAGS_AT_HEX));
		}
		else {

		}
	}
}



#endif
