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

	_uv_canopen_nmt_init();
	_uv_canopen_heartbeat_init();
	_uv_canopen_sdo_init();
	_uv_canopen_pdo_init();

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
	}
}



void uv_canopen_set_state(canopen_node_states_e state) {
	_uv_canopen_nmt_set_state(state);
}

canopen_node_states_e uv_canopen_get_state(void) {
	return _uv_canopen_nmt_get_state();
}




#endif
