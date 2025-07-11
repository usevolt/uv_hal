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


#include "canopen/canopen_sdo_server.h"
#include "uv_canopen.h"
#include CONFIG_MAIN_H
#include <string.h>
#if CONFIG_UV_BOOTLOADER
#include "uv_reset.h"
#endif

#if CONFIG_CANOPEN


#define this (&_canopen.sdo.server)
#define this_nonvol	(&CONFIG_NON_VOLATILE_START.canopen_data)
#define NODEID			_canopen.current_node_id

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



void _uv_canopen_sdo_server_add_write_callb(void (*write_callb)(uint16_t mindex, uint8_t sindex)) {
	this->write_callb = write_callb;
}

void _uv_canopen_sdo_server_add_read_callb(void (*read_callb)(uint16_t mindex, uint8_t sindex)) {
	this->read_callb = read_callb;
}

/// @brief: Send a SDO Server abort response message
static void sdo_server_abort(uint16_t main_index,
				uint8_t sub_index, uv_sdo_error_codes_e err_code) {
	_uv_canopen_sdo_abort(CANOPEN_SDO_RESPONSE_ID, main_index, sub_index, err_code);
	this->state = CANOPEN_SDO_STATE_READY;
}



void _uv_canopen_sdo_server_init(void) {
	this->state = CANOPEN_SDO_STATE_READY;
	this->write_callb = NULL;
}

void _uv_canopen_sdo_server_reset(void) {
}

void _uv_canopen_sdo_server_step(uint16_t step_ms) {
#if (CONFIG_CANOPEN_SDO_SEGMENTED || CONFIG_CANOPEN_SDO_BLOCK_TRANSFER)
	if (this->state >= CANOPEN_SDO_STATE_SEGMENTED_UPLOAD) {
		if (uv_delay(&this->delay, step_ms)) {
			sdo_server_abort(this->mindex, this->sindex,
					CANOPEN_SDO_ERROR_SDO_PROTOCOL_TIMED_OUT);
			this->state = CANOPEN_SDO_STATE_READY;
		}
	}
#endif
}

