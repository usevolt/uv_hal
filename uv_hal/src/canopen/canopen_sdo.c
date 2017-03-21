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
#include CONFIG_MAIN_H

#define this (&_canopen)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			(CONFIG_NON_VOLATILE_START.id)


#define GET_CMD_BYTE(msg_ptr)			((msg_ptr)->data_8bit[0])
#define GET_MINDEX(msg_ptr)				((msg_ptr)->data_8bit[1] + ((msg_ptr)->data_8bit[2] * 256))
#define GET_SINDEX(msg_ptr)				((msg_ptr)->data_8bit[3])
#define GET_NODEID(msg_ptr)				((msg_ptr)->id & 0x7F)
#define SET_CMD_BYTE(msg_ptr, value)	(msg_ptr)->data_8bit[0] = (value)
#define SET_MINDEX(msg_ptr, value)		(msg_ptr)->data_8bit[1] = (value) % 256; \
										(msg_ptr)->data_8bit[2] = (value) / 256
#define SET_SINDEX(msg_ptr, value)		(msg_ptr)->data_8bit[3] = (value)


enum {
	INVALID_MSG = 0,
	UNKNOWN_SDO_MSG,
	ABORT_MSG,
	INITIATE_WRITE_EXPEDITED_MSG,
	INITIATE_WRITE_SEGMENTED_MSG,
	INITIATE_READ_MSG
};
typedef uint8_t sdo_request_type_e;


static inline bool sdo_transfer_finished() {
	return this->sdo.state & CANOPEN_SDO_STATE_FINISHED_MASK;
}

static sdo_request_type_e get_request_type(const uv_can_message_st *msg, int32_t *byte_count) {
	if (msg->data_length != 8) {
		return UNKNOWN_SDO_MSG;
	}
	// initiate domain download message
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == 0b100000) {
		// expedited transfer
		if (GET_CMD_BYTE(msg) & (1 << 1)) {
			if (GET_CMD_BYTE(msg) & (1 << 0)) {
				*byte_count = 4 - ((GET_CMD_BYTE(msg) & (0b1100)) >> 2);
			}
			else {
				*byte_count = -1;
			}
			return INITIATE_WRITE_EXPEDITED_MSG;
		}
		// segmented transfer
		else {
			// for segmented transfer 's' bit should be one
			if (GET_CMD_BYTE(msg) & (1 << 0)) {
				*byte_count = msg->data_32bit[1];
			}
			else {
				return UNKNOWN_SDO_MSG;
			}
			return INITIATE_WRITE_SEGMENTED_MSG;
		}
	}
	// initiate domain upload
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == 0b1000000) {
		*byte_count = -1;
		return INITIATE_READ_MSG;
	}
	else if ((GET_CMD_BYTE(msg) & 0b11100000) == 0x80) {
		// abort
		return ABORT_MSG;
	}
	else {
		return UNKNOWN_SDO_MSG;
	}
}



void _uv_canopen_sdo_init(void) {
#if CONFIG_TARGET_LPC1785
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_REQUEST_ID + NODEID, CAN_STD);
#else
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_REQUEST_ID + NODEID, CAN_ID_MASK_DEFAULT, CAN_STD);
#endif

	this->sdo.state = CANOPEN_SDO_STATE_READY;
}

void _uv_canopen_sdo_reset(void) {

}

void _uv_canopen_sdo_step(uint16_t step_ms) {

}


static bool find_object(const uv_can_message_st *msg,
		canopen_object_st *obj, canopen_permissions_e permission_req) {
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
			return false;
		}
	}
	else {
		// couldn't find the requested object
		_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, GET_MINDEX(msg), GET_SINDEX(msg),
				CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
		return false;
	}
	return true;
}

