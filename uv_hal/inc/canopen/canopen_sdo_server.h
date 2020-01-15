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

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_SDO_SERVER_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_SDO_SERVER_H_

#include <uv_hal_config.h>
#include "canopen/canopen_common.h"
#include "uv_can.h"
#include "canopen/canopen_sdo.h"

#if CONFIG_CANOPEN


typedef struct {
	canopen_sdo_state_e state;
	uint16_t mindex;
	uint8_t sindex;
	// an optional callback to be called when something has been written to an object
	// pointed by **mindex** and **sindex**.
	void (*callb)(uint16_t mindex, uint8_t sindex);
#if (CONFIG_CANOPEN_SDO_SEGMENTED || CONFIG_CANOPEN_SDO_BLOCK_TRANSFER)
	// contains the index of next data to be transmitted
	uint16_t data_index;
	const canopen_object_st *obj;
	union {
		uint8_t toggle;
		/// @brief: Last correctly received sequence number for block transfer.
		/// Starts from -1.
		int8_t seq;
	};
	uv_delay_st delay;
#if CONFIG_CANOPEN_SDO_BLOCK_TRANSFER
	uint8_t data_buffer[7];
	bool new_data;
	uint8_t client_blksize;
	bool crc_enabled;
#endif
#endif
} _uv_canopen_sdo_server_st;


void _uv_canopen_sdo_server_init(void);

void _uv_canopen_sdo_server_reset(void);

void _uv_canopen_sdo_server_step(uint16_t step_ms);

void _uv_canopen_sdo_server_rx(const uv_can_message_st *msg, sdo_request_type_e sdo_type);

void _uv_canopen_sdo_server_add_callb(void (*callb)(uint16_t mindex, uint8_t sindex));



#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_SDO_SERVER_H_ */
