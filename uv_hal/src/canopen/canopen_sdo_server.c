/*
 * canopen_sdo_server.c
 *
 *  Created on: Aug 27, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_sdo_server.h"
#include "uv_canopen.h"

#if CONFIG_CANOPEN


#define this (&_canopen.sdo.server)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			(CONFIG_NON_VOLATILE_START.id)

/// @brief: Send a SDO Server abort response message
static inline void sdo_server_abort(uint16_t main_index,
				uint8_t sub_index, uv_sdo_error_codes_e err_code) {
	_uv_canopen_sdo_abort(CANOPEN_SDO_REQUEST_ID, main_index, sub_index, err_code);
}



void _uv_canopen_sdo_server_init(void) {
	this->state = CANOPEN_SDO_STATE_READY;
}

void _uv_canopen_sdo_server_reset(void) {

}

void _uv_canopen_sdo_server_step(uint16_t step_ms) {

}

void _uv_canopen_sdo_server_rx(const uv_can_message_st *msg, sdo_request_type_e sdo_type) {

}


#endif
