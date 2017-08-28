/*
 * canopen_sdo_client.c
 *
 *  Created on: Aug 27, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_sdo_client.h"
#include "uv_canopen.h"


#if CONFIG_CANOPEN

#define this (&_canopen.sdo.client)
#define NODEID			(CONFIG_NON_VOLATILE_START.id)


/// @brief: Send a SDO Client abort response message
static inline void sdo_client_abort(uint16_t main_index,
				uint8_t sub_index, uv_sdo_error_codes_e err_code) {
	_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, main_index, sub_index, err_code);
}


void _uv_canopen_sdo_client_init(void) {
	this->state = CANOPEN_SDO_STATE_READY;
}

void _uv_canopen_sdo_client_reset(void) {

}

void _uv_canopen_sdo_client_step(uint16_t step_ms) {

}

void _uv_canopen_sdo_client_rx(const uv_can_message_st *msg,
		sdo_request_type_e sdo_type, uint8_t node_id) {

}

/// @brief: Sends a CANOpen SDO write request without waiting for the response
uv_errors_e _uv_canopen_sdo_client_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data) {
	uv_errors_e ret = ERR_NONE;


	return ret;
}




/// @brief: Sends a CANOpen SDO write request and waits for the response
/// **timeout_ms** milliseconds. If the write request failed or the timeout
/// expires, returns an error.
uv_errors_e _uv_canopen_sdo_client_write_sync(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data_len, void *data, int32_t timeout_ms) {
	uv_errors_e ret = ERR_NONE;


	return ret;
}

/// @brief: Sends a CANOpen SDO read request and waits for the response
/// **timeout_ms** milliseconds. If the read request failed or the timeout
/// expires, returns an error.
uv_errors_e _uv_canopen_sdo_client_read_sync(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data, int32_t timeout_ms) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}



#endif
