/*
 * canopen_heartbeat.c
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_heartbeat.h"
#include "uv_canopen.h"
#include "string.h"
#include CONFIG_MAIN_H

#if CONFIG_CANOPEN

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			this->current_node_id


#define PRODUCER_NODEID(x)	CAT(CONFIG_CANOPEN_HEARTBEAT_PRODUCER_NODEID, INC(x))
#define PRODUCER_TIME(x)	CAT(CONFIG_CANOPEN_HEARTBEAT_PRODUCER_TIME, INC(x))

#define PRODUCER_RESET(x)	this_nonvol->consumer_heartbeats[x].cycle_time = PRODUCER_TIME(x);\
							this_nonvol->consumer_heartbeats[x].node_id = PRODUCER_NODEID(x);\



void _uv_canopen_heartbeat_init(void) {
	uv_delay_init(&this->heartbeat_time, this_nonvol->producer_heartbeat_time_ms);
#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
	memset(this->consumer_heartbeat_times, 0, sizeof(this->consumer_heartbeat_times));
	memset(this->consumer_heartbeat_states, CANOPEN_STOPPED, sizeof(this->consumer_heartbeat_states));
	// if Heartbeat consumer is enabled, configure to receive all heartbeat messages
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL, CANOPEN_HEARTBEAT_ID, ~0x7F, CAN_STD);
#endif
}


void _uv_canopen_heartbeat_reset(void) {
#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
	REPEAT(CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT, PRODUCER_RESET);
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

#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
	// increase time counters for all heartbeat producers which are followed
	for (uint16_t i = 0; i < CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT; i++) {
		if (this->consumer_heartbeat_times[i] <
				this_nonvol->consumer_heartbeats[i].cycle_time) {
			this->consumer_heartbeat_times[i] += step_ms;
		}
	}
#endif

}


void _uv_canopen_heartbeat_rx(const uv_can_message_st *msg) {
#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
	// filter off all else than heartbeat messages
	if ((msg->type == CAN_STD) &&
			((msg->id & ~0x7F) == CANOPEN_HEARTBEAT_ID) &&
			(msg->data_length == 1)) {

		uint8_t node_id = msg->id & 0x7F;
		for (uint16_t i = 0; i < CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT; i++) {
			if (this_nonvol->consumer_heartbeats[i].node_id == node_id) {
				this->consumer_heartbeat_times[i] = 0;
				this->consumer_heartbeat_states[i] = msg->data_8bit[0];
				break;
			}
		}
	}
#endif
}


#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
bool uv_canopen_heartbeat_producer_is_expired(uint8_t node_id) {
	for (uint16_t i = 0; i < CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT; i++) {
		if (this_nonvol->consumer_heartbeats[i].node_id == node_id) {
			return (this->consumer_heartbeat_times[i] >=
				this_nonvol->consumer_heartbeats[i].cycle_time);
		}
	}
	return false;
}


canopen_node_states_e uv_canopen_heartbeat_producer_get_state(uint8_t nodeid) {
	canopen_node_states_e ret = CANOPEN_STOPPED;
	for (uint16_t i = 0; i < CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT; i++) {
		if (this_nonvol->consumer_heartbeats[i].node_id == nodeid) {
			ret = this->consumer_heartbeat_states[i];
			break;
		}
	}
	return ret;
}


#endif

#endif
