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


#include "canopen/canopen_sdo_client.h"
#include "uv_canopen.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#include <string.h>
#include CONFIG_MAIN_H


#if CONFIG_CANOPEN

#define this (&_canopen.sdo.client)
#define NODEID			this->current_node_id


#define GET_CMD_BYTE(msg_ptr)			((msg_ptr)->data_8bit[0])
#define GET_MINDEX(msg_ptr)				((msg_ptr)->data_8bit[1] + ((msg_ptr)->data_8bit[2] * 256))
#define GET_SINDEX(msg_ptr)				((msg_ptr)->data_8bit[3])
#define GET_NODEID(msg_ptr)				((msg_ptr)->id & 0x7F)
#define SET_CMD_BYTE(msg_ptr, value)	(msg_ptr)->data_8bit[0] = (value)
#define SET_MINDEX(msg_ptr, value)		do {(msg_ptr)->data_8bit[1] = (value) % 256; \
										(msg_ptr)->data_8bit[2] = (value) / 256; } while (0)
#define SET_SINDEX(msg_ptr, value)		(msg_ptr)->data_8bit[3] = (value)
#define BLKSIZE()						((CONFIG_CANOPEN_SDO_BLOCK_SIZE % 7) ? \
										(CONFIG_CANOPEN_SDO_BLOCK_SIZE / 7 + 1) : \
										(CONFIG_CANOPEN_SDO_BLOCK_SIZE / 7))



/// @brief: Send a SDO Client abort response message
static inline void sdo_client_abort(uint16_t main_index,
				uint8_t sub_index, uv_sdo_error_codes_e err_code) {
	_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, main_index, sub_index, err_code);
	this->last_err_code = err_code;
	this->state = CANOPEN_SDO_STATE_TRANSFER_ABORTED;
}


void _uv_canopen_sdo_client_init(void) {
	this->state = CANOPEN_SDO_STATE_READY;
	this->delay = -1;
	this->last_err_code = CANOPEN_SDO_ERROR_NONE;
	this->wait_callb = NULL;
	this->wait_callb_req = false;
	uv_delay_init(&this->wait_delay, CANOPEN_SDO_CLIENT_WAIT_CALLB_DELAY_MS);
}

void _uv_canopen_sdo_client_reset(void) {

}

void _uv_canopen_sdo_client_step(uint16_t step_ms) {
	if (this->state != CANOPEN_SDO_STATE_READY) {
		// abort delay logic
		if (uv_delay(&this->delay, step_ms)) {
			if (this->state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
				this->state = CANOPEN_SDO_STATE_READY;
			}
			else {
				sdo_client_abort(this->mindex, this->sindex, CANOPEN_SDO_ERROR_SDO_PROTOCOL_TIMED_OUT);
				this->state = CANOPEN_SDO_STATE_TRANSFER_ABORTED;
				uv_delay_init(&this->delay, step_ms);
			}
		}
		// wait callback function logic
		if (uv_delay(&this->wait_delay, step_ms)) {
			// request to call wait callback if one is assigned
			this->wait_callb_req = true;
			uv_delay_init(&this->wait_delay, CANOPEN_SDO_CLIENT_WAIT_CALLB_DELAY_MS);
		}
	}
	else {
		uv_delay_init(&this->wait_delay, CANOPEN_SDO_CLIENT_WAIT_CALLB_DELAY_MS);
		this->wait_callb_req = false;
	}
}



