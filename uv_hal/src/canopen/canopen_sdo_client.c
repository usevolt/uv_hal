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



/// @brief: Send a SDO Client abort request message to the server and mark the
/// transfer as aborted.
///
/// The abort has to be addressed to the *server's* request COB-ID
/// (CANOPEN_SDO_REQUEST_ID + server_node_id), exactly like every other frame
/// this client sends (see the segmented paths that build their frames on
/// CANOPEN_SDO_REQUEST_ID + node_id). The generic _uv_canopen_sdo_abort()
/// helper instead builds the id from the *local* node id and the response
/// base (CANOPEN_SDO_RESPONSE_ID), which is correct for the SDO server role
/// but sends a client abort to the wrong COB-ID, so the server never sees it.
/// When that happens a segmented transfer left open on the server is not
/// cancelled and the server self-aborts with CANOPEN_SDO_ERROR_SDO_PROTOCOL_TIMED_OUT
/// (0x05040000). Build and send the abort with the correct addressing here.
static inline void sdo_client_abort(uint16_t main_index,
				uint8_t sub_index, uv_sdo_error_codes_e err_code) {
	uv_can_msg_st msg;
	msg.type = CAN_STD;
	msg.id = CANOPEN_SDO_REQUEST_ID + this->server_node_id;
	msg.data_length = 8;
	SET_CMD_BYTE(&msg, ABORT_DOMAIN_TRANSFER);
	SET_MINDEX(&msg, main_index);
	SET_SINDEX(&msg, sub_index);
	msg.data_32bit[1] = err_code;
	_uv_canopen_sdo_send(&msg);

	this->last_err_code = err_code;
	this->state = CANOPEN_SDO_STATE_TRANSFER_ABORTED;
}


/// @brief: Returns the backoff in milliseconds to wait before re-sending a
/// transfer the server would not serve, *attempt* being the number of retries
/// already made.
///
/// A backoff of a fixed length phase-locks this client to the other one
/// sharing the server: if that client polls the server at a period near ours,
/// every retry lands on one of its transfers again and the whole retry budget
/// is spent on the very same collision. The backoff therefore grows with the
/// attempt count, and carries a jitter taken from the RTOS tick count, which
/// has nothing to do with the other client's period.
static uint32_t sdo_client_retry_delay(uint8_t attempt) {
	uint32_t base = CONFIG_CANOPEN_SDO_CLIENT_RETRY_DELAY_MS;
	uint32_t jitter = (base != 0) ? (uv_rtos_get_tick_count() % (base / 2 + 1)) : 0;
	return ((base * ((uint32_t) attempt + 1)) / 2) + jitter;
}


/// @brief: Remaining retry attempts of a transfer, see sdo_client_take_retry().
typedef struct {
	uint8_t timeout;
	uint8_t busy;
} sdo_client_retries_st;


/// @brief: Returns true if the aborted transfer is worth another attempt, and
/// spends one attempt of the budget it belongs to.
///
/// The budgets are kept apart because the failures behind them cost a
/// different amount of time.
///
/// A server that was busy with another transfer refuses ours right away, and
/// so does one that dropped a transfer of ours it had already accepted
/// (*initiated*): that abort says nothing about the object - the server
/// answered the initiate request for it - so what failed is the transfer
/// itself. Both are the normal outcome of sharing a server's SDO channel with
/// another client on the bus, both clear up on their own, and retrying them
/// costs only the backoff, so they retry from the *busy* budget.
///
/// A transfer that timed out instead got no answer at all, from a device that
/// may not even be there, and every further attempt costs a full timeout. Those
/// retry from the smaller *timeout* budget.
///
/// Any other abort is the server refusing this very object, which is permanent:
/// a retry would only reproduce it.
static inline bool sdo_client_take_retry(sdo_client_retries_st *retries,
				uint32_t err_code, bool initiated) {
	bool ret;
	if (err_code == CANOPEN_SDO_ERROR_SDO_PROTOCOL_TIMED_OUT) {
		ret = (retries->timeout != 0);
		if (ret) {
			retries->timeout--;
		}
	}
	else if (initiated ||
			(err_code == CANOPEN_SDO_ERROR_CMD_SPECIFIER_NOT_FOUND)) {
		ret = (retries->busy != 0);
		if (ret) {
			retries->busy--;
		}
	}
	else {
		ret = false;
	}
	return ret;
}


