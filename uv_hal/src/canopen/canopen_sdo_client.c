/*
 * canopen_sdo_client.c
 *
 *  Created on: Aug 27, 2017
 *      Author: usevolt
 */


#include "canopen/canopen_sdo_client.h"
#include "uv_canopen.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#include <string.h>


#if CONFIG_CANOPEN

#define this (&_canopen.sdo.client)
#define NODEID			(CONFIG_NON_VOLATILE_START.id)


#define GET_CMD_BYTE(msg_ptr)			((msg_ptr)->data_8bit[0])
#define GET_MINDEX(msg_ptr)				((msg_ptr)->data_8bit[1] + ((msg_ptr)->data_8bit[2] * 256))
#define GET_SINDEX(msg_ptr)				((msg_ptr)->data_8bit[3])
#define GET_NODEID(msg_ptr)				((msg_ptr)->id & 0x7F)
#define SET_CMD_BYTE(msg_ptr, value)	(msg_ptr)->data_8bit[0] = (value)
#define SET_MINDEX(msg_ptr, value)		do {(msg_ptr)->data_8bit[1] = (value) % 256; \
										(msg_ptr)->data_8bit[2] = (value) / 256; } while (0)
#define SET_SINDEX(msg_ptr, value)		(msg_ptr)->data_8bit[3] = (value)



/// @brief: Send a SDO Client abort response message
static inline void sdo_client_abort(uint16_t main_index,
				uint8_t sub_index, uv_sdo_error_codes_e err_code) {
	_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, main_index, sub_index, err_code);
}


void _uv_canopen_sdo_client_init(void) {
	this->state = CANOPEN_SDO_STATE_READY;
	this->delay = -1;
#if CONFIG_TARGET_LPC1785
#warning "Note: On LPC1785 receiving SDO server response messages have to be configured individually"
#else
	// configure to receive all SDO response messages
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID, ~0x7F, CAN_STD);
#endif

}

void _uv_canopen_sdo_client_reset(void) {

}

void _uv_canopen_sdo_client_step(uint16_t step_ms) {
	if ((this->state != CANOPEN_SDO_STATE_READY) &&
			uv_delay(step_ms, &this->delay)) {
		sdo_client_abort(this->mindex, this->sindex, CANOPEN_SDO_ERROR_SDO_PROTOCOL_TIMED_OUT);
		this->state = CANOPEN_SDO_STATE_READY;
	}
}

void _uv_canopen_sdo_client_rx(const uv_can_message_st *msg,
		sdo_request_type_e sdo_type, uint8_t node_id) {

	if ((this->state != CANOPEN_SDO_STATE_READY) &&
			(GET_NODEID(msg) == this->server_node_id) &&
			(GET_MINDEX(msg) == this->mindex) &&
			(GET_SINDEX(msg) == this->sindex)) {
		// aborted transfers
		if (sdo_type == ABORT_DOMAIN_TRANSFER) {
			this->state = CANOPEN_SDO_STATE_TRANSFER_ABORTED;
		}
		// reply to expedited downloads
		else if ((this->state == CANOPEN_SDO_STATE_EXPEDITED_DOWNLOAD) &&
				(sdo_type == INITIATE_DOMAIN_DOWNLOAD_REPLY)) {
			// transfer done
			this->state = CANOPEN_SDO_STATE_READY;
		}
		else if ((this->state == CANOPEN_SDO_STATE_EXPEDITED_UPLOAD) &&
				(sdo_type == INITIATE_DOMAIN_UPLOAD)) {
			if (this->data_ptr) {
				// copy data to destination
				memcpy(this->data_ptr, &msg->data_32bit[1],
						4 - ((GET_CMD_BYTE(msg) & 0xC) >> 2));
			}
			else {
				sdo_client_abort(this->mindex, this->sindex, CANOPEN_SDO_ERROR_GENERAL);
			}
			// transfer done
			this->state = CANOPEN_SDO_STATE_READY;
		}
	}
}

/// @brief: Sends a CANOpen SDO write request without waiting for the response
uv_errors_e _uv_canopen_sdo_client_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data) {
	uv_errors_e ret = ERR_NONE;
	uv_can_msg_st msg;
	msg.type = CAN_STD;
	msg.id = CANOPEN_SDO_REQUEST_ID + node_id;
	msg.data_length = 8;
	memset(&msg.data_32bit[1], 0, 4);
	SET_MINDEX(&msg, mindex);
	SET_SINDEX(&msg, sindex);

	if (this->state != CANOPEN_SDO_STATE_READY) {
		ret = ERR_HW_BUSY;
	}
	else {
		this->server_node_id = node_id;
		this->mindex = mindex;
		this->sindex = sindex;
		uv_delay_init(CONFIG_CANOPEN_SDO_TIMEOUT_MS, &this->delay);

		if (data_len <= 4) {
			// expedited write
			this->state = CANOPEN_SDO_STATE_EXPEDITED_DOWNLOAD;
			SET_CMD_BYTE(&msg, INITIATE_DOMAIN_DOWNLOAD | 0b11 | ((4 - data_len) << 2));
			memcpy(&msg.data_32bit[1], data, data_len);
			uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);

			// wait for reply
			while ((this->state != CANOPEN_SDO_STATE_READY) &&
					(this->state != CANOPEN_SDO_STATE_TRANSFER_ABORTED)) {
				uv_rtos_task_yield();
			}

			if (this->state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
				this->state = CANOPEN_SDO_STATE_READY;
				ret = ERR_ABORTED;
			}

		}
		else {
			// segmented write
			this->state = CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD;
			// todo: implementation missing
		}
	}


	return ret;
}




/// @brief: Sends a CANOpen SDO read request and waits for the response
/// **timeout_ms** milliseconds. If the read request failed or the timeout
/// expires, returns an error.
uv_errors_e _uv_canopen_sdo_client_read(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data) {
	uv_errors_e ret = ERR_NONE;
	uv_can_msg_st msg;
	msg.type = CAN_STD;
	msg.data_length = 8;
	msg.id = CANOPEN_SDO_REQUEST_ID + node_id;
	memset(&msg.data_32bit[1], 0, 4);
	SET_MINDEX(&msg, mindex);
	SET_SINDEX(&msg, sindex);

	if (this->state != CANOPEN_SDO_STATE_READY) {
		ret = ERR_HW_BUSY;
	}
	else {
		this->server_node_id = node_id;
		this->mindex = mindex;
		this->sindex = sindex;
		this->data_ptr = data;
		uv_delay_init(CONFIG_CANOPEN_SDO_TIMEOUT_MS, &this->delay);

		if (data_len <= 4) {
			// expedited read
			this->state = CANOPEN_SDO_STATE_EXPEDITED_UPLOAD;
			SET_CMD_BYTE(&msg, INITIATE_DOMAIN_UPLOAD);
			uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);

			// wait for reply
			while ((this->state != CANOPEN_SDO_STATE_READY) &&
					(this->state != CANOPEN_SDO_STATE_TRANSFER_ABORTED)) {
				uv_rtos_task_yield();
			}

			if (this->state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
				this->state = CANOPEN_SDO_STATE_READY;
				ret = ERR_ABORTED;
			}
			// data should now be copied and transfer is finished

		}
		else {
			// segmented read
			this->state = CANOPEN_SDO_STATE_SEGMENTED_UPLOAD;
			// todo: implementation missing
		}
	}


	return ret;
}



#endif