void _uv_canopen_sdo_server_rx(const uv_can_message_st *msg, sdo_request_type_e sdo_type) {
	const canopen_object_st *obj;
	uv_can_msg_st reply_msg;
	reply_msg.type = CAN_STD;
	reply_msg.id = CANOPEN_SDO_RESPONSE_ID + NODEID;
	reply_msg.data_length = 8;
	memset(reply_msg.data_8bit, 0, 8);
	SET_MINDEX(&reply_msg, GET_MINDEX(msg));
	SET_SINDEX(&reply_msg, GET_SINDEX(msg));

#if CONFIG_UV_BOOTLOADER
	// Bootloader special check: If object 0x1F50, u.e. program data is written,
	// set the message reception flag and reset the device in order to enter
	// uv bootloader. Bootloader is responsible for answering the message.
	if (GET_MINDEX(msg) == CONFIG_CANOPEN_PROGRAM_DATA_INDEX &&
			GET_SINDEX(msg) == 1) {
		// Note: SDO type is not checked here, because the application might not
		// support Block transfers but uv bootloader does.

		// copy received message to bootloader shared mem address
		memcpy(UV_BOOTLOADER_DATA_ADDR, msg, sizeof(uv_can_msg_st));
		// reset the device
		uv_bootloader_start();
	}
#endif
#if (CONFIG_CANOPEN_UPDATE_PDO_MAPPINGS_ON_NODEID_WRITE == 1)
	uint8_t last_nodeid = uv_canopen_get_our_nodeid();
#endif

	// receiving an abort message returns the node to default state
	if (sdo_type == ABORT_DOMAIN_TRANSFER) {
		this->state = CANOPEN_SDO_STATE_READY;
	}
	else if (this->state == CANOPEN_SDO_STATE_READY) {
		// try to find the requested object. If object was not found,
		// abort message is sent automatically
#if (CONFIG_CANOPEN_SDO_SEGMENTED || CONFIG_CANOPEN_SDO_BLOCK_TRANSFER)
		// initialize data variables
		this->data_index = 0;
		this->mindex = GET_MINDEX(msg);
		this->sindex = GET_SINDEX(msg);
		this->toggle = 0;
		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
#endif

		// initiate download (write request)
		if (sdo_type == INITIATE_DOMAIN_DOWNLOAD) {
			if ((obj = _canopen_find_object(msg, CANOPEN_WO))) {
				// segmented transfer
				if (!(GET_CMD_BYTE(msg) & (1 << 1))) {
#if CONFIG_CANOPEN_SDO_SEGMENTED
					// segmented transfer can be started only on string or array type objects
					if (uv_canopen_is_string(obj) ||
							uv_canopen_is_array(obj)) {
						this->obj = obj;
						this->state = CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD;
						this->data_index = uv_canopen_is_string(obj) ?
								GET_SINDEX(msg) :
								((GET_SINDEX(msg) - 1) * CANOPEN_SIZEOF(obj->type));
						SET_CMD_BYTE(&reply_msg,
								INITIATE_DOMAIN_DOWNLOAD_REPLY);
						// data bytes indicate data index which starts from 0
						memset(&reply_msg.data_8bit[4], 0, 4);
						uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
						// send all replies also locally in case it was us that
						// initiated the SDO transfer on ourselves
						uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
								CAN_SEND_FLAGS_LOCAL |
								CAN_SEND_FLAGS_NO_TX_CALLB);
					}
					else {
						sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
								CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
					}
#endif
				}
				// expedited transfer
				else {
					// expedited transfer can be started to all kinds of objects,
					// also on strings, but only if the string len is less than 4 bytes
					SET_CMD_BYTE(&reply_msg,
							INITIATE_DOMAIN_DOWNLOAD_REPLY);
					if (_canopen_write_data(obj, msg, GET_SINDEX(msg))) {
						memcpy(&reply_msg.data_32bit[1], &msg->data_32bit[1], 4);
						uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
						// send all replies also locally in case it was us that
						// initiated the SDO transfer on ourselves
						uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
								CAN_SEND_FLAGS_LOCAL |
								CAN_SEND_FLAGS_NO_TX_CALLB);

						// break PDO linkage if new COB-ID is written
						if ((this->mindex & ~0x700) == (CANOPEN_TXPDO1_ID & ~0x700) &&
								this->sindex == 1) {
							uint8_t pdo = ((this->mindex & 0x700) >> 8);
							canopen_pdo_com_parameter_st *p =
									uv_canopen_txpdo_get_com(pdo);
							if (p) {
								p->reserved |=
										CANOPEN_PDO_RESERVED_FLAGS_BREAKNODEIDLINKAGE;
							}
						}
						else if ((this->mindex & ~0x700) ==
								(CANOPEN_RXPDO1_ID & ~0x700) &&
								this->sindex == 1) {
							uint8_t pdo = ((this->mindex & 0x700) >> 8);
							canopen_pdo_com_parameter_st *p =
									uv_canopen_rxpdo_get_com(pdo);
							if (p) {
								p->reserved |=
										CANOPEN_PDO_RESERVED_FLAGS_BREAKNODEIDLINKAGE;
							}
						}
						// update PDO's if nodeid was written
						else if (this->mindex == CONFIG_CANOPEN_NODEID_INDEX) {
							canopen_pdo_cobid_update();
						}
						else {

						}

						if (this->write_callb) {
							this->write_callb(this->mindex, this->sindex);
						}
					}
					else {
						// tried to write to array index 0, abort transfer
						sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
								CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
					}
				}
			}
		}

		// initiate upload (read request)
		else if (sdo_type == INITIATE_DOMAIN_UPLOAD) {
			if ((obj = _canopen_find_object(msg, CANOPEN_RO))) {
				// segmented transfer for strings or arrays
				if (uv_canopen_is_string(obj) ||
						(uv_canopen_is_array(obj) &&
								GET_SINDEX(msg) != 0)) {
#if CONFIG_CANOPEN_SDO_SEGMENTED
					this->state = CANOPEN_SDO_STATE_SEGMENTED_UPLOAD;
					this->data_index = uv_canopen_is_string(obj) ?
							GET_SINDEX(msg) :
							((GET_SINDEX(msg) - 1) * CANOPEN_SIZEOF(obj->type));
					this->obj = obj;
					// initiate segmented domain upload with data size indicated
					SET_CMD_BYTE(&reply_msg,
							INITIATE_DOMAIN_UPLOAD | (1 << 0));
					// data bytes contain the total byte count
					reply_msg.data_32bit[1] = uv_canopen_is_string(obj) ?
							(obj->string_len - this->data_index) :
							(obj->array_max_size * CANOPEN_SIZEOF(obj->type) - this->data_index);
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					// send all replies also locally in case it was us that
					// initiated the SDO transfer on ourselves
					uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
							CAN_SEND_FLAGS_LOCAL |
							CAN_SEND_FLAGS_NO_TX_CALLB);
#else
					sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
							CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
#endif
				}

				// expedited transfer
				else {
					// set command byte: expedited transfer, size indicated,
					// object type len set correctly
					SET_CMD_BYTE(&reply_msg,
							INITIATE_DOMAIN_UPLOAD | (1 << 1) | (1 << 0) |
							((4 - CANOPEN_TYPE_LEN(obj->type)) << 2));
					_canopen_copy_data(&reply_msg, obj, GET_SINDEX(msg));
					if (this->read_callb) {
						this->read_callb(GET_MINDEX(msg), GET_SINDEX(msg));
					}
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					// send all replies also locally in case it was us that
					// initiated the SDO transfer on ourselves
					uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
							CAN_SEND_FLAGS_LOCAL |
							CAN_SEND_FLAGS_NO_TX_CALLB);
				}
			}
		}