void _uv_canopen_sdo_client_rx(const uv_can_message_st *msg,
		sdo_request_type_e sdo_type, uint8_t node_id) {

	uv_can_msg_st reply_msg;
	reply_msg.type = CAN_STD;
	reply_msg.id = CANOPEN_SDO_REQUEST_ID + node_id;
	reply_msg.data_length = 8;
	memset(reply_msg.data_8bit, 0, 8);
	SET_MINDEX(&reply_msg, GET_MINDEX(msg));
	SET_SINDEX(&reply_msg, GET_SINDEX(msg));


	if ((this->state != CANOPEN_SDO_STATE_READY) &&
			(GET_NODEID(msg) == this->server_node_id)) {
		// aborted transfers
		if (sdo_type == ABORT_DOMAIN_TRANSFER) {
			this->state = CANOPEN_SDO_STATE_TRANSFER_ABORTED;
			this->last_err_code = msg->data_32bit[1];
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		}
		// reply to expedited downloads
		else if ((this->state == CANOPEN_SDO_STATE_EXPEDITED_DOWNLOAD) &&
				(sdo_type == INITIATE_DOMAIN_DOWNLOAD_REPLY)) {
			// transfer done
			this->state = CANOPEN_SDO_STATE_READY;
		}
#if CONFIG_CANOPEN_SDO_SEGMENTED
		// start of segmented download
		else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD) &&
				(sdo_type == INITIATE_DOMAIN_DOWNLOAD_REPLY)) {
			int32_t n = 7 - (this->data_count - this->data_index);
			uint8_t c = (n < 0) ? 0 : 1;
			if (n < 0) {
				n = 0;
			}
			SET_CMD_BYTE(&reply_msg, DOWNLOAD_DOMAIN_SEGMENT | (this->toggle << 4) | (n << 1) | c);
			// copy data to message
			c = 0;
			while ((this->data_index < this->data_count) && (c < 7)) {
				reply_msg.data_8bit[1 + c] = ((uint8_t*) this->data_ptr)[this->data_index++];
				c++;
			}
			uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
			this->toggle = !this->toggle;
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		}
		// segmented downloads
		else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD) &&
				(sdo_type == DOWNLOAD_DOMAIN_SEGMENT_REPLY)) {
			if (((GET_CMD_BYTE(msg) & (1 << 4)) >> 4) == this->toggle) {
				sdo_client_abort(this->mindex, this->sindex,
						CANOPEN_SDO_ERROR_SDO_TOGGLE_BIT_NOT_ALTERED);
			}
			else {
				// all data transfered
				if (this->data_index >= this->data_count) {
					this->state = CANOPEN_SDO_STATE_READY;
				}
				// send more data
				else {
					int32_t n = 7 - (this->data_count - this->data_index);
					uint8_t c = (n < 0) ? 0 : 1;
					if (n < 0) {
						n = 0;
					}
					SET_CMD_BYTE(&reply_msg, DOWNLOAD_DOMAIN_SEGMENT | (this->toggle << 4) | (n << 1) | c);
					// copy data to message
					c = 0;
					while ((this->data_index < this->data_count) && (c < 7)) {
						reply_msg.data_8bit[1 + c++] = ((uint8_t*) this->data_ptr)[this->data_index++];
					}
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					this->toggle = !this->toggle;
					uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
				}
			}
		}
		// start of segmented upload
		else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_UPLOAD) &&
				(sdo_type == INITIATE_DOMAIN_UPLOAD)) {

			if (GET_CMD_BYTE(msg) & (1 << 1)) {
				// client returned as expedited transfer, segmented transfer is finished
				memcpy(this->data_ptr, &msg->data_32bit[1],
						4 - ((GET_CMD_BYTE(msg) & (0b11 << 2)) >> 2));
				this->state = CANOPEN_SDO_STATE_READY;
			}
			else {
				// segmented transfer
				if (GET_CMD_BYTE(msg) & (1 << 0)) {
					// data size indicated
					if (msg->data_32bit[1] < this->data_count) {
						this->data_count = msg->data_32bit[1];
					}
				}
				//segmented transfer, send upload domain segment message
				SET_CMD_BYTE(&reply_msg, UPLOAD_DOMAIN_SEGMENT | (this->toggle << 4));
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				this->toggle = !this->toggle;
			}
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		}
		// segmented upload
		else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_UPLOAD) &&
				(sdo_type == UPLOAD_DOMAIN_SEGMENT_REPLY)) {
			bool finished = false;
			// first check the toggle bit
			if (((GET_CMD_BYTE(msg) & (1 << 4)) >> 4) == this->toggle) {
				sdo_client_abort(this->mindex, this->sindex,
						CANOPEN_SDO_ERROR_SDO_TOGGLE_BIT_NOT_ALTERED);
			}
			else {
				uint8_t byte_count = 7 - ((GET_CMD_BYTE(msg) & (0b111 << 1)) >> 1);
				for (uint8_t i = 0; i < byte_count; i++) {
					if (this->data_index < this->data_count) {
						((uint8_t*) this->data_ptr)[this->data_index++] = msg->data_8bit[1 + i];
					}
					else {
						// segmented transfer is finished since we
						// uploaded enough bytes from server
						finished = true;
						break;
					}
				}
				// check if the transfer is finished
				if (GET_CMD_BYTE(msg) & (1 << 0)) {
					this->state = CANOPEN_SDO_STATE_READY;
				}
				else if (finished) {
					this->state = CANOPEN_SDO_STATE_READY;
					// Send an abort message to the server to notify that
					// we ended the transfer
					SET_CMD_BYTE(&reply_msg, ABORT_DOMAIN_TRANSFER);
					reply_msg.data_length = 8;
					reply_msg.data_8bit[1] = this->mindex & 0xFF;
					reply_msg.data_8bit[2] = this->mindex >> 8;
					reply_msg.data_8bit[3] = this->sindex;
					reply_msg.data_32bit[1] = SDO_ABORT_OUT_OF_MEMORY;
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				}
				else {
					// ask for more data
					SET_CMD_BYTE(&reply_msg, UPLOAD_DOMAIN_SEGMENT | (this->toggle << 4));
					memset(&reply_msg.data_8bit[1], 0, 7);
					memcpy(&reply_msg.data_8bit[1], &msg->data_8bit[1],
							uv_mini(msg->data_length, 7));
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					this->toggle = !this->toggle;
				}
			}
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		}
#endif
#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER
		// block downloads
		else if (this->state == CANOPEN_SDO_STATE_BLOCK_DOWNLOAD) {
			// initiate block download
			if (sdo_type == INITIATE_BLOCK_DOWNLOAD_REPLY) {
				uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
				this->crc_enabled = ((GET_CMD_BYTE(msg) & (1 << 2)) >> 2);
				this->server_blksize = msg->data_8bit[4];
				if ((this->server_blksize == 0) || (this->server_blksize >= 128)) {
					sdo_client_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_INVALID_BLOCK_SIZE);
				}
				else {
					// download block segments
					for (uint8_t i = 0; i < this->server_blksize; i++) {
						uint8_t data_len = ((this->data_index + 7) > this->data_count) ?
								(this->data_count - this->data_index) : 7;
						if (data_len != 7) {
							i = this->server_blksize - 1;
						}
						memset(reply_msg.data_8bit, 0, 8);
						this->seq++;
						memcpy(&reply_msg.data_8bit[1], this->data_ptr + this->data_index, data_len);
						this->data_index += data_len;
						SET_CMD_BYTE(&reply_msg, this->seq | ((this->data_index >= this->data_count) << 7));
						uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);

						if ((data_len != 7) || (this->data_index >= this->data_count)) {
							// data transfer finished
							break;
						}
					}
				}
			}
			// download block segment reply
			else if (sdo_type == DOWNLOAD_BLOCK_SEGMENT_REPLY) {
				uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
				if (msg->data_8bit[1] == this->seq) {
					// transfer finished correctly
					if (this->data_index >= this->data_count) {
						// end the transfer
						uint8_t n = 7 - (this->data_count % 7);
						SET_CMD_BYTE(&reply_msg, END_BLOCK_DOWNLOAD | (n << 2));
						uint16_t crc = uv_memory_calc_crc(this->data_ptr, this->data_count);
						reply_msg.data_8bit[1] = crc;
						reply_msg.data_8bit[2] = crc / 256;
						uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					}
					else {
						this->seq = 0;
						// download more data
						for (uint8_t i = 0; i < this->server_blksize; i++) {
							uint8_t data_len = ((this->data_index + 7) > this->data_count) ?
									(this->data_count - this->data_index) : 7;
							if (data_len != 7) {
								i = this->server_blksize - 1;
							}
							memset(reply_msg.data_8bit, 0, 8);
							this->seq++;
							memcpy(&reply_msg.data_8bit[1], this->data_ptr + this->data_index, data_len);
							this->data_index += data_len;
							SET_CMD_BYTE(&reply_msg, this->seq | ((this->data_index >= this->data_count) << 7));
							uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);

							if ((data_len != 7) ||
									(this->data_index >= this->data_count)) {
								// data transfer finished
								break;
							}
						}
						uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
					}
				}
				else {
					sdo_client_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_INVALID_SEQ_NUMBER);
				}
			}
			// end block download
			else if (sdo_type == END_BLOCK_DOWNLOAD_REPLY) {
				this->state = CANOPEN_SDO_STATE_READY;
			}
			else {
				// invalid message received
				sdo_client_abort(this->mindex, this->sindex,
						CANOPEN_SDO_ERROR_GENERAL);
			}

		}
		// block uploads
		else if (this->state == CANOPEN_SDO_STATE_BLOCK_UPLOAD) {

			// initiate block upload reply
			if ((this->seq == 0) && (sdo_type == INITIATE_BLOCK_UPLOAD_REPLY)) {
				this->seq = 1;
				this->crc_enabled = GET_CMD_BYTE(msg) & (1 << 2);
				if (msg->data_32bit[1] > this->data_count) {
					sdo_client_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_OUT_OF_MEMORY);
				}
				else {
					SET_CMD_BYTE(&reply_msg, INITIATE_BLOCK_UPLOAD_REPLY2);
					reply_msg.data_8bit[4] = BLKSIZE();
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				}
			}
			// end block upload
			else if ((this->seq == 1) && (sdo_type == END_BLOCK_UPLOAD)) {
				uint8_t n = 7 - ((GET_CMD_BYTE(msg) & (0b111 << 2)) >> 2);
				// copy the last set of data to destination address.
				// Note: No need to check if there is new data, as there always should be.
				if (this->data_index + n <= this->data_count) {
					memcpy(this->data_ptr + this->data_index, this->data_buffer, n);
					this->data_index += n;
				}
				else {
					sdo_client_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_OUT_OF_MEMORY);
				}
				bool done = true;
				if (this->crc_enabled) {
					uint16_t crc = uv_memory_calc_crc(this->data_ptr, this->data_index);
					uint16_t server_crc = msg->data_8bit[1] + (msg->data_8bit[2] * 256);
					if (crc != server_crc) {
						sdo_client_abort(this->mindex, this->sindex,
								CANOPEN_SDO_ERROR_CRC_ERROR);
						done = false;
					}
				}
				if (done) {
					// transfer finished successfully
					SET_CMD_BYTE(&reply_msg, END_BLOCK_UPLOAD_REPLY);
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					this->state = CANOPEN_SDO_STATE_READY;
				}
			}
			// upload block segment
			else {
				// check the sequence number
				if ((GET_CMD_BYTE(msg) & 0x7F) == this->seq) {
					// copy last message data to destination, if new data is available in the buffer
					if (this->new_data) {
						if ((this->data_index + 7) <= this->data_count) {
							memcpy(this->data_ptr + this->data_index, this->data_buffer, 7);
							this->data_index += 7;
							this->new_data = false;
						}
						else {
							sdo_client_abort(this->mindex, this->sindex,
									CANOPEN_SDO_ERROR_OUT_OF_MEMORY);
						}
					}
					// copy new data to the buffer
					memcpy(this->data_buffer, &msg->data_8bit[1], 7);
					this->new_data = true;
					// increase sequence number
					this->seq++;

					// check if this was the last message of this block
					if (GET_CMD_BYTE(msg) & (1 << 7)) {
						// reply to block and get ready for a new one
						SET_CMD_BYTE(&reply_msg, UPLOAD_BLOCK_SEGMENT_REPLY);
						reply_msg.data_8bit[1] = this->seq;
						reply_msg.data_8bit[2] = BLKSIZE();
						uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
						this->seq = 1;
					}
				}
				else {
					// wrong sequence number
					sdo_client_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_INVALID_SEQ_NUMBER);
				}
			}
		}
