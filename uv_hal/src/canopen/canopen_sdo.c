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


#include "canopen/canopen_sdo.h"

#if CONFIG_CANOPEN

#include <string.h>
#include "uv_canopen.h"
#include "uv_rtos.h"
#include CONFIG_MAIN_H

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			this->current_node_id


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

#if CONFIG_TARGET_LPC40XX
/// @brief: How long _uv_canopen_sdo_send() waits for room in a full CAN
/// transmit buffer before it gives the frame up.
#define SDO_SEND_WAIT_MS				20
#endif


/// @brief: The last frame our SDO client sent to our own SDO server. Such a
/// transfer is kept off the bus, see _uv_canopen_sdo_send(). Written by the
/// task running the client, read by the CANopen task.
static volatile struct {
	uv_can_msg_st msg;
	bool valid;
} sdo_self_req = { .valid = false };

/// @brief: True while the SDO server is answering a request our own client
/// sent, so that its replies stay off the bus as well.
static volatile bool sdo_server_reply_local = false;


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
	else if ((GET_CMD_BYTE(msg) & 0b11111111) == ABORT_DOMAIN_TRANSFER) {
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
	else if ((GET_CMD_BYTE(msg) & 0b11100011) == INITIATE_BLOCK_UPLOAD_REPLY2) {
		ret = INITIATE_BLOCK_UPLOAD_REPLY2;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100001) == END_BLOCK_DOWNLOAD) {
		ret = END_BLOCK_DOWNLOAD;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100001) == END_BLOCK_DOWNLOAD_REPLY) {
		ret = END_BLOCK_DOWNLOAD_REPLY;
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
	_uv_canopen_sdo_client_init();
#if CONFIG_CANOPEN_SDO_SERVER
	_uv_canopen_sdo_server_init();
#endif

}


void _uv_canopen_sdo_step(uint16_t step_ms) {
	if (uv_canopen_get_state() != CANOPEN_STOPPED) {
		_uv_canopen_sdo_client_step(step_ms);
#if CONFIG_CANOPEN_SDO_SERVER
		_uv_canopen_sdo_server_step(step_ms);
#endif
	}
}



void _uv_canopen_sdo_rx(const uv_can_message_st *msg) {
	if (uv_canopen_get_state() != CANOPEN_STOPPED) {
		// parse received message and filter only SDO reponses and requests
		sdo_request_type_e msg_type = _canopen_sdo_get_request_type(msg);
		if (msg_type != INVALID_MSG) {
			// SDO Server receives only SDO requested dedicated to this device
			if ((GET_NODEID(msg) == NODEID) &&
					IS_SDO_REQUEST(msg)) {
	#if CONFIG_CANOPEN_SDO_SERVER
				// a request our own client sent is answered locally only. A
				// request from anyone else, a frame the remote module injected
				// on a remote sink's behalf included, is answered on the bus.
				bool local = false;
				uv_disable_int();
				if (sdo_self_req.valid &&
						(sdo_self_req.msg.id == msg->id) &&
						(sdo_self_req.msg.type == msg->type) &&
						(sdo_self_req.msg.data_length == msg->data_length) &&
						(sdo_self_req.msg.data_64bit == msg->data_64bit)) {
					sdo_self_req.valid = false;
					local = true;
				}
				else {
				}
				uv_enable_int();
				sdo_server_reply_local = local;
				_uv_canopen_sdo_server_rx(msg, msg_type);
	#endif
			}
			else if ((IS_SDO_RESPONSE(msg) ||
							msg_type == ABORT_DOMAIN_TRANSFER)) {
				_uv_canopen_sdo_client_rx(msg, msg_type, GET_NODEID(msg));
			}
			else {
			}
		}
	}
}


const canopen_object_st *_canopen_find_object(const uv_can_message_st *msg,
		canopen_permissions_e permission_req) {
	const canopen_object_st *ret;

	if ((ret = _uv_canopen_obj_dict_get(GET_MINDEX(msg), GET_SINDEX(msg)))) {
		if (!(ret->permissions & permission_req)) {
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
			dest->data_32bit[1] = 0;
			if (src->data_ptr != NULL) {
				memcpy(&dest->data_32bit[1],
						&((uint8_t*) src->data_ptr)[(subindex - 1) * CANOPEN_TYPE_LEN(src->type)],
						CANOPEN_TYPE_LEN(src->type));
			}
		}
	}
	else {
		if (src->data_ptr != NULL) {
			memcpy(&dest->data_32bit[1], src->data_ptr, CANOPEN_TYPE_LEN(src->type));
		}
	}
	uv_enable_int();
}

