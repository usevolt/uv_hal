/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */


#include "uv_can.h"

#if CONFIG_CAN

#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif

#include "uv_gpio.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#if CONFIG_TERMINAL_CAN
#include "uv_terminal.h"
#include "uv_memory.h"
#include "uv_uart.h"
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <PCANBasic.h>


#define DEV_COUNT_MAX			30

typedef struct {
	bool connection;
	unsigned int baudrate;
	char dev[32];
	TPCANHandle handle;
	uv_can_message_st rx_buffer_data[CONFIG_CAN0_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	uv_can_message_st tx_buffer_data[CONFIG_CAN0_TX_BUFFER_SIZE];
	uv_ring_buffer_st tx_buffer;
	struct timeval lastrxtime;

	// list of available CAN devices
	char devs[DEV_COUNT_MAX][32];
	// count of CAN devs
	int32_t dev_count;

} can_st;


static can_st _can = {
		.connection = false,
		.baudrate = CONFIG_CAN0_BAUDRATE,
		.dev = "can0",
		.dev_count = 0
};
#define this (&_can)



char *uv_can_get_device_name(int32_t i) {
	if (i < this->dev_count) {
		return this->devs[i];
	}
	else {
		return NULL;
	}
}

/// @brief: Returns the count of found CAN interface devices
int32_t uv_can_get_device_count(void) {
	return this->dev_count;
}




#if CONFIG_TARGET_WIN

char *uv_can_set_up(void) {
	char *ret = NULL;

	if (this->connection) {
		uv_can_close();
	}

	/* open socket */
	this->handle = PCAN_USBBUS1;
	int32_t baudrate;
	bool valid_baudrate = true;
	switch(this->baudrate) {
	case 100000:
		baudrate = PCAN_BAUD_100K;
		break;
	case 125000:
		baudrate = PCAN_BAUD_125K;
		break;
	case 250000:
		baudrate = PCAN_BAUD_250K;
		break;
	case 500000:
		baudrate = PCAN_BAUD_500K;
		break;
	case 1000000:
		baudrate = PCAN_BAUD_1M;
		break;
	default:
		baudrate = 0;
		valid_baudrate = false;
		break;
	}
	if (!valid_baudrate) {
		ret = "Invalid baudrate. Baudrate has to be 100k, 125k, 250k, 500k or 1M.";
	}
	else {
		TPCANStatus st = CAN_Initialize(this->handle, baudrate, 0, 0, 0);

		if (st != PCAN_ERROR_OK) {
			ret = "Error when connecting to PCAN hardware. Check that PCAN-USB is"
					" connected and the driver installed.";
		}
		else {
			this->connection = true;
		}
	}

	return ret;
}


void uv_can_close(void) {
	if (this->connection) {
		CAN_Uninitialize(this->handle);
		this->connection = false;
	}
}


bool uv_can_set_baudrate(uv_can_channels_e channel, unsigned int baudrate) {
	bool ret = true;
	this->baudrate = baudrate;
	strcpy(this->dev, channel);

	// find out the names of network interfaces



	return ret;
}


struct timeval uv_can_get_rx_time(void) {
	return this->lastrxtime;
}

bool uv_can_is_connected(void) {
	return this->connection;
}


#endif


uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {

	// SocketCAN doesn't support message filtering, this function does nothing.

	return ERR_NONE;
}


uv_errors_e _uv_can_init() {
	uv_errors_e ret = ERR_NONE;

	// CAN is already initialized. Connection will be opened when baudrate is set.
	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			sizeof(this->rx_buffer_data) / sizeof(this->rx_buffer_data[0]),
			sizeof(this->rx_buffer_data[0]));

	return ret;
}



#if CONFIG_TERMINAL_CAN
uv_errors_e uv_can_get_char(char *dest) {
	return uv_ring_buffer_pop(&this->char_buffer, dest);
}
#endif



uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	uv_errors_e ret = ERR_NONE;

	if (this->connection) {
	}
	else {
		uv_can_set_up();
	}

	TPCANMsg msg;
	msg.ID = message->id;
	msg.LEN = message->data_length;
	msg.MSGTYPE = (message->type == CAN_STD) ? PCAN_MESSAGE_STANDARD : PCAN_MESSAGE_EXTENDED;
	memcpy(msg.DATA, message->data_8bit, message->data_length);
	CAN_Write(this->handle, &msg);

	return ret;
}



uv_errors_e uv_can_pop_message(uv_can_channels_e channel, uv_can_message_st *message) {
	uv_errors_e ret = uv_ring_buffer_pop(&this->rx_buffer, message);
	return ret;
}



uv_errors_e uv_can_reset(uv_can_channels_e channel) {
	return ERR_NONE;
}



/// @brief: Inner hal step function which is called in rtos hal task
void _uv_can_hal_step(unsigned int step_ms) {
	if (this->connection) {
		bool go = true;
		while (go) {
			if (this->connection) {
				TPCANMsg msg;
				TPCANStatus st = CAN_Read(this->handle, &msg, NULL);
				while (st != PCAN_ERROR_QRCVEMPTY) {
					uv_can_msg_st m;
					bool known_msg = true;
					if (msg.MSGTYPE == PCAN_MESSAGE_STANDARD) {
						m.type = CAN_STD;
					}
					else if (msg.MSGTYPE == PCAN_MESSAGE_EXTENDED) {
						m.type = CAN_EXT;
					}
					else if (msg.MSGTYPE == PCAN_MESSAGE_ERRFRAME) {
						m.type = CAN_ERR;
					}
					else {
						known_msg = false;
					}

					if (known_msg) {
						m.id = msg.ID;
						m.data_length = msg.LEN;
						memcpy(m.data_8bit, msg.DATA, msg.LEN);

						uv_ring_buffer_push(&this->rx_buffer, &m);
					}

					st = CAN_Read(this->handle, &msg, NULL);
				}

			}

			go = false;

		}
	}
}


void uv_can_clear_rx_buffer(uv_can_channels_e channel) {
	uv_ring_buffer_clear(&this->rx_buffer);
}



#endif