#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER
		// Initiate block download (write)
		else if (sdo_type == INITIATE_BLOCK_DOWNLOAD) {
			if ((obj = _canopen_find_object(msg, CANOPEN_WO))) {
				if (obj->type == CANOPEN_STRING) {
					this->state = CANOPEN_SDO_STATE_BLOCK_DOWNLOAD;
					this->obj = obj;
					this->seq = 0;
					this->new_data = false;
					this->crc_enabled = (GET_CMD_BYTE(msg) & (1 << 2));
					SET_CMD_BYTE(&reply_msg,
							INITIATE_BLOCK_DOWNLOAD_REPLY | (1 << 2));
					reply_msg.data_8bit[4] = BLKSIZE();
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					// send all replies also locally in case it was us that
					// initiated the SDO transfer on ourselves
					uv_can_send_local(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				}
				else {
					sdo_server_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
				}
			}
		}
		// Initiate block upload (read)
		else if (sdo_type == INITIATE_BLOCK_UPLOAD) {
			if ((obj = _canopen_find_object(msg, CANOPEN_RO))) {
				if (obj->type == CANOPEN_STRING) {
					this->state = CANOPEN_SDO_STATE_BLOCK_UPLOAD_WFR;
					this->obj = obj;
					this->seq = 0;
					this->crc_enabled = (GET_CMD_BYTE(msg) & (1 << 2));
					this->client_blksize = msg->data_8bit[4];
					SET_CMD_BYTE(&reply_msg,
							INITIATE_BLOCK_UPLOAD_REPLY | (1 << 2) | (1 << 1));
					reply_msg.data_32bit[1] = obj->string_len;
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					// send all replies also locally in case it was us that
					// initiated the SDO transfer on ourselves
					uv_can_send_local(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				}
				else {
					sdo_server_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
				}
			}
		}
#endif
		else {
			sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
					CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
		}
	}

#if CONFIG_CANOPEN_SDO_SEGMENTED
	// segmented upload
	else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_UPLOAD) &&
			(sdo_type == UPLOAD_DOMAIN_SEGMENT)) {

		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		SET_MINDEX(&reply_msg, this->mindex);
		SET_SINDEX(&reply_msg, this->sindex);
		// check toggle bit
		if (((GET_CMD_BYTE(msg) & (1 << 4)) >> 4) == this->toggle) {
			uint16_t len = uv_canopen_is_string(this->obj) ?
					this->obj->string_len :
					(this->obj->array_max_size * CANOPEN_SIZEOF(this->obj->type));
			uint8_t data_count = uv_mini(len - this->data_index, 7);
			// transmission continues
			if (len - this->data_index > 7) {
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
			if (this->obj->data_ptr != NULL) {
				memcpy(&reply_msg.data_8bit[1],
						((uint8_t*) this->obj->data_ptr) + this->data_index, data_count);
			}
			this->data_index += data_count;
			uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
			// send all replies also locally in case it was us that
			// initiated the SDO transfer on ourselves
			uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
					CAN_SEND_FLAGS_LOCAL |
					CAN_SEND_FLAGS_NO_TX_CALLB);
			this->toggle = !this->toggle;
			if (this->read_callb) {
				this->read_callb(this->mindex, this->sindex);
			}
		}
		else {
			sdo_server_abort(this->mindex, this->sindex,
					CANOPEN_SDO_ERROR_SDO_TOGGLE_BIT_NOT_ALTERED);
		}
	}

	// segmented download
	else if ((this->state == CANOPEN_SDO_STATE_SEGMENTED_DOWNLOAD) &&
			(sdo_type == DOWNLOAD_DOMAIN_SEGMENT)) {

		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
		SET_MINDEX(&reply_msg, this->mindex);
		SET_SINDEX(&reply_msg, this->sindex);
		// check toggle bit
		if (((GET_CMD_BYTE(msg) & (1 << 4)) >> 4) == this->toggle) {
			// last segment
			if (GET_CMD_BYTE(msg) & (1 << 0)) {
				this->state = CANOPEN_SDO_STATE_READY;
			}
			uint8_t data_count = 7 - ((GET_CMD_BYTE(msg) & 0b1110) >> 1);
			uint16_t len = (uv_canopen_is_string(this->obj)) ?
					this->obj->string_len :
					this->obj->array_max_size * CANOPEN_SIZEOF(this->obj->type);
			if ((this->data_index + data_count) <= len) {
				// copy data to destination. Bits 1-4 in command byte indicate
				// how much data is copied
				if (this->obj->data_ptr) {
					memcpy(((uint8_t*)this->obj->data_ptr) + this->data_index,
							&msg->data_8bit[1], data_count);
				}
				this->data_index += data_count;

				SET_CMD_BYTE(&reply_msg, DOWNLOAD_DOMAIN_SEGMENT_REPLY | (this->toggle << 4));
				memset(&reply_msg.data_8bit[1], 0, 7);
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				// send all replies also locally in case it was us that
				// initiated the SDO transfer on ourselves
				uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
						CAN_SEND_FLAGS_LOCAL |
						CAN_SEND_FLAGS_NO_TX_CALLB);
				this->toggle = !this->toggle;
			}
			else {
				sdo_server_abort(this->mindex, this->sindex,
						CANOPEN_SDO_ERROR_OUT_OF_MEMORY);
			}
			if ((this->state == CANOPEN_SDO_STATE_READY) && this->write_callb) {
				this->write_callb(this->mindex, this->sindex);
			}
		}
		else {
			sdo_server_abort(this->mindex, this->sindex,
					CANOPEN_SDO_ERROR_SDO_TOGGLE_BIT_NOT_ALTERED);
		}
	}
