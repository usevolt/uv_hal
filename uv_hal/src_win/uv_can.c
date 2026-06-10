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
#include <stdlib.h>
#include <sys/time.h>
#include <PCANBasic.h>


#if !defined(PRINT)
#define PRINT(...) printf(__VA_ARGS__)
#endif

#if CONFIG_CAN_LOG
extern bool can_log;
#endif

#define DEV_COUNT_MAX			30

// On Windows the PCAN-Basic driver always talks to the first PCAN-USB channel.
#define CAN_PCAN_CHANNEL		PCAN_USBBUS1

typedef enum {
	CAN_STATE_INIT = 0,
	CAN_STATE_OPEN,
	CAN_STATE_FAULT
} can_state_e;

typedef struct {
	can_state_e state;
	unsigned int baudrate;
	// PCAN hardware channel handle
	TPCANHandle handle;
	char dev[32];
	uv_can_message_st rx_buffer_data[CONFIG_CAN0_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	uv_can_message_st tx_buffer_data[CONFIG_CAN0_TX_BUFFER_SIZE];
	uv_ring_buffer_st tx_buffer;
	struct timeval lastrxtime;

	// list of available CAN devices
	char devs[DEV_COUNT_MAX][32];
	// count of CAN devs
	int32_t dev_count;

	bool (*rx_callback)(void *user_ptr, uv_can_msg_st *msg);
	bool (*tx_callb)(void *user_ptr, uv_can_msg_st *msg, can_send_flags_e flags);

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
#endif

} can_st;


static can_st _can = {
		.state = CAN_STATE_INIT,
		.baudrate = 250000,
		.dev = "PCAN_USBBUS1",
		.handle = CAN_PCAN_CHANNEL,
		.dev_count = 0,
		.rx_callback = NULL,
		.tx_callb = NULL
};
#define this (&_can)


void uv_can_set_rx_msg_callbacks(void (*config_callb)(uv_can_channels_e chn,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type),
		void (*clear_callb)(uv_can_channels_e chn)) {

}


static bool cclose(void);
static bool copen(void);


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


void uv_can_set_dev(uv_can_channels_e can_dev) {
	strcpy(this->dev, can_dev);
}


uv_can_channels_e uv_can_get_dev(void) {
	return (uv_can_channels_e) this->dev;
}


/// @brief: Translates a numeric baudrate to a PCAN-Basic baudrate register value.
///
/// @return: true if the baudrate was valid
static bool baudrate_to_pcan(unsigned int baudrate, TPCANBaudrate *dest) {
	bool ret = true;
	switch (baudrate) {
	case 100000:
		*dest = PCAN_BAUD_100K;
		break;
	case 125000:
		*dest = PCAN_BAUD_125K;
		break;
	case 250000:
		*dest = PCAN_BAUD_250K;
		break;
	case 500000:
		*dest = PCAN_BAUD_500K;
		break;
	case 1000000:
		*dest = PCAN_BAUD_1M;
		break;
	default:
		*dest = PCAN_BAUD_250K;
		ret = false;
		break;
	}
	return ret;
}


/// @brief: Opens the connection to the PCAN-USB hardware
static bool copen(void) {
	bool ret = true;

	if (this->state == CAN_STATE_OPEN) {
		cclose();
	}

	TPCANBaudrate baudrate;
	if (!baudrate_to_pcan(this->baudrate, &baudrate)) {
		PRINT("Invalid baudrate %u. Baudrate has to be 100k, 125k, 250k, 500k or 1M.\n",
				this->baudrate);
		ret = false;
	}
	else {
		TPCANStatus st = CAN_Initialize(this->handle, baudrate, 0, 0, 0);
		if (st != PCAN_ERROR_OK) {
			PRINT("Error when connecting to PCAN hardware. Check that PCAN-USB is "
					"connected and the driver installed.\n");
			ret = false;
		}
		else {
			PRINT("CAN connection opened to PCAN channel 0x%x\n", this->handle);
			this->state = CAN_STATE_OPEN;
		}
	}

	return ret;
}


/// @brief: Closes the connection to the PCAN-USB hardware
static bool cclose(void) {
	bool ret = true;

	if (this->state == CAN_STATE_OPEN) {
		CAN_Uninitialize(this->handle);
		PRINT("CAN connection closed.\n");
	}

	this->state = CAN_STATE_INIT;
	return ret;
}

void uv_can_close(void) {
	cclose();
}

uv_errors_e uv_can_add_tx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg,
				can_send_flags_e flags)) {
	this->tx_callb = callback_function;

	return ERR_NONE;
}