#endif
		else {

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

	if (node_id != _canopen.current_node_id) {
		if (this->state != CANOPEN_SDO_STATE_READY) {
			ret = ERR_HW_BUSY;
		}
		else {
			this->server_node_id = node_id;
			this->mindex = mindex;
			this->sindex = sindex;
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);

			if (data_len <= 4) {
				// expedited write
				// if the data_len is given, size is indicated. Otherwise
				// 4 bytes of data is copied from *data* to the message and the
				// SDO receiver (server) is responsible to read only the amount of
				// bytes that it requires.
				this->state = CANOPEN_SDO_STATE_EXPEDITED_DOWNLOAD;
				if (data_len == 0) {
					SET_CMD_BYTE(&msg, INITIATE_DOMAIN_DOWNLOAD | 0b10);
				}
				else {
					SET_CMD_BYTE(&msg, INITIATE_DOMAIN_DOWNLOAD | 0b11 | ((4 - data_len) << 2));
				}
				memcpy(&msg.data_32bit[1], data, (data_len == 0) ? 4 : data_len);
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
			}
			else {
#if CONFIG_CANOPEN_SDO_SEGMENTED
				// segmented write
				this->state = CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD;
				this->data_count = data_len;
				this->data_index = 0;
				this->data_ptr = data;
				this->toggle = 0;
				SET_CMD_BYTE(&msg, INITIATE_DOMAIN_DOWNLOAD | (1 << 0));
				// data count indicated in the data bytes
				msg.data_32bit[1] = this->data_count;
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);
#endif
			}
			// wait for transfer to finish
			while ((this->state != CANOPEN_SDO_STATE_READY) &&
					(this->state != CANOPEN_SDO_STATE_TRANSFER_ABORTED)) {
				// check wait callback request and call it
				if (this->wait_callb_req && this->wait_callb) {
					this->wait_callb_req = false;
					this->wait_callb(this->mindex, this->sindex);
				}
				uv_rtos_task_yield();
			}

			if (this->state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
				this->state = CANOPEN_SDO_STATE_READY;
				ret = ERR_ABORTED;
			}
		}
	}
	else {
		// todo: write locally to obj dict
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
	this->server_node_id = node_id;
	this->mindex = mindex;
	this->sindex = sindex;
	this->data_ptr = data;
	this->data_index = 0;
	this->data_count = data_len;
	this->toggle = 0;

	if (node_id != _canopen.current_node_id) {

		if (this->state != CANOPEN_SDO_STATE_READY) {
			ret = ERR_HW_BUSY;
		}
		else {
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);

			// just in case put us to segmented upload state.
			// expedited answers from server are handled as well
			this->state = CANOPEN_SDO_STATE_SEGMENTED_UPLOAD;
			SET_CMD_BYTE(&msg, INITIATE_DOMAIN_UPLOAD);
			uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);

			// wait for reply
			while ((this->state != CANOPEN_SDO_STATE_READY) &&
					(this->state != CANOPEN_SDO_STATE_TRANSFER_ABORTED)) {
				// check wait callback request and call it
				if (this->wait_callb_req && this->wait_callb) {
					this->wait_callb_req = false;
					this->wait_callb(this->mindex, this->sindex);
				}
				uv_rtos_task_yield();
			}

			if (this->state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
				this->state = CANOPEN_SDO_STATE_READY;
				ret = ERR_ABORTED;
			}
			// data should now be copied and transfer is finished
		}
	}
	else {
		const canopen_object_st *obj;
		// read from our local obj dict
		if ((obj = _canopen_find_object(&msg, CANOPEN_RO)) &&
				data != NULL &&
				obj->data_ptr != NULL) {
			if (uv_canopen_is_array(obj)) {
				if (sindex == 0) {
					// copy array size
					memcpy(data, &obj->array_max_size, sizeof(obj->array_max_size));
				}
				if (obj->array_max_size > sindex - 1) {
					memcpy(data,
							obj->data_ptr + (sindex - 1) * CANOPEN_SIZEOF(obj->type),
							CANOPEN_SIZEOF(obj->type));
				}
			}
			else if (uv_canopen_is_string(obj)) {
				memcpy(data, obj->data_ptr,
						MIN(data_len, obj->string_len));
			}
			else {
				memcpy(data, obj->data_ptr,
						MIN(data_len, CANOPEN_SIZEOF(obj->type)));
			}
		}
		else {
			ret = ERR_CANOPEN_MAPPED_OBJECT_NOT_FOUND;
		}
	}

	return ret;
}


