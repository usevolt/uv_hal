/*
 * canopen_sdo_server.c
 *
 *  Created on: Aug 27, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_sdo_server.h"
#include "uv_canopen.h"
#include CONFIG_MAIN_H
#include <string.h>

#if CONFIG_CANOPEN


#define this (&_canopen.sdo.server)
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


/// @brief: Send a SDO Server abort response message
static inline void sdo_server_abort(uint16_t main_index,
				uint8_t sub_index, uv_sdo_error_codes_e err_code) {
	_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, main_index, sub_index, err_code);
}



void _uv_canopen_sdo_server_init(void) {
	this->state = CANOPEN_SDO_STATE_READY;
}

void _uv_canopen_sdo_server_reset(void) {
	_uv_canopen_sdo_server_init();
}

void _uv_canopen_sdo_server_step(uint16_t step_ms) {
	if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD) ||
			this->state == CANOPEN_SDO_STATE_SEGMENTED_UPLOAD) {
		if (uv_delay(&this->delay, step_ms)) {
			sdo_server_abort(this->mindex, this->sindex,
					CANOPEN_SDO_ERROR_GENERAL);
			this->state = CANOPEN_SDO_STATE_READY;
		}
	}
}

void _uv_canopen_sdo_server_rx(const uv_can_message_st *msg, sdo_request_type_e sdo_type) {
	canopen_object_st obj;
	uv_can_msg_st reply_msg;
	reply_msg.type = CAN_STD;
	reply_msg.id = CANOPEN_SDO_RESPONSE_ID + NODEID;
	reply_msg.data_length = 8;
	memset(reply_msg.data_8bit, 0, 8);
	SET_MINDEX(&reply_msg, GET_MINDEX(msg));
	SET_SINDEX(&reply_msg, GET_SINDEX(msg));

	// receiving an abort message returns the node to default state
	if (sdo_type == ABORT_DOMAIN_TRANSFER) {
		this->state = CANOPEN_SDO_STATE_READY;
	}
	else if (this->state == CANOPEN_SDO_STATE_READY) {

		// initiate download (write request)
		if (sdo_type == INITIATE_DOMAIN_DOWNLOAD) {
			// try to find the requested object. If object was not found,
			// abort message is sent automatically
			if (_canopen_find_object(msg, &obj, CANOPEN_WO)) {

				// segmented transfer
				if (!(GET_CMD_BYTE(msg) & (1 << 1))) {
					// segmented transfer can be started only on string type objects
					if (uv_canopen_is_string(&obj)) {
						this->state = CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD;
						this->data_index = 0;
						this->mindex = GET_MINDEX(msg);
						this->sindex = GET_SINDEX(msg);
						this->toggle = 0;
						uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
						SET_CMD_BYTE(&reply_msg,
								INITIATE_DOMAIN_DOWNLOAD_REPLY);
						// data bytes indicate data index which starts from 0
						memset(&reply_msg.data_8bit[4], 0, 4);
						uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					}
					else {
						sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
								CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
					}
				}

				// expedited transfer
				else {
					// expedited transfer can be started to all other than string type objects
					if (!uv_canopen_is_string(&obj)) {
						SET_CMD_BYTE(&reply_msg,
								INITIATE_DOMAIN_DOWNLOAD_REPLY);
						if (_canopen_write_data(&obj, msg, GET_SINDEX(msg))) {
							memcpy(&reply_msg.data_32bit[1], &msg->data_32bit[1], 4);
							uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
						}
						else {
							// tried to write to array index 0, abort transfer
							sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
									CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
						}
					}
					else {
						sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
								CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
					}
				}
			}
		}

		// initiate upload (read request)
		else if (sdo_type == INITIATE_DOMAIN_UPLOAD) {
			// try to find the requested object. If object was not found,
			// abort message is sent automatically
			if (_canopen_find_object(msg, &obj, CANOPEN_RO)) {

				// segmented transfer
				if (obj.type == CANOPEN_STRING) {
					this->state = CANOPEN_SDO_STATE_SEGMENTED_UPLOAD;
					this->data_index = 0;
					this->mindex = GET_MINDEX(msg);
					this->sindex = GET_SINDEX(msg);
					this->toggle = 0;
					uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
					// initiate segmented domain upload with data size indicated
					SET_CMD_BYTE(&reply_msg,
							INITIATE_DOMAIN_UPLOAD | (1 << 0));
					// data bytes contain the total byte count
					reply_msg.data_32bit[1] = obj.string_len;
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				}

				// expedited transfer
				else {
					// set command byte: expedited transfer, size indicated,
					// object type len set correctly
					SET_CMD_BYTE(&reply_msg,
							INITIATE_DOMAIN_UPLOAD | (1 << 1) | (1 << 0) |
							((4 - CANOPEN_TYPE_LEN(obj.type)) << 2));
					_canopen_copy_data(&reply_msg, &obj, GET_SINDEX(msg));
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				}
			}
		}
		else {
			sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
					CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
		}
	}

	// segmented upload
	else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_UPLOAD) &&
			(sdo_type == UPLOAD_DOMAIN_SEGMENT)) {

		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		SET_MINDEX(&reply_msg, this->mindex);
		SET_SINDEX(&reply_msg, this->sindex);
		if (_canopen_find_object(&reply_msg, &obj, CANOPEN_RO)) {
			// check toggle bit
			if (((GET_CMD_BYTE(msg) & (1 << 4)) >> 4) == this->toggle) {
				uint8_t data_count = uv_mini(obj.string_len - this->data_index, 7);
				// transmission continues
				if (obj.string_len - this->data_index > 7) {
					SET_CMD_BYTE(&reply_msg, UPLOAD_DOMAIN_SEGMENT_REPLY |
							(this->toggle << 4));
				}
				// last message
				else {
					SET_CMD_BYTE(&reply_msg, UPLOAD_DOMAIN_SEGMENT_REPLY |
							(this->toggle << 4) | (7 - data_count) | (1 << 0));
					this->state = CANOPEN_SDO_STATE_READY;
				}
				memset(&reply_msg.data_8bit[1], 0, 7);
				memcpy(&reply_msg.data_8bit[1],
						((uint8_t*) obj.data_ptr) + this->data_index, data_count);
				this->data_index += data_count;
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				this->toggle = !this->toggle;

			}
			else {
				sdo_server_abort(this->mindex, this->sindex,
						CANOPEN_SDO_ERROR_GENERAL);
			}
		}
	}

	// segmented download
	else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD) &&
			(sdo_type == DOWNLOAD_DOMAIN_SEGMENT)) {

		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		SET_MINDEX(&reply_msg, this->mindex);
		SET_SINDEX(&reply_msg, this->sindex);
		if (_canopen_find_object(&reply_msg, &obj, CANOPEN_WO)) {
			// check toggle bit
			if (((GET_CMD_BYTE(msg) & (1 << 4)) >> 4) == this->toggle) {
				// last segment
				if (GET_CMD_BYTE(msg) & (1 << 0)) {
					this->state = CANOPEN_SDO_STATE_READY;
				}
				uint8_t data_count = 7 - ((GET_CMD_BYTE(msg) & 0b1110) >> 1);
				if ((this->data_index + data_count) <= obj.string_len) {
					// copy data to destination. Bits 1-4 in command byte indicate
					// how much data is copied
					memcpy(((uint8_t*)obj.data_ptr) + this->data_index,
							&msg->data_8bit[1], data_count);
					this->data_index += data_count;

					SET_CMD_BYTE(&reply_msg, DOWNLOAD_DOMAIN_SEGMENT_REPLY | (this->toggle << 4));
					memset(&reply_msg.data_8bit[1], 0, 7);
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					this->toggle = !this->toggle;
				}
				else {
					sdo_server_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
				}
			}
			else {
				sdo_server_abort(this->mindex, this->sindex,
						CANOPEN_SDO_ERROR_GENERAL);
			}
		}
	}
	else {
		sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
				CANOPEN_SDO_ERROR_OBJECT_ACCESS_FAILED_DUE_TO_HARDWARE);
		printf("sdo type: %u\n", sdo_type);
	}
}















#endif