char *uv_can_set_up(bool force_set_up) {
	char *ret = NULL;

	// the connection is (re)opened if it is not yet open, or if a forced
	// set up was requested.
	if (this->state != CAN_STATE_OPEN ||
			force_set_up) {
		if (!copen()) {
			ret = "Couldn't open the connection to the PCAN hardware.";
			this->state = CAN_STATE_FAULT;
		}
	}

	return ret;
}


bool uv_can_set_baudrate(uv_can_channels_e channel, unsigned int baudrate) {
	bool ret = true;
	this->baudrate = baudrate;
	strcpy(this->dev, channel);

	// PCAN-Basic does not enumerate the channels the way SocketCAN does. Expose
	// the single PCAN-USB channel that is used so that the device listing APIs
	// have at least one entry.
	this->dev_count = 0;
	strcpy(this->devs[this->dev_count++], "PCAN_USBBUS1");

	return ret;
}


unsigned int uv_can_get_baudrate(uv_can_channels_e channel) {
	return this->baudrate;
}


struct timeval uv_can_get_rx_time(void) {
	return this->lastrxtime;
}

bool uv_can_is_connected(void) {
	return (this->state == CAN_STATE_OPEN);
}


uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {

	// PCAN-Basic receives all messages, this function does nothing.

	return ERR_NONE;
}


uv_errors_e uv_can_config_rx_message_no_callb(
		uv_can_channels_e chn,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {

	return ERR_NONE;
}


uv_errors_e _uv_can_init() {
	uv_errors_e ret = ERR_NONE;

	// CAN is already initialized. Connection will be opened when baudrate is set.
	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			sizeof(this->rx_buffer_data) / sizeof(this->rx_buffer_data[0]),
			sizeof(this->rx_buffer_data[0]));

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_init(&this->char_buffer, this->char_buffer_data,
			CONFIG_TERMINAL_BUFFER_SIZE, sizeof(char));
#if !CONFIG_CANOPEN
	uv_can_config_rx_message(CAN0, UV_TERMINAL_CAN_RX_ID + uv_get_id(),
			CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
#endif
	this->rx_callback = NULL;

	return ret;
}



#if CONFIG_TERMINAL_CAN
uv_errors_e uv_can_get_char(char *dest) {
	return uv_ring_buffer_pop(&this->char_buffer, dest);
}
#endif


uv_errors_e uv_can_add_rx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg)) {
	this->rx_callback = callback_function;

	return ERR_NONE;
}



static uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	uv_errors_e ret = ERR_NONE;

	if (this->state == CAN_STATE_INIT) {
		uv_can_set_up(false);
	}

	if (this->state == CAN_STATE_OPEN) {
		TPCANMsg msg;
		msg.ID = message->id;
		msg.LEN = message->data_length;
		msg.MSGTYPE = (message->type == CAN_EXT) ?
				PCAN_MESSAGE_EXTENDED : PCAN_MESSAGE_STANDARD;
		memcpy(msg.DATA, message->data_8bit, message->data_length);

		TPCANStatus st = CAN_Write(this->handle, &msg);
		if (st != PCAN_ERROR_OK) {
			PRINT("Sending a message with ID of 0x%x resulted in a CAN error: 0x%x\n",
					message->id, (unsigned int) st);
			ret = ERR_HARDWARE_NOT_SUPPORTED;
		}
	}

	return ret;
}