bool _canopen_write_data(const canopen_object_st *dest,
		const uv_can_msg_st *src, uint8_t subindex) {
	uv_disable_int();
	bool ret = true;
	int8_t len = CANOPEN_SIZEOF(dest->type);
	if (src->data_8bit[0] & 1) {
		len = 4 - ((src->data_8bit[0] >> 2) & 0b11);
	}
	if (CANOPEN_IS_ARRAY(dest->type)) {
		// cannot write to subindex 0
		if (subindex == 0) {
			ret = false;
		}
		else {
			if (dest->data_ptr != NULL) {
				if ((subindex - 1) * CANOPEN_SIZEOF(dest->type) + len <=
						dest->array_max_size * CANOPEN_SIZEOF(dest->type)) {
					memcpy(&((uint8_t*) dest->data_ptr)[(subindex - 1) *
														CANOPEN_TYPE_LEN(dest->type)],
							&src->data_32bit[1],
							len);
				}
				else {
					ret = false;
				}
			}
		}
	}
	else {
		if (CANOPEN_IS_STRING(dest->type)) {
			len = MIN(len, dest->string_len);
		}
		else {
			len = MIN(len, dest->type);
		}
		if (dest->data_ptr != NULL) {
			memcpy(dest->data_ptr, &src->data_32bit[1], len);
		}
	}

	uv_enable_int();
	return ret;
}



/// @brief: Sends an SDO frame of either the client or the server.
///
/// A transfer between our own client and our own server runs entirely in the
/// receive buffer: the client's requests (CANOPEN_SDO_REQUEST_ID + NODEID can
/// only come from our client) and the server's replies to them are delivered
/// locally and never reach the bus or the tx callback. Putting them on the
/// bus as well did nothing but harm: both ends answer each other at once from
/// the receive buffer, much faster than the bus can carry the copies, so a
/// burst of self-reads filled the transmit buffer and the frames queued
/// behind it - a request to another node among them - were lost.
///
/// Every other frame goes on the bus and through the tx callback, a server
/// reply to another client also locally as before. On LPC40XX, should the
/// transmit buffer be full, the frame waits up to SDO_SEND_WAIT_MS for room
/// instead of being dropped: a lost SDO frame costs the full SDO timeout of
/// the transfer. The wait is bounded, as with no other node on the bus to
/// acknowledge them the frames never drain.
void _uv_canopen_sdo_send(uv_can_msg_st *msg) {
	bool to_us = ((msg->id & 0x7F) == NODEID);
	bool self_req = to_us && IS_SDO_REQUEST(msg);
	bool self_reply = to_us && IS_SDO_RESPONSE(msg) && sdo_server_reply_local;

	if (self_req) {
		// remembered before it is delivered, so that the server knows it
		uv_disable_int();
		memcpy((void*) &sdo_self_req.msg, msg, sizeof(*msg));
		sdo_self_req.valid = true;
		uv_enable_int();
	}
	else if (!self_reply) {
#if CONFIG_TARGET_LPC40XX
		// waited for before sending rather than retried after a failure:
		// uv_can_send() runs the tx callback even when the buffer is full, and
		// a retry would hand the remote module the same frame twice
		uint16_t waited_ms = 0;
		while (uv_can_tx_full(CONFIG_CANOPEN_CHANNEL) &&
				(waited_ms < SDO_SEND_WAIT_MS)) {
			uv_rtos_task_delay(1);
			waited_ms++;
		}
#endif
		uv_can_send(CONFIG_CANOPEN_CHANNEL, msg);
	}
	else {
	}

	if (to_us) {
		uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, msg,
				CAN_SEND_FLAGS_LOCAL |
				CAN_SEND_FLAGS_NO_TX_CALLB);
	}
	else {
	}
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
	_uv_canopen_sdo_send(&msg);
}


#endif
