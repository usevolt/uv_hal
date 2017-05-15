/*
 * canopen_nmt.c
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_nmt.h"

#if CONFIG_CANOPEN
#include "uv_canopen.h"
#include CONFIG_MAIN_H
#include "uv_uart.h"
#include "uv_reset.h"


#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			(CONFIG_NON_VOLATILE_START.id)


void _uv_canopen_nmt_init(void) {

	this->state = CANOPEN_PREOPERATIONAL;
#if CONFIG_TARGET_LPC1785
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, CANOPEN_NMT_ID, CAN_STD);
#else
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, CANOPEN_NMT_ID, CAN_ID_MASK_DEFAULT, CAN_STD);
#endif

	// uv bootloader sends the NMT boot up message,
	// we should sent here NMT preoperational message
	uv_can_message_st msg;
	msg.type = CAN_STD;
	msg.id = CANOPEN_HEARTBEAT_ID + NODEID;
	msg.data_8bit[0] = 128;
	msg.data_length = 1;
	uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);

}

void _uv_canopen_nmt_reset(void) {
	NODEID = CONFIG_CANOPEN_DEFAULT_NODE_ID;
}

void _uv_canopen_nmt_step(uint16_t step_ms) {
	if (step_ms);
}

void _uv_canopen_nmt_rx(const uv_can_message_st *msg) {
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
}


canopen_node_states_e _uv_canopen_nmt_get_state(void) {
	return this->state;
}

void _uv_canopen_nmt_set_state(canopen_node_states_e state) {
	this->state = state;
}


#endif