#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER

uv_errors_e _uv_canopen_sdo_client_block_write(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data) {
	uv_errors_e ret = ERR_NONE;
	uv_can_msg_st msg;
	msg.type = CAN_STD;
	msg.id = CANOPEN_SDO_REQUEST_ID + node_id;
	msg.data_length = 8;
	memset(&msg.data_32bit[1], 0, 4);
	SET_MINDEX(&msg, mindex);
	SET_SINDEX(&msg, sindex);
	this->server_node_id = node_id;
	this->mindex = mindex;
	this->sindex = sindex;
	this->data_ptr = data;
	this->data_count = data_len;
	this->data_index = 0;
	this->seq = 0;

	while (this->state != CANOPEN_SDO_STATE_READY) {
		uv_rtos_task_yield();
	}
	uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);

	this->state = CANOPEN_SDO_STATE_BLOCK_DOWNLOAD;
	SET_CMD_BYTE(&msg, INITIATE_BLOCK_DOWNLOAD | (1 << 2) | (1 << 1));
	msg.data_32bit[1] = this->data_count;
	uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);

	// wait for transfer to finish
	while ((this->state != CANOPEN_SDO_STATE_READY) &&
			(this->state != CANOPEN_SDO_STATE_TRANSFER_ABORTED)) {
		uv_rtos_task_yield();
	}

	if (this->state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
		this->state = CANOPEN_SDO_STATE_READY;
		ret = ERR_ABORTED;
	}

	return ret;
}


