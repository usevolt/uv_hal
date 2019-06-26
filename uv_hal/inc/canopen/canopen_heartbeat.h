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

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_HEARTBEAT_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_HEARTBEAT_H_



#include <uv_hal_config.h>
#include "uv_can.h"
#include "canopen/canopen_common.h"
#include "canopen/canopen_nmt.h"

#if CONFIG_CANOPEN



#if CONFIG_CANOPEN_HEARTBEAT_CONSUMER
/// @brief: Returns true if the heartbeat producer indicated by **node_id**
/// has been expired, i.e. no heartbeat messages has been received from that node
/// in time specified in index [1016].
bool uv_canopen_heartbeat_producer_is_expired(uint8_t node_id);

canopen_node_states_e uv_canopen_heartbeat_producer_get_state(uint8_t nodeid);
#endif

void _uv_canopen_heartbeat_init(void);

void _uv_canopen_heartbeat_reset(void);

void _uv_canopen_heartbeat_step(uint16_t step_ms);

void _uv_canopen_heartbeat_rx(const uv_can_message_st *msg);


#endif /* UV_HAL_INC_CANOPEN_CANOPEN_HEARTBEAT_H_ */
#endif
