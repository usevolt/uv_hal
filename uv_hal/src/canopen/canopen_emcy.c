/*
 * canopen_emcy.c
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_emcy.h"
#include "uv_canopen.h"
#include CONFIG_MAIN_H

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			this->current_node_id


#define CONFIG_RX_MSG(i)	do { uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, \
CAT(CONFIG_CANOPEN_EMCY_MSG_ID_, INC(i)), CAN_STD); } while(0)

void _uv_canopen_emcy_init(void) {
	uv_ring_buffer_init(&this->emcy_rx, this->emcy_rx_buffer,
			CONFIG_CANOPEN_EMCY_RX_BUFFER_SIZE, sizeof(canopen_emcy_msg_st));
#if CONFIG_TARGET_LPC1785
	REPEAT(CONFIG_CANOPEN_EMCY_MSG_COUNT, CONFIG_RX_MSG);

#endif
}

void _uv_canopen_emcy_reset(void) {
	_uv_canopen_emcy_init();
}


void _uv_canopen_emcy_rx(const uv_can_message_st *msg) {
	if (((msg->id & (~CANOPEN_NODE_ID_MASK)) == CANOPEN_EMCY_ID) &&
			(msg->type == CAN_STD)) {
		canopen_emcy_msg_st emcy;
		emcy.node_id = msg->id & CANOPEN_NODE_ID_MASK;
		emcy.error_code = msg->data_16bit[3];
		emcy.data = msg->data_32bit[0];
		uv_ring_buffer_push(&this->emcy_rx, &emcy);
	}
}


bool uv_canopen_emcy_get(canopen_emcy_msg_st *dest) {
	uv_errors_e e;
	e = uv_ring_buffer_pop(&this->emcy_rx, dest);
	return (e == ERR_NONE);
}


void uv_canopen_emcy_send(const uv_emcy_codes_e err_code, uint32_t data) {

	uv_can_message_st msg = { };
	msg.id = CANOPEN_EMCY_ID + NODEID;
	msg.type = CAN_STD;
	msg.data_length = 8;
	msg.data_16bit[3] = err_code;
	msg.data_32bit[0] = data;
	uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);

	// todo: add error code to [1003]
}