#endif

#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER
	// segmented block download
	else if (this->state == CANOPEN_SDO_STATE_BLOCK_DOWNLOAD) {

		// either 1st sequence message or end of block download can be received
		if ((this->seq == 0) && (sdo_type == END_BLOCK_DOWNLOAD)) {
			// end of block download
			uint8_t numbytes = 7 - ((GET_CMD_BYTE(msg) & (0b111 << 2)) >> 2);
			if (this->obj->string_len >= numbytes) {
				// copy last data from buffer to destination
				memcpy(this->obj->data_ptr + this->data_index, this->data_buffer, numbytes);
				this->data_index += numbytes;
			}
			else {
				sdo_server_abort(this->mindex, this->sindex,
						CANOPEN_SDO_ERROR_OUT_OF_MEMORY);
			}
			if (this->crc_enabled) {
				// calculate data crc
				uint16_t crc = uv_memory_calc_crc(this->obj->data_ptr, this->data_index);
				uint16_t client_crc = (msg->data_8bit[1]) + (msg->data_8bit[2] * 256);
				if (crc != client_crc) {
					sdo_server_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_CRC_ERROR);
				}
				else {
					// block download finished
					SET_CMD_BYTE(&reply_msg, END_BLOCK_DOWNLOAD_REPLY);
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					// send all replies also locally in case it was us that
					// initiated the SDO transfer on ourselves
					uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
							CAN_SEND_FLAGS_LOCAL |
							CAN_SEND_FLAGS_NO_TX_CALLB);
					this->state = CANOPEN_SDO_STATE_READY;
					if (this->write_callb) {
						this->write_callb(this->mindex, this->sindex);
					}
				}
			}
		}
		// downloading sequence data
		else if ((GET_CMD_BYTE(msg) & 0x7F) == this->seq + 1) {
			// copy data to destination
			// copy last message data from buffer to destination
			if (this->new_data) {
				if (this->obj->string_len >= this->data_index + 7) {
					memcpy(this->obj->data_ptr + this->data_index, this->data_buffer, 7);
					this->data_index += 7;
					this->new_data = false;
				}
				else {
					// tried to write too long data to object, aborting.
					sdo_server_abort(this->mindex, this->sindex,
							CANOPEN_SDO_ERROR_OUT_OF_MEMORY);
				}
			}
			// copy new data into data buffer
			memcpy(this->data_buffer, &msg->data_8bit[1], msg->data_length - 1);
			this->new_data = true;
			// increase the sequence number of correctly received messages
			this->seq++;

			if ((GET_CMD_BYTE(msg) & (1 << 7))) {
				// last sequence received, reply to the client and possibly wait for another block
				memset(reply_msg.data_8bit, 0, 8);
				SET_CMD_BYTE(&reply_msg, DOWNLOAD_BLOCK_SEGMENT_REPLY);
				reply_msg.data_8bit[1] = this->seq;
				reply_msg.data_8bit[2] = BLKSIZE();
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				// send all replies also locally in case it was us that
				// initiated the SDO transfer on ourselves
				uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
						CAN_SEND_FLAGS_LOCAL |
						CAN_SEND_FLAGS_NO_TX_CALLB);
				this->seq = 0;
			}
		}
		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
	}
	// initiate block upload handshake
	else if ((this->state == CANOPEN_SDO_STATE_BLOCK_UPLOAD_WFR) &&
			(sdo_type == INITIATE_BLOCK_UPLOAD_REPLY2)) {
		if (!this->client_blksize) {
			this->client_blksize = msg->data_8bit[4];
		}
		if (!this->client_blksize) {
			// client didn't provide blksize, cannot continue with the transfer
			sdo_server_abort(this->mindex, this->sindex,
					CANOPEN_SDO_ERROR_INVALID_BLOCK_SIZE);
		}
		else {
			// upload the block
			for (uint8_t i = 0; i < this->client_blksize; i++) {
				uint8_t len = (this->obj->string_len >= (this->data_index + 7)) ?
						7 : (this->obj->string_len - this->data_index);
				if (len != 7) {
					// in this case the uploaded object was shorter than client blksize
					// indicated. Normal operation, we just end the transfer earlier.
					i = this->client_blksize - 1;
				}
				this->seq++;
				SET_CMD_BYTE(&reply_msg, (((i + 1) == this->client_blksize) << 7) | this->seq);
				memcpy(&reply_msg.data_8bit[1], this->obj->data_ptr + this->data_index, len);
				this->data_index += len;
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				// send all replies also locally in case it was us that
				// initiated the SDO transfer on ourselves
				uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
						CAN_SEND_FLAGS_LOCAL |
						CAN_SEND_FLAGS_NO_TX_CALLB);
			}
			this->seq = 0;
			this->state = CANOPEN_SDO_STATE_BLOCK_UPLOAD;
		}
		uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
	}
	else if ((this->state == CANOPEN_SDO_STATE_BLOCK_UPLOAD) &&
			(sdo_type == UPLOAD_BLOCK_SEGMENT_REPLY)) {
		if (msg->data_8bit[1] < this->seq) {
			// message transfer failed for some reason
			sdo_server_abort(this->mindex, this->sindex,
					CANOPEN_SDO_ERROR_INVALID_SEQ_NUMBER);
		}
		else {
			if (this->data_index >= this->obj->string_len) {
				// end block transfer
				SET_CMD_BYTE(&reply_msg, END_BLOCK_UPLOAD | ((7 - (this->obj->string_len % 7)) << 2));
				uint16_t crc = uv_memory_calc_crc(this->obj->data_ptr, this->data_index);
				reply_msg.data_8bit[1] = crc;
				reply_msg.data_8bit[2] = crc / 256;
				uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
				// send all replies also locally in case it was us that
				// initiated the SDO transfer on ourselves
				uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
						CAN_SEND_FLAGS_LOCAL |
						CAN_SEND_FLAGS_NO_TX_CALLB);
				this->state = CANOPEN_SDO_STATE_BLOCK_END_UPLOAD;
				uv_delay_init(&this->delay, CONFIG_CANOPEN_SDO_TIMEOUT_MS);
			}
			else {
				// upload more data
				for (uint8_t i = 0; i < this->client_blksize; i++) {
					uint8_t len = (this->obj->string_len >= (this->data_index + 7)) ?
							7 : (this->obj->string_len - this->data_index);
					if (len != 7) {
						// in this case the uploaded object was shorter than client blksize
						// indicated. Normal operation, we just end the transfer earlier.
						i = this->client_blksize - 1;
					}
					this->seq++;
					memset(reply_msg.data_8bit, 0, 8);
					SET_CMD_BYTE(&reply_msg, (((i + 1) == this->client_blksize) << 7) | this->seq);
					memcpy(&reply_msg.data_8bit[1], this->obj->data_ptr + this->data_index, len);
					this->data_index += len;
					uv_can_send(CONFIG_CANOPEN_CHANNEL, &reply_msg);
					// send all replies also locally in case it was us that
					// initiated the SDO transfer on ourselves
					uv_can_send_flags(CONFIG_CANOPEN_CHANNEL, &reply_msg,
							CAN_SEND_FLAGS_LOCAL |
							CAN_SEND_FLAGS_NO_TX_CALLB);
				}
				// get ready for next transfer
				this->seq = 0;
			}
		}
	}
	else if ((this->state == CANOPEN_SDO_STATE_BLOCK_END_UPLOAD) &&
			(sdo_type == END_BLOCK_UPLOAD_REPLY)) {
		// block transfer finished
		this->state = CANOPEN_SDO_STATE_READY;
	}
