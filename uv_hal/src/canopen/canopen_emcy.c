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


#include "canopen/canopen_emcy.h"
#include "uv_canopen.h"
#include "uv_rtos.h"
#include CONFIG_MAIN_H

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			this->current_node_id


void _uv_canopen_emcy_init(void) {
	uv_ring_buffer_init(&this->emcy_rx, this->emcy_rx_buffer,
			CONFIG_CANOPEN_EMCY_RX_BUFFER_SIZE, sizeof(canopen_emcy_msg_st));
	uv_delay_init(&this->emcy_inihbit_delay, CONFIG_CANOPEN_EMCY_INHIBIT_TIME_MS);
	this->emcy_callb = NULL;
}

void _uv_canopen_emcy_reset(void) {
	_uv_canopen_emcy_init();
}


void _uv_canopen_emcy_rx(const uv_can_message_st *msg) {
	if (uv_canopen_get_state() != CANOPEN_STOPPED) {
		if (((msg->id & (~CANOPEN_NODE_ID_MASK)) == CANOPEN_EMCY_ID) &&
				(msg->type == CAN_STD)) {
			canopen_emcy_msg_st emcy;
			emcy.node_id = msg->id & CANOPEN_NODE_ID_MASK;
			emcy.error_code = msg->data_16bit[3];
			emcy.data = msg->data_32bit[0];
			uv_disable_int();
			uv_ring_buffer_push(&this->emcy_rx, &emcy);
			uv_enable_int();
		}
	}
}


uv_errors_e uv_canopen_emcy_get(canopen_emcy_msg_st *dest) {
	uv_errors_e e;
	e = uv_ring_buffer_pop(&this->emcy_rx, dest);
	return e;
}


void uv_canopen_emcy_send(const uv_emcy_codes_e err_code, uint32_t data) {

	if (uv_canopen_get_state() != CANOPEN_STOPPED) {
		if (uv_delay_has_ended(&this->emcy_inihbit_delay)) {
			uv_can_message_st msg = { };
			msg.id = CANOPEN_EMCY_ID + NODEID;
			msg.type = CAN_STD;
			msg.data_length = 8;
			msg.data_16bit[3] = err_code;
			msg.data_32bit[0] = data;
			uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
			if ((msg.id & 0x7F) == NODEID) {
				// send reply also locally if it was directed to us
				uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &msg,
						CAN_SEND_FLAGS_LOCAL |
						CAN_SEND_FLAGS_NO_TX_CALLB);
			}

			// call the emcy callback if one has been assigned
			if (this->emcy_callb != NULL) {
				this->emcy_callb(err_code, data);
			}
		}
		// todo: add error code to [1003]
	}
}



void _uv_canopen_emcy_step(uint16_t step_ms) {
	if (uv_canopen_get_state() != CANOPEN_STOPPED) {
		uv_delay(&_canopen.emcy_inihbit_delay, step_ms);
	}
}
