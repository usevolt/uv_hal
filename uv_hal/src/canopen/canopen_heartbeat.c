/*
 * canopen_heartbeat.c
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_heartbeat.h"
#include "uv_canopen.h"
#include CONFIG_MAIN_H

#if CONFIG_CANOPEN

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			(CONFIG_NON_VOLATILE_START.id)


void _uv_canopen_heartbeat_init(void) {
	uv_delay_init(&this->heartbeat_time, this_nonvol->producer_heartbeat_time_ms);
}

void _uv_canopen_heartbeat_reset(void) {
#if CONFIG_CANOPEN_CONSUMER_HEARTBEAT_COUNT
	for (int i = 0; i < CONFIG_CANOPEN_CONSUMER_HEARTBEAT_COUNT; i++) {
		this_nonvol->consumer_heartbeats[i] = 0;
	}
#endif
	this_nonvol->producer_heartbeat_time_ms = CONFIG_CANOPEN_PRODUCER_HEARTBEAT_TIME_MS;

}

void _uv_canopen_heartbeat_step(uint16_t step_ms) {

	if (uv_delay(&this->heartbeat_time, step_ms)) {
		uv_delay_init(&this->heartbeat_time, this_nonvol->producer_heartbeat_time_ms);

		uv_can_message_st msg;
		msg.type = CAN_STD;
		msg.id = CANOPEN_HEARTBEAT_ID + NODEID;
		msg.data_8bit[0] = _uv_canopen_nmt_get_state();
		msg.data_length = 1;
		uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
	}
}


void _uv_canopen_heartbeat_rx(const uv_can_message_st *msg) {
	if (msg);
	// todo: implement logic for receiver heartbeat msgs
}


#endif
