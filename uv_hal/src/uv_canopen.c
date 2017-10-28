/*
 * uv_canopen.c
 *
 *  Created on: Dec 1, 2015
 *      Author: usevolt
 */


#include "uv_canopen.h"
#if CONFIG_CANOPEN

#include "uv_can.h"
#include "uv_reset.h"
#include "uv_utilities.h"
#include "uv_memory.h"
#include <string.h>
#if CONFIG_CANOPEN_LOG
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#endif







_uv_canopen_st _canopen;

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			(CONFIG_NON_VOLATILE_START.id)




void _uv_canopen_init(void) {
	this->can_callback = NULL;
	this->if_revision = CONFIG_INTERFACE_REVISION;
	_uv_canopen_nmt_init();
	_uv_canopen_heartbeat_init();
	_uv_canopen_sdo_init();
	_uv_canopen_pdo_init();
	_uv_canopen_emcy_init();
}

void _uv_canopen_reset(void) {
	_uv_canopen_nmt_reset();
	_uv_canopen_heartbeat_reset();
	_uv_canopen_sdo_reset();
	_uv_canopen_pdo_reset();
}




void _uv_canopen_step(unsigned int step_ms) {
	_uv_canopen_heartbeat_step(step_ms);
	_uv_canopen_pdo_step(step_ms);
	_uv_canopen_nmt_step(step_ms);
	_uv_canopen_sdo_step(step_ms);

	uv_can_message_st msg;
	uv_errors_e e;
	while (!(e = uv_can_pop_message(CONFIG_CANOPEN_CHANNEL, &msg))) {
		// for every received message, canopen module rx functions are called
		_uv_canopen_nmt_rx(&msg);
		_uv_canopen_heartbeat_rx(&msg);
		_uv_canopen_pdo_rx(&msg);
		_uv_canopen_sdo_rx(&msg);
		_uv_canopen_emcy_rx(&msg);
		if (this->can_callback) {
			this->can_callback(__uv_get_user_ptr(), &msg);
		}
	}
}



void uv_canopen_set_state(canopen_node_states_e state) {
	_uv_canopen_nmt_set_state(state);
}

canopen_node_states_e uv_canopen_get_state(void) {
	return _uv_canopen_nmt_get_state();
}

void uv_canopen_set_can_callback(void (*callb)(void *user_ptr, uv_can_message_st *msg)) {
	this->can_callback = callb;
}


#if CONFIG_CANOPEN_SDO_SYNC

uint8_t uv_canopen_sdo_read8(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data_len) {
	uint8_t ret = 0;
	uv_canopen_sdo_read_sync(node_id, mindex, sindex, data_len, &ret, 100);
	return ret;
}

uint16_t uv_canopen_sdo_read16(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data_len) {
	uint16_t ret = 0;
	uv_canopen_sdo_read_sync(node_id, mindex, sindex, data_len, &ret, 100);
	return ret;
}

uint32_t uv_canopen_sdo_read32(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data_len) {
	uint32_t ret = 0;
	uv_canopen_sdo_read_sync(node_id, mindex, sindex, data_len, &ret, 100);
	return ret;
}

#endif


#endif