/// @brief: Returns true if a server answer of type *sdo_type* carries the
/// object multiplexer (main index + sub index) in the data bytes 1...3.
///
/// Segment answers carry payload in those bytes instead, so there is nothing
/// in them to match against the transfer in progress. Block transfer answers
/// are left out on purpose: their command specifiers overlap with each other
/// and are told apart by the block state machine only.
static inline bool sdo_msg_has_mux(sdo_request_type_e sdo_type) {
	return ((sdo_type == ABORT_DOMAIN_TRANSFER) ||
			(sdo_type == INITIATE_DOMAIN_DOWNLOAD_REPLY) ||
			(sdo_type == INITIATE_DOMAIN_UPLOAD));
}


/// @brief: Returns true if *msg* is an answer to the transfer this client
/// currently has in flight.
///
/// The SDO channel of a server is shared by every client on the bus: answers
/// to another client's transfer of the same server arrive here with the exact
/// same COB-ID as the answers to ours. Following one of them would make this
/// client read the wrong object (or, for an abort, give up on a transfer the
/// server is still serving), so the object the answer names has to match the
/// one we asked for.
static inline bool sdo_msg_is_for_transfer(const uv_can_message_st *msg,
				sdo_request_type_e sdo_type) {
	bool ret = (GET_NODEID(msg) == this->server_node_id);
	if (ret && sdo_msg_has_mux(sdo_type)) {
		ret = ((GET_MINDEX(msg) == this->mindex) &&
				(GET_SINDEX(msg) == this->sindex));
	}
	else {
		// segment answers name no object, they are matched with *initiated*
	}
	return ret;
}


