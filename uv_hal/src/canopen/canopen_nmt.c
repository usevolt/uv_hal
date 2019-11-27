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


#include "canopen/canopen_nmt.h"

#if CONFIG_CANOPEN
#include "uv_canopen.h"
#include CONFIG_MAIN_H
#include "uv_uart.h"
#include "uv_reset.h"


#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			this->current_node_id


void _uv_canopen_nmt_init(void) {
#if CONFIG_UV_BOOTLOADER
	// uv bootloader has already sent the CANOPEN_BOOT_UP_MESSAGE
#elif CONFIG_CANOPEN_HEARTBEAT_PRODUCER
	uv_can_message_st msg;
	msg.type = CAN_STD;
	msg.id = CANOPEN_HEARTBEAT_ID + NODEID;
	msg.data_8bit[0] = CANOPEN_BOOT_UP;
	msg.data_length = 1;
	uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
#endif
	this->state = CANOPEN_BOOT_UP;
#if CONFIG_CANOPEN_AUTO_PREOPERATIONAL
	uv_canopen_set_state(CANOPEN_PREOPERATIONAL);
#endif
#if !CONFIG_CANOPEN_RXCONFIG_DISABLE
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_NMT_ID, CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
}

void _uv_canopen_nmt_reset(void) {
	CONFIG_NON_VOLATILE_START.id = CONFIG_CANOPEN_DEFAULT_NODE_ID;
}

void _uv_canopen_nmt_step(uint16_t step_ms) {
	if (step_ms);
}


void _uv_canopen_nmt_rx(const uv_can_message_st *msg) {
#if CONFIG_CANOPEN_NMT_SLAVE
	if (msg->id == CANOPEN_NMT_ID) {
		if (msg->data_length >= 2) {
			if (msg->data_8bit[1] == NODEID || msg->data_8bit[1] == 0) {
				switch (msg->data_8bit[0]) {
				case CANOPEN_NMT_START_NODE:
					this->state = CANOPEN_OPERATIONAL;
					break;
				case CANOPEN_NMT_STOP_NODE:
					this->state = CANOPEN_STOPPED;
					break;
				case CANOPEN_NMT_RESET_NODE:
				case CANOPEN_NMT_RESET_COM:
					uv_system_reset();
					break;
				case CANOPEN_NMT_SET_PREOPERATIONAL:
					this->state = CANOPEN_PREOPERATIONAL;
					break;
				default:
					break;
				}
			}
		}
	}
#endif
}


canopen_node_states_e _uv_canopen_nmt_get_state(void) {
	return this->state;
}

void _uv_canopen_nmt_set_state(canopen_node_states_e state) {
	if (this->state == CANOPEN_BOOT_UP &&
			state == CANOPEN_PREOPERATIONAL) {
		// when changing from boot up to preoperational, instantly send
		// the message indicating that
		uv_can_message_st msg;
		msg.type = CAN_STD;
		msg.id = CANOPEN_HEARTBEAT_ID + NODEID;
		msg.data_8bit[0] = CANOPEN_PREOPERATIONAL;
		msg.data_length = 1;
		uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
	}
	this->state = state;
}

void uv_canopen_command(uint8_t nodeid, canopen_nmt_commands_e cmd) {
	uv_can_msg_st msg;
	msg.type = CAN_STD;
	msg.id = 0;
	msg.data_length = 2;
	msg.data_8bit[0] = cmd;
	msg.data_8bit[1] = nodeid;
	uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
}


#if CONFIG_CANOPEN_NMT_MASTER

void uv_canopen_nmt_master_reset_node(uint8_t nodeid) {
	uv_can_msg_st msg;
	msg.type = CAN_STD;
	msg.data_length = 2;
	msg.id = CANOPEN_NMT_ID;
	msg.data_8bit[0] = CANOPEN_NMT_RESET_NODE;
	msg.data_8bit[1] = nodeid;
	uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
}


void uv_canopen_nmt_master_set_node_state(uint8_t nodeid, canopen_nmt_commands_e state) {
	uv_can_msg_st msg;
	msg.type = CAN_STD;
	msg.data_length = 2;
	msg.id = CANOPEN_NMT_ID;
	msg.data_8bit[0] = state;
	msg.data_8bit[1] = nodeid;
	uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
}


#endif


#endif