void _uv_canopen_sdo_rx(const uv_can_message_st *msg) {


#if CONFIG_CANOPEN_SDO_SYNC
	/* SDO responses from other nodes are parsed only if there was an active transfer*/
	if (GET_NODEID(msg) != NODEID && !sdo_transfer_finished() && msg->data_length > 4) {

		// check that this SDO was actually from the currently open active transfer
		if (GET_MINDEX(msg) == this->sdo.transfer.mindex &&
				GET_SINDEX(msg) == this->sdo.transfer.sindex &&
				GET_NODEID(msg) == this->sdo.transfer.node_id) {
			/*
			 * ABORT SDO TRANSFER
			 */
			if (GET_CMD_BYTE(msg) == 0x80) {
				this->sdo.state = CANOPEN_SDO_STATE_TRANSFER_ABORTED;
			}
			/*
			 * EXPEDITED WRITE
			 */

			else if (this->sdo.state == CANOPEN_SDO_STATE_EXPEDITED_WRITE) {
				// write done
				this->sdo.state = CANOPEN_SDO_STATE_TRANSFER_DONE;
			}
			/*
			 * EXPEDITED READ
			 */
			else if (this->sdo.state == CANOPEN_SDO_STATE_EXPEDITED_READ) {
				uint8_t data_len = uv_mini(msg->data_length - 4, this->sdo.transfer.rx_data_len);
				if (GET_CMD_BYTE(msg) & (1 << 0)) {
					data_len = uv_mini(4 - ((GET_CMD_BYTE(msg) & 0b1100) >> 2), data_len);
				}
				memcpy(this->sdo.transfer.rx_data, &msg->data_8bit[4], data_len);
				this->sdo.state = CANOPEN_SDO_STATE_TRANSFER_DONE;
			}
#if CONFIG_CANOPEN_SDO_SEGMENTED
			/*
			 * SEGMENTED WRITE
			 */
			else if (this->sdo.state == CANOPEN_SDO_STATE_SEGMENTED_READ) {

			}
			/*
			 * SEGMENTED READ
			 */
			else if (this->sdo.state == CANOPEN_SDO_STATE_SEGMENTED_READ) {

			}
#endif

		}


	}
#endif

	// from this point onward, only messages addressed to this device are parsed
	if (GET_NODEID(msg) != NODEID) {
		return;
	}

	int32_t byte_len = 0;
	sdo_request_type_e type = get_request_type(msg, &byte_len);


	if (type == ABORT_MSG) {
		uv_can_message_st response;
		response.type = CAN_STD;
		response.id = CANOPEN_SDO_RESPONSE_ID + NODEID;
		response.data_length = 8;
		memcpy(response.data_8bit, msg->data_8bit, 8);
		uv_can_send_message(CONFIG_CANOPEN_CHANNEL, &response);

		// abort transfer if it is addressed to us
		__disable_irq();
		if (this->sdo.state != CANOPEN_SDO_STATE_READY) {
			if (GET_NODEID(msg) == this->sdo.transfer.node_id &&
					GET_MINDEX(msg) == this->sdo.transfer.mindex &&
					GET_SINDEX(msg) == this->sdo.transfer.sindex) {
				this->sdo.state = CANOPEN_SDO_STATE_TRANSFER_ABORTED;
			}
		}
		__enable_irq();
	}
	/*
	 * CANOPEN READ REQUESTS
	 */
	else if (type == INITIATE_READ_MSG) {
		canopen_object_st obj;
		if (find_object(msg, &obj, CANOPEN_RO)) {

			if (uv_canopen_is_string(&obj)) {
				__disable_irq();
				if (this->sdo.state == CANOPEN_SDO_STATE_READY) {
					this->sdo.state = CANOPEN_SDO_STATE_SEGMENTED_READ;
					this->sdo.transfer.mindex = GET_MINDEX(msg);
					this->sdo.transfer.sindex = GET_SINDEX(msg);
					this->sdo.transfer.node_id = NODEID;
					__enable_irq();
#if CONFIG_CANOPEN_SDO_SEGMENTED

					// todo: initiate a segmented read transfer for string type object
#endif

				}
				else {
					__enable_irq();
					_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, obj.main_index, obj.sub_index,
							CANOPEN_SDO_ERROR_OBJECT_ACCESS_FAILED_DUE_TO_HARDWARE);
				}
			}
			else {
				// respond to an expedited read transfer
				uv_can_message_st response;
				response.type = CAN_STD;
				response.id = CANOPEN_SDO_RESPONSE_ID + NODEID;
				response.data_length = 8;
				memset(response.data_8bit, 0, 8);

				SET_CMD_BYTE(&response, (1 << 6) | (1 << 1) | (1 << 0) |
						((4 - uv_canopen_get_object_data_size(&obj)) << 2));
				SET_MINDEX(&response, obj.main_index);
				// note: subindex is taken from the request message since array type objects
				// differ in subindex handling
				SET_SINDEX(&response, GET_SINDEX(msg));

				// if ojbect is array, subindex 0 returns the array size
				if (uv_canopen_is_array(&obj) && !GET_SINDEX(msg)) {
					response.data_8bit[4] = obj.array_max_size;
				}
				else {
					// copy data bytes to response message from the object
					for (int8_t i = 0; i < uv_canopen_get_object_data_size(&obj); i++) {
						if (uv_canopen_is_array(&obj)) {
							response.data_8bit[4 + i] = ((uint8_t*) obj.data_ptr)
									[(GET_SINDEX(msg) - 1) * uv_canopen_get_object_data_size(&obj) + i];
						}
						else {
							response.data_8bit[4 + i] = ((uint8_t*) obj.data_ptr)[i];
						}
					}
				}
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &response);
				return;
			}
		}
	}
	/*
	 * CANOPEN EXPEDITED WRITE REQUEST
	 */
	else if (type == INITIATE_WRITE_EXPEDITED_MSG) {
		canopen_object_st obj;
		if (find_object(msg, &obj, CANOPEN_WO)) {
			// check that requested data length matches
			if (GET_CMD_BYTE(msg) & (1 << 0)) {
				if (uv_canopen_get_object_data_size(&obj) !=
						4 - ((GET_CMD_BYTE(msg) & 0b1100) >> 2)) {
					_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, GET_MINDEX(msg), GET_SINDEX(msg),
							CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
					return;
				}
			}
			// if object is an array, writing to subindex 0 is prohibited
			if (uv_canopen_is_array(&obj) && !GET_SINDEX(msg)) {
				_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, GET_MINDEX(msg), GET_SINDEX(msg),
						CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
				return;
			}
			// if object is a string, start segmented write transfer
			else if (uv_canopen_is_string(&obj)) {
				__disable_irq();
				if (this->sdo.state == CANOPEN_SDO_STATE_READY) {
					this->sdo.state = CANOPEN_SDO_STATE_SEGMENTED_WRITE;
					this->sdo.transfer.mindex = GET_MINDEX(msg);
					this->sdo.transfer.sindex = GET_SINDEX(msg);
					this->sdo.transfer.node_id = NODEID;
					__enable_irq();

#if CONFIG_CANOPEN_SDO_SEGMENTED
					// todo: respond to segmented transfer

#endif
					return;
				}
				else {
					__enable_irq();
					// busy, only 1 segmented transfer can be active at one time
					_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, obj.main_index, obj.sub_index,
							CANOPEN_SDO_ERROR_OBJECT_ACCESS_FAILED_DUE_TO_HARDWARE);
					return;
				}
			}

			// copy data to destination object
			for (int8_t i = 0; i < uv_canopen_get_object_data_size(&obj); i++) {
				if (uv_canopen_is_array(&obj)) {
					((uint8_t*) obj.data_ptr)
							[(GET_SINDEX(msg) - 1) * uv_canopen_get_object_data_size(&obj) + i] =
									msg->data_8bit[4 + i];
				}
				else {
					((uint8_t*) obj.data_ptr)[i] = msg->data_8bit[4 + i];
				}
			}

			// send a response message from a successful expedited write
			uv_can_message_st response;
			response.id = CANOPEN_SDO_RESPONSE_ID + NODEID;
			response.type = CAN_STD;
			response.data_length = 8;
			memset(response.data_8bit, 0, 8);
			SET_CMD_BYTE(&response, 0x60);
			SET_MINDEX(&response, obj.main_index);
			SET_SINDEX(&response, GET_SINDEX(msg));
			memcpy(&response.data_8bit[4], &msg->data_8bit[4],
					uv_canopen_get_object_data_size(&obj));
			uv_can_send(CONFIG_CANOPEN_CHANNEL, &response);
		}
	}
	/*
	 * CANOPEN SEGMENTED WRITE REQUEST
	 */