void _uv_canopen_sdo_client_init(void) {
	this->state = CANOPEN_SDO_STATE_READY;
	this->initiated = false;
	// Created here and never again: _uv_canopen_sdo_client_reset() must not
	// touch the mutex, as re-creating it would leak the old semaphore and
	// could hand the lock to a second task while a transfer still owns it.
	// This runs from _uv_canopen_init() before any task has been started, so
	// no transfer can race the creation.
	uv_mutex_init(&this->mutex);
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
			sdo_msg_is_for_transfer(msg, sdo_type)) {
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
			// the server answered our own initiate request: from here on the
			// segment answers arriving on the shared channel are ours
			this->initiated = true;
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
			_uv_canopen_sdo_send(&reply_msg);
			this->toggle = !this->toggle;
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		}
		// segmented downloads
		else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD) &&
				(sdo_type == DOWNLOAD_DOMAIN_SEGMENT_REPLY) &&
				this->initiated) {
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
					_uv_canopen_sdo_send(&reply_msg);
					this->toggle = !this->toggle;
					uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
				}
			}
		}
		// start of segmented upload
		else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_UPLOAD) &&
				(sdo_type == INITIATE_DOMAIN_UPLOAD)) {
			// the server answered our own initiate request: from here on the
			// segment answers arriving on the shared channel are ours
			this->initiated = true;

			if (GET_CMD_BYTE(msg) & (1 << 1)) {
				// client returned as expedited transfer, segmented transfer is finished
				this->obj_size = 4 - ((GET_CMD_BYTE(msg) & (0b11 << 2)) >> 2);
				memcpy(this->data_ptr, &msg->data_32bit[1], this->obj_size);
				this->state = CANOPEN_SDO_STATE_READY;
			}
			else {
				// segmented transfer
				if (GET_CMD_BYTE(msg) & (1 << 0)) {
					// data size indicated
					this->obj_size = msg->data_32bit[1];
					if (msg->data_32bit[1] < this->data_count) {
						this->data_count = msg->data_32bit[1];
					}
				}
				//segmented transfer, send upload domain segment message
				SET_CMD_BYTE(&reply_msg, UPLOAD_DOMAIN_SEGMENT | (this->toggle << 4));
				_uv_canopen_sdo_send(&reply_msg);
				this->toggle = !this->toggle;
			}
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		}
		// segmented upload
		else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_UPLOAD) &&
				(sdo_type == UPLOAD_DOMAIN_SEGMENT_REPLY) &&
				this->initiated) {
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
					_uv_canopen_sdo_send(&reply_msg);
				}
				else {
					// ask for more data
					SET_CMD_BYTE(&reply_msg, UPLOAD_DOMAIN_SEGMENT | (this->toggle << 4));
					memset(&reply_msg.data_8bit[1], 0, 7);
					memcpy(&reply_msg.data_8bit[1], &msg->data_8bit[1],
							uv_mini(msg->data_length, 7));
					_uv_canopen_sdo_send(&reply_msg);
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
						_uv_canopen_sdo_send(&reply_msg);

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
						// end the transfer. "n" is the number of bytes of the
						// last segment that do NOT contain data, so it is zero
						// when the transfer ends exactly on a segment boundary.
						// Computing it as 7 - (count % 7) gave 7 for a transfer
						// whose size is a multiple of 7, telling the server to
						// throw away a full segment of real data - which cost
						// the last bytes of every such firmware image.
						uint8_t n = (this->data_count % 7) ?
								(7 - (this->data_count % 7)) : 0;
						SET_CMD_BYTE(&reply_msg, END_BLOCK_DOWNLOAD | (n << 2));
						uint16_t crc = uv_memory_calc_crc(this->data_ptr, this->data_count);
						reply_msg.data_8bit[1] = crc;
						reply_msg.data_8bit[2] = crc / 256;
						_uv_canopen_sdo_send(&reply_msg);
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
							_uv_canopen_sdo_send(&reply_msg);

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
					_uv_canopen_sdo_send(&reply_msg);
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
					_uv_canopen_sdo_send(&reply_msg);
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
						_uv_canopen_sdo_send(&reply_msg);
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

	// Only one transfer at a time can use the single SDO client, so wait here
	// for a transfer started by another task to finish instead of failing the
	// caller. See the note on _uv_canopen_sdo_client_st for why this wait is
	// bounded. Taken before the rx filter is configured, which is shared state
	// as well.
	uv_mutex_lock(&this->mutex);

	// configure to receive target device's SDO response messages
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID + node_id, CAN_ID_MASK_DEFAULT, CAN_STD);

	if (this->state != CANOPEN_SDO_STATE_READY) {
		// Unreachable as long as every transfer holds the mutex: each of them
		// leaves the state machine READY before releasing it. Kept as a guard
		// against a state machine left mid-transfer by something else.
		ret = ERR_HW_BUSY;
	}
	else {
		this->server_node_id = node_id;
		this->mindex = mindex;
		this->sindex = sindex;

		// Retry loop: re-send the request if the transfer aborted for a
		// reason another attempt can get past, see sdo_client_take_retry().
		// A server-side refusal of this object is surfaced immediately.
		sdo_client_retries_st retries = {
				.timeout = CONFIG_CANOPEN_SDO_CLIENT_RETRY_COUNT,
				.busy = CONFIG_CANOPEN_SDO_CLIENT_BUSY_RETRY_COUNT
		};
		uint8_t attempt = 0;
		while (true) {
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
			// no answer to this attempt's initiate request seen yet
			this->initiated = false;

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
				_uv_canopen_sdo_send(&msg);
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
				_uv_canopen_sdo_send(&msg);
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
				uv_rtos_task_delay(1);
			}

			if (this->state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
				this->state = CANOPEN_SDO_STATE_READY;
				ret = ERR_ABORTED;
				if (!sdo_client_take_retry(&retries, this->last_err_code,
						this->initiated)) {
					break;
				}
				// back off so a busy server can finish the other transfer
				// before we re-send.
				uv_rtos_task_delay(sdo_client_retry_delay(attempt));
				attempt++;
			}
			else {
				ret = ERR_NONE;
				break;
			}
		}
	}

	uv_mutex_unlock(&this->mutex);

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

	// Wait for any transfer another task started, see the write path. Taken
	// before the request state below is written: those writes are the live
	// state of whatever transfer is currently in flight, so doing them without
	// the lock corrupted that transfer even when this call went on to bail out
	// with ERR_HW_BUSY.
	uv_mutex_lock(&this->mutex);

	this->server_node_id = node_id;
	this->mindex = mindex;
	this->sindex = sindex;
	this->data_ptr = data;
	this->data_index = 0;
	this->data_count = data_len;
	this->obj_size = 0;
	this->toggle = 0;

	// configure to receive target device's SDO response messages
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID + node_id, CAN_ID_MASK_DEFAULT, CAN_STD);

	if (this->state != CANOPEN_SDO_STATE_READY) {
		// See the write path: unreachable while every transfer holds the mutex.
		ret = ERR_HW_BUSY;
	}
	else {
		// Retry loop: re-send the upload request if the transfer aborts for
		// a reason another attempt can get past. See the write path.
		sdo_client_retries_st retries = {
				.timeout = CONFIG_CANOPEN_SDO_CLIENT_RETRY_COUNT,
				.busy = CONFIG_CANOPEN_SDO_CLIENT_BUSY_RETRY_COUNT
		};
		uint8_t attempt = 0;
		while (true) {
			// Reset the per-attempt state machine fields so each retry
			// starts from a clean slate (data_index was reset before the
			// loop but a partial segmented upload may have advanced it).
			this->data_index = 0;
			this->data_count = data_len;
			this->obj_size = 0;
			this->toggle = 0;
			// no answer to this attempt's initiate request seen yet
			this->initiated = false;
			uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);

			// just in case put us to segmented upload state.
			// expedited answers from server are handled as well
			this->state = CANOPEN_SDO_STATE_SEGMENTED_UPLOAD;
			SET_CMD_BYTE(&msg, INITIATE_DOMAIN_UPLOAD);
			_uv_canopen_sdo_send(&msg);

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
				if (!sdo_client_take_retry(&retries, this->last_err_code,
						this->initiated)) {
					break;
				}
				// back off so a busy server can finish the other transfer
				// before we re-send.
				uv_rtos_task_delay(sdo_client_retry_delay(attempt));
				attempt++;
			}
			else {
				ret = ERR_NONE;
				break;
			}
		}
		// data should now be copied and transfer is finished
	}

	uv_mutex_unlock(&this->mutex);

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

	// Wait for any transfer another task started, see the write path. This
	// replaces the "while (state != READY) yield()" spin that used to sit below:
	// that spin was the racy version of exactly this wait, as two tasks could
	// both observe READY and leave it together. Taken before the request state
	// is written for the same reason as in the read path.
	uv_mutex_lock(&this->mutex);

	this->server_node_id = node_id;
	this->mindex = mindex;
	this->sindex = sindex;
	this->data_ptr = data;
	this->data_count = data_len;
	this->data_index = 0;
	this->seq = 0;
	this->initiated = false;

	// configure to receive target device's SDO response messages
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID + node_id, CAN_ID_MASK_DEFAULT, CAN_STD);

	uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);

	this->state = CANOPEN_SDO_STATE_BLOCK_DOWNLOAD;
	SET_CMD_BYTE(&msg, INITIATE_BLOCK_DOWNLOAD | (1 << 2) | (1 << 1));
	msg.data_32bit[1] = this->data_count;
	_uv_canopen_sdo_send(&msg);

	// wait for transfer to finish
	while ((this->state != CANOPEN_SDO_STATE_READY) &&
			(this->state != CANOPEN_SDO_STATE_TRANSFER_ABORTED)) {
		uv_rtos_task_yield();
	}

	if (this->state == CANOPEN_SDO_STATE_TRANSFER_ABORTED) {
		this->state = CANOPEN_SDO_STATE_READY;
		ret = ERR_ABORTED;
	}

	uv_mutex_unlock(&this->mutex);

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

	// Wait for any transfer another task started, see the write path. Taken
	// before the request state is written for the same reason as in the read
	// path.
	uv_mutex_lock(&this->mutex);

	this->server_node_id = node_id;
	this->mindex = mindex;
	this->sindex = sindex;
	this->data_ptr = data;
	this->data_count = data_len;
	this->data_index = 0;
	this->new_data = false;
	this->seq = 0;
	this->initiated = false;

	// configure to receive target device's SDO response messages
	uv_can_config_rx_message(CONFIG_CANOPEN_CHANNEL,
			CANOPEN_SDO_RESPONSE_ID + node_id, CAN_ID_MASK_DEFAULT, CAN_STD);

	if (this->state != CANOPEN_SDO_STATE_READY) {
		// See the write path: unreachable while every transfer holds the mutex.
		ret = ERR_HW_BUSY;
	}
	else {
		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);

		this->state = CANOPEN_SDO_STATE_BLOCK_UPLOAD;
		SET_CMD_BYTE(&msg, INITIATE_BLOCK_UPLOAD | (1 << 2));
		msg.data_8bit[4] = BLKSIZE();
		_uv_canopen_sdo_send(&msg);

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

	uv_mutex_unlock(&this->mutex);

	return ret;
}

#endif


uv_sdo_error_codes_e _uv_canopen_sdo_get_error_code(void) {
	return this->last_err_code;
}


uint32_t _uv_canopen_sdo_client_get_obj_size(void) {
	return this->obj_size;
}


void uv_canopen_sdo_client_set_wait_callback(void (*callb)(uint16_t, uint8_t)) {
	this->wait_callb = callb;
}



#endif