uv_errors_e _uv_canopen_sdo_client_block_read(uint8_t node_id,
		uint16_t mindex, uint8_t sindex, uint32_t data_len, void *data) {
	uv_errors_e ret = ERR_NONE;
	uv_can_msg_st msg;
	msg.type = CAN_STD;
	msg.data_length = 8;
	msg.id = CANOPEN_SDO_REQUEST_ID + node_id;
	memset(&msg.data_32bit[1], 0, 4);
	SET_MINDEX(&msg, mindex);
	SET_SINDEX(&msg, sindex);
	this->server_node_id = node_id;
	this->mindex = mindex;
	this->sindex = sindex;
	this->data_ptr = data;
	this->data_count = data_len;
	this->data_index = 0;
	this->new_data = false;
	this->seq = 0;

	if (this->state != CANOPEN_SDO_STATE_READY) {
		ret = ERR_HW_BUSY;
	}
	else {
		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);

		this->state = CANOPEN_SDO_STATE_BLOCK_UPLOAD;
		SET_CMD_BYTE(&msg, INITIATE_BLOCK_UPLOAD | (1 << 2));
		msg.data_8bit[4] = BLKSIZE();
		uv_can_send(CONFIG_CANOPEN_CHANNEL, &msg);

		// wait for transfer to finish
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

	return ret;
}

#endif


uv_sdo_error_codes_e _uv_canopen_sdo_get_error_code(void) {
	return this->last_err_code;
}


void uv_canopen_sdo_client_set_wait_callback(void (*callb)(uint16_t, uint8_t)) {
	this->wait_callb = callb;
}



#endif