#if CONFIG_CANOPEN_SDO_SEGMENTED
	else if (type == INITIATE_WRITE_SEGMENTED_MSG) {

		// todo: segmented transfer needs to be implemented

	}
#endif
	else if (type == UNKNOWN_SDO_MSG) {
		_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, GET_MINDEX(msg), GET_SINDEX(msg),
				CANOPEN_SDO_ERROR_CMD_SPECIFIER_NOT_FOUND);
	}
}




uv_errors_e _uv_canopen_sdo_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data) {
	uv_can_message_st msg = {
			.type = CAN_STD,
			.data_length = 8
	};
	msg.id = CANOPEN_SDO_REQUEST_ID + node_id;
	memset(msg.data_8bit, 0, 8);
	if (data_len <= 4) {
		SET_CMD_BYTE(&msg, (1 << 5) |
				(1 << 1) | (1 << 0) | ((4 - data_len) << 2));
		SET_MINDEX(&msg, mindex);
		SET_SINDEX(&msg, sindex);
		memcpy(&msg.data_8bit[4], data, data_len);
	}
	else {
#if CONFIG_CANOPEN_SDO_SEGMENTED
		// todo: start a SDO segmented write

#endif
		return uv_err(ERR_NOT_IMPLEMENTED);
	}

	return uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
}


#if CONFIG_CANOPEN_SDO_SYNC

