/*
 * canopen_sdo.c
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_sdo.h"

#if CONFIG_CANOPEN

#include <string.h>
#include "uv_canopen.h"
#include "uv_rtos.h"
#include CONFIG_MAIN_H

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			(CONFIG_NON_VOLATILE_START.id)


#define GET_CMD_BYTE(msg_ptr)			((msg_ptr)->data_8bit[0])
#define GET_MINDEX(msg_ptr)				((msg_ptr)->data_8bit[1] + ((msg_ptr)->data_8bit[2] * 256))
#define GET_SINDEX(msg_ptr)				((msg_ptr)->data_8bit[3])
#define GET_NODEID(msg_ptr)				((msg_ptr)->id & 0x7F)
#define SET_CMD_BYTE(msg_ptr, value)	(msg_ptr)->data_8bit[0] = (value)
#define SET_MINDEX(msg_ptr, value)		do {(msg_ptr)->data_8bit[1] = (value) % 256; \
										(msg_ptr)->data_8bit[2] = (value) / 256; } while (0)
#define SET_SINDEX(msg_ptr, value)		(msg_ptr)->data_8bit[3] = (value)
#define IS_SDO_REQUEST(msg_ptr)			(((msg_ptr)->id & (~0x7F)) == CANOPEN_SDO_REQUEST_ID)
#define IS_SDO_RESPONSE(msg_ptr)		(((msg_ptr)->id & (~0x7F)) == CANOPEN_SDO_RESPONSE_ID)


sdo_request_type_e _canopen_sdo_get_request_type(const uv_can_message_st *msg) {
	sdo_request_type_e ret;

	if ((msg->type != CAN_STD) ||
			(msg->data_length != 8) ||
			(((msg->id & (~0x7F)) != CANOPEN_SDO_REQUEST_ID) &&
					(msg->id & (~0x7F)) != CANOPEN_SDO_RESPONSE_ID)) {
		ret = INVALID_MSG;
	}
	// initiate domain download message
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == INITIATE_DOMAIN_DOWNLOAD) {
		ret = INITIATE_DOMAIN_DOWNLOAD;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == INITIATE_DOMAIN_DOWNLOAD_REPLY) {
		ret = INITIATE_DOMAIN_DOWNLOAD_REPLY;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == DOWNLOAD_DOMAIN_SEGMENT) {
		ret = DOWNLOAD_DOMAIN_SEGMENT;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == DOWNLOAD_DOMAIN_SEGMENT_REPLY) {
		ret = DOWNLOAD_DOMAIN_SEGMENT_REPLY;
	}
	// initiate domain upload
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == INITIATE_DOMAIN_UPLOAD) {
		ret = INITIATE_DOMAIN_UPLOAD;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == UPLOAD_DOMAIN_SEGMENT) {
		ret = UPLOAD_DOMAIN_SEGMENT;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == ABORT_DOMAIN_TRANSFER) {
		// abort
		ret = ABORT_DOMAIN_TRANSFER;
	}
#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER
	else if ((GET_CMD_BYTE(msg) & 0b11100001) == INITIATE_BLOCK_DOWNLOAD) {
		ret = INITIATE_BLOCK_DOWNLOAD;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100011) == INITIATE_BLOCK_DOWNLOAD_REPLY) {
		ret = INITIATE_BLOCK_DOWNLOAD_REPLY;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100011) == DOWNLOAD_BLOCK_SEGMENT_REPLY) {
		ret = DOWNLOAD_BLOCK_SEGMENT_REPLY;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100001) == END_BLOCK_DOWNLOAD) {
		ret = END_BLOCK_DOWNLOAD;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100001) == END_BLOCK_DOWNLOAD_REPLY) {
		ret = END_BLOCK_DOWNLOAD_REPLY;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100011) == INITIATE_BLOCK_UPLOAD_REPLY2) {
		ret = INITIATE_BLOCK_UPLOAD_REPLY2;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100011) == DOWNLOAD_BLOCK_SEGMENT_REPLY) {
		ret = DOWNLOAD_BLOCK_SEGMENT_REPLY;
	}
#endif
	else {
		ret = UNKNOWN_SDO_MSG;
	}

	return ret;
}



void _uv_canopen_sdo_init(void) {
#if CONFIG_TARGET_LPC1785
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_REQUEST_ID + NODEID, CAN_STD);
#else
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_REQUEST_ID + NODEID, CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
	_uv_canopen_sdo_client_init();
	_uv_canopen_sdo_server_init();

}



void _uv_canopen_sdo_rx(const uv_can_message_st *msg) {

	// parse received message and filter only SDO reponses and requests
	sdo_request_type_e msg_type = _canopen_sdo_get_request_type(msg);
	if (msg_type != INVALID_MSG) {
		// SDO Server receives only SDO requested dedicated to this device
		if ((GET_NODEID(msg) == NODEID) &&
				IS_SDO_REQUEST(msg)) {
			_uv_canopen_sdo_server_rx(msg, msg_type);
		}
		// SDO Client receives only SDO responses from other nodes than this device
		else if ((GET_NODEID(msg) != NODEID) &&
				IS_SDO_RESPONSE(msg)) {
			_uv_canopen_sdo_client_rx(msg, msg_type, GET_NODEID(msg));
		}
	}
}


bool _canopen_find_object(const uv_can_message_st *msg,
		canopen_object_st *obj, canopen_permissions_e permission_req) {
	bool ret = true;

	if (_uv_canopen_obj_dict_get(GET_MINDEX(msg), GET_SINDEX(msg), obj)) {
		if (!(obj->permissions & permission_req)) {
			// object is not readable
			if (permission_req == CANOPEN_RO) {
				_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, GET_MINDEX(msg), GET_SINDEX(msg),
						CANOPEN_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT);
			}
			else {
				_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, GET_MINDEX(msg), GET_SINDEX(msg),
						CANOPEN_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT);
			}
			ret = false;
		}
	}
	else {
		// couldn't find the requested object
		_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, GET_MINDEX(msg), GET_SINDEX(msg),
				CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
		ret = false;
	}
	return ret;
}



void _canopen_copy_data(uv_can_message_st *dest, const canopen_object_st *src, uint8_t subindex) {
	uv_disable_int();
	if (CANOPEN_IS_ARRAY(src->type)) {
		// for objects subindex 0 returns the array max size
		if (subindex == 0) {
			dest->data_32bit[1] = src->array_max_size;
		}
		else {
			dest->data_32bit[1] = ((uint8_t*) src->data_ptr)[(subindex - 1) * CANOPEN_TYPE_LEN(src->type)];
		}
	}
	else {
		memcpy(&dest->data_32bit[1], src->data_ptr, CANOPEN_TYPE_LEN(src->type));
	}
	uv_enable_int();
}

bool _canopen_write_data(canopen_object_st *dest, const uv_can_msg_st *src, uint8_t subindex) {
	uv_disable_int();
	bool ret = true;
	if (CANOPEN_IS_ARRAY(dest->type)) {
		// cannot write to subindex 0
		if (subindex == 0) {
			ret = false;
		}
		else {
			memcpy(&((uint8_t*) dest->data_ptr)[(subindex - 1) * CANOPEN_TYPE_LEN(dest->type)],
					&src->data_32bit[1], CANOPEN_TYPE_LEN(dest->type));
		}
	}
	else {
		memcpy(dest->data_ptr, &src->data_32bit[1], CANOPEN_TYPE_LEN(dest->type));
	}

	uv_enable_int();
	return ret;
}





void _uv_canopen_sdo_abort(uint16_t request_response, uint16_t main_index,
		uint8_t sub_index, uv_sdo_error_codes_e err_code) {
	uv_can_message_st msg;
	msg.type = CAN_STD;
	msg.id = request_response + NODEID;
	msg.data_length = 8;
	msg.data_8bit[0] = 0x80;
	msg.data_8bit[1] = main_index % 256;
	msg.data_8bit[2] = main_index / 256;
	msg.data_8bit[3] = sub_index;
	msg.data_32bit[1] = err_code;
	uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
}


#endif
