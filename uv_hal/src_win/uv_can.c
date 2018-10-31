/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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

void uv_can_set_up(void) {
	if (this->connection) {
		uv_can_close();
	}

	/* open socket */
	this->connection = true;

}


void uv_can_close(void) {

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
			int recvbytes = 0;

			go = false;

		}
	}
}


void uv_can_clear_rx_buffer(uv_can_channels_e channel) {
	uv_ring_buffer_clear(&this->rx_buffer);
}



#endif