uv_errors_e uv_can_send_flags(uv_can_channels_e chn, uv_can_msg_st *msg,
		can_send_flags_e flags) {
	uv_errors_e ret = ERR_NONE;
	uv_disable_int();
	if (flags & CAN_SEND_FLAGS_LOCAL) {
		ret = uv_ring_buffer_push(&this->rx_buffer, msg);
	}
	if ((flags & CAN_SEND_FLAGS_SYNC) ||
			(flags & CAN_SEND_FLAGS_NORMAL)) {
		ret = uv_can_send_message(chn, msg);
	}
	if (!(flags & CAN_SEND_FLAGS_NO_TX_CALLB) &&
			(this->tx_callb != NULL)) {
		this->tx_callb(__uv_get_user_ptr(), msg, flags);
	}

	uv_enable_int();
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
	if (this->state == CAN_STATE_OPEN) {
		TPCANMsg pcan_msg;
		TPCANTimestamp pcan_time;
		TPCANStatus st = CAN_Read(this->handle, &pcan_msg, &pcan_time);

		while (st == PCAN_ERROR_OK) {
			uv_can_msg_st msg;
			bool known_msg = true;

			if (pcan_msg.MSGTYPE & PCAN_MESSAGE_ERRFRAME) {
				msg.type = CAN_ERR;
				msg.id = pcan_msg.ID;
			}
			else if (pcan_msg.MSGTYPE & PCAN_MESSAGE_EXTENDED) {
				msg.type = CAN_EXT;
				msg.id = pcan_msg.ID;
			}
			else if (pcan_msg.MSGTYPE & PCAN_MESSAGE_STANDARD) {
				msg.type = CAN_STD;
				msg.id = pcan_msg.ID;
			}
			else {
				// status / RTR frames are not handled
				known_msg = false;
			}

			if (known_msg) {
				msg.data_length = pcan_msg.LEN;
				memcpy(msg.data_8bit, pcan_msg.DATA, pcan_msg.LEN);

				// store the reception time
				uint64_t total_us = (uint64_t) pcan_time.micros +
						1000ULL * (uint64_t) pcan_time.millis +
						0x100000000ULL * 1000ULL * (uint64_t) pcan_time.millis_overflow;
				this->lastrxtime.tv_sec = total_us / 1000000ULL;
				this->lastrxtime.tv_usec = total_us % 1000000ULL;

				if (this->rx_callback != NULL &&
						!this->rx_callback(__uv_get_user_ptr(), &msg)) {

				}
				else {
#if CONFIG_TERMINAL_CAN
					// terminal characters are sent to their specific buffer
					if (msg.id == UV_TERMINAL_CAN_RX_ID + uv_canopen_get_our_nodeid() &&
							msg.type == CAN_STD &&
							msg.data_8bit[0] == 0x22 &&
							msg.data_8bit[1] == (UV_TERMINAL_CAN_INDEX & 0xFF) &&
							msg.data_8bit[2] == UV_TERMINAL_CAN_INDEX >> 8 &&
							msg.data_8bit[3] == UV_TERMINAL_CAN_SUBINDEX &&
							msg.data_length > 4) {
						uint8_t i;
						for (i = 0; i < msg.data_length - 4; i++) {
							uv_ring_buffer_push(&this->char_buffer,
									(char*) &msg.data_8bit[4 + i]);
						}
					}
					else {
#endif
						if (uv_ring_buffer_push(&this->rx_buffer, &msg) != ERR_NONE) {
							PRINT("** CAN RX buffer full**\n");
							fflush(stdout);
						}
#if CONFIG_TERMINAL_CAN
					}
#endif
				}
			}

			st = CAN_Read(this->handle, &pcan_msg, &pcan_time);
		}
	}
}


void uv_can_clear_rx_buffer(uv_can_channels_e channel) {
	uv_ring_buffer_clear(&this->rx_buffer);
}



uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
	if (channel) {};
	uv_can_errors_e e = CAN_ERROR_ACTIVE;

	if (this->state == CAN_STATE_INIT ||
			this->state == CAN_STATE_FAULT) {
		e = CAN_ERROR_BUS_OFF;
	}
	return e;
}


void uv_can_clear_rx_messages(uv_can_chn_e chn) {
	if (chn);
}



#endif