uv_errors_e _uv_canopen_sdo_write_sync(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data_len, void *data, int32_t timeout_ms) {
	while (true) {
		__disable_irq();
		if (this->sdo.state == CANOPEN_SDO_STATE_READY) {
			if (data_len <= 4) {
				this->sdo.state = CANOPEN_SDO_STATE_EXPEDITED_WRITE;
			}
			else {
				this->sdo.state = CANOPEN_SDO_STATE_SEGMENTED_WRITE;
			}
			this->sdo.transfer.mindex = mindex;
			this->sdo.transfer.sindex = sindex;
			this->sdo.transfer.node_id = node_id;
			__enable_irq();
			break;
		}
		__enable_irq();

		if (timeout_ms > 0) {
			timeout_ms--;
		}
		else {
			return ERR_HW_BUSY;
		}
		uv_rtos_task_delay(1);
	}

	// make sure that CAN module is configured to receive response from this node
#if CONFIG_TARGET_LPC1785
	uv_errors_e e = uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID + node_id, CAN_STD);
#else
	uv_errors_e e = uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID + node_id, CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
	if (e) {
		return e;
	}

	_uv_canopen_sdo_write(node_id, mindex, sindex, data_len, data);

	while (true) {
		__disable_irq();
		if (this->sdo.state == CANOPEN_SDO_STATE_TRANSFER_DONE) {
			// got response
			this->sdo.state = CANOPEN_SDO_STATE_READY;
			__enable_irq();
			return ERR_NONE;
		}
		else if (this->sdo.state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
			this->sdo.state = CANOPEN_SDO_STATE_READY;
			__enable_irq();
			return ERR_ABORTED;
		}
		__enable_irq();

		if (timeout_ms > 0) {
			timeout_ms--;
		}
		else {
			this->sdo.state = CANOPEN_SDO_STATE_READY;
			return ERR_NOT_RESPONDING;
		}
		uv_rtos_task_delay(1);
	}
}




uv_errors_e _uv_canopen_sdo_read_sync(uint8_t node_id, uint16_t mindex,
		uint8_t sindex, uint32_t data_len, void *data, int32_t timeout_ms) {
	while (true) {
		__disable_irq();
		if (this->sdo.state == CANOPEN_SDO_STATE_READY) {
			if (data_len <= 4) {
				this->sdo.state = CANOPEN_SDO_STATE_EXPEDITED_READ;
			}
			else {
				this->sdo.state = CANOPEN_SDO_STATE_SEGMENTED_READ;
			}
			this->sdo.transfer.mindex = mindex;
			this->sdo.transfer.sindex = sindex;
			this->sdo.transfer.node_id = node_id;
			this->sdo.transfer.rx_data = data;
			this->sdo.transfer.rx_data_len = data_len;
			__enable_irq();
			break;
		}
		__enable_irq();

		if (timeout_ms > 0) {
			timeout_ms--;
		}
		else {
			return ERR_HW_BUSY;
		}
		uv_rtos_task_delay(1);
	}
	// make sure that CAN module is configured to receive response from this node
#if CONFIG_TARGET_LPC1785
	uv_errors_e e = uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID + node_id, CAN_STD);
#else
	uv_errors_e e = uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID + node_id, CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
	if (e) {
		return e;
	}

	// send SDO read request
	uv_can_message_st msg = {
			.type = CAN_STD,
			.data_length = 8
	};
	msg.id = CANOPEN_SDO_REQUEST_ID + node_id;
	if (data_len <= 4) {
		SET_CMD_BYTE(&msg, (1 << 6));
		SET_MINDEX(&msg, mindex);
		SET_SINDEX(&msg, sindex);
		memset(&msg.data_8bit[4], 0, 4);
		uv_errors_e e = uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
		if (e) {
			return e;
		}

	}
	else {
#if CONFIG_CANOPEN_SDO_SEGMENTED
		// todo: send segmented read transfer start message

#endif
	}

	while (true) {
		__disable_irq();
		if (this->sdo.state == CANOPEN_SDO_STATE_TRANSFER_DONE) {
			// got response
			this->sdo.state = CANOPEN_SDO_STATE_READY;
			__enable_irq();
			return ERR_NONE;
		}
		else if (this->sdo.state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
			this->sdo.state = CANOPEN_SDO_STATE_READY;
			__enable_irq();
			return ERR_ABORTED;
		}

		if (timeout_ms > 0) {
			timeout_ms--;
		}
		else {
			this->sdo.state = CANOPEN_SDO_STATE_READY;
			__enable_irq();
			return ERR_NOT_RESPONDING;
		}
		__enable_irq();
		uv_rtos_task_delay(1);
	}

}
#endif



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
