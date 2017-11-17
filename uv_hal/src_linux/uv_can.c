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

#if CONFIG_CAN_LOG
extern bool can_log;
#endif

typedef struct {
	bool init;
	unsigned int baudrate;
	char dev_name[20];
} can_st;


static can_st _can = {
		.init = false,
		.baudrate = CONFIG_CAN0_BAUDRATE,
		.dev_name = "can0"
};
#define this (&_can)


void _uv_can_hal_send(uv_can_channels_e chn);



void uv_can_set_baudrate(uv_can_channels_e channel, unsigned int baudrate) {
	this->baudrate = baudrate;
}

void uv_can_set_name(uv_can_channels_e channel, const char *dev_name) {
	strcpy(this->dev_name, dev_name);
}


uv_errors_e _uv_can_init() {
	uv_errors_e ret = ERR_NONE;

	if (!this->baudrate) {
		printf("Error: Baudrate has to be set for CAN before initialization.\n");
		ret = ERR_NOT_INITIALIZED;
	}
	else {
		//todo: socket can initialization
	}

	return ret;
}


#if CONFIG_TERMINAL_CAN
uv_errors_e uv_can_get_char(char *dest) {
	return uv_ring_buffer_pop(&this->char_buffer, dest);
}
#endif



uv_errors_e uv_can_send_message(uv_can_channels_e channel, uv_can_message_st* message) {
	uv_errors_e ret = ERR_NONE;
	return ret;
}



uv_errors_e uv_can_pop_message(uv_can_channels_e channel, uv_can_message_st *message) {
	uv_errors_e ret = ERR_NONE;
	return ret;
}



uv_errors_e uv_can_reset(uv_can_channels_e channel) {
	return ERR_NONE;
}



/// @brief: Inner hal step function which is called in rtos hal task
void _uv_can_hal_step(unsigned int step_ms) {

}


#endif