#endif
	else {
		printf("unknown sdo 0x%x, 0x%x\n", sdo_type, this->state);
		sdo_server_abort(GET_MINDEX(msg), GET_SINDEX(msg),
				CANOPEN_SDO_ERROR_OBJECT_ACCESS_FAILED_DUE_TO_HARDWARE);
	}

#if (CONFIG_CANOPEN_UPDATE_PDO_MAPPINGS_ON_NODEID_WRITE == 1)
	if (GET_MINDEX(msg) == CONFIG_CANOPEN_NODEID_INDEX) {
		uint8_t new_nodeid = CONFIG_NON_VOLATILE_START.id;


		// copy new nodeid to all PDO's that had our nodeid in them
		for (uint8_t i = 0; i < CONFIG_CANOPEN_TXPDO_COUNT; i++) {
			canopen_pdo_com_parameter_st *txcom = uv_canopen_txpdo_get_com(i);
			if (!(txcom->cob_id & CANOPEN_PDO_EXT) &&
					!(txcom->cob_id & CANOPEN_PDO_DISABLED)) {
				if ((txcom->cob_id & 0x7F) == last_nodeid) {
					txcom->cob_id &= ~(0x7F);
					txcom->cob_id |= new_nodeid;
				}
			}
		}
		for (uint8_t i = 0; i < CONFIG_CANOPEN_RXPDO_COUNT; i++) {
			canopen_pdo_com_parameter_st *rxcom = uv_canopen_rxpdo_get_com(i);
			if (!(rxcom->cob_id & CANOPEN_PDO_EXT) &&
					!(rxcom->cob_id & CANOPEN_PDO_DISABLED)) {
				if ((rxcom->cob_id & 0x7F) == last_nodeid) {
					rxcom->cob_id &= ~(0x7F);
					rxcom->cob_id |= new_nodeid;
				}
			}
		}
	}
#endif

}


#endif

