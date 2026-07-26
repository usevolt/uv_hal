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

#ifndef UV_HAL_TESTS_CANOPEN_TEST_ENV_H_
#define UV_HAL_TESTS_CANOPEN_TEST_ENV_H_

#include <stdint.h>
#include <stdbool.h>

#include "uv_can.h"
#include "canopen/canopen_common.h"

/// @file: A fake CAN bus for the CANopen tests.
///
/// The CANopen stack has exactly one way in and one way out: frames arrive
/// through _uv_canopen_sdo_rx() and leave through uv_can_send(). That makes it
/// straightforward to test against the protocol rather than against the
/// implementation - a test hands the stack the bytes a real master would put on
/// the wire and checks the bytes that come back.
///
/// Nothing here talks to a CAN peripheral. uv_can_send_flags() simply appends
/// the frame to a list the test can inspect.


/// @brief: The node id the fake device answers to
#define CANOPEN_TEST_NODEID			0x0A

/// @brief: How many sent frames the capture buffer holds
#define CANOPEN_TEST_TX_MAX			64


/// @brief: Clears the captured frames, the object dictionary contents and the
/// CANopen state, and puts the node back into the operational state with
/// CANOPEN_TEST_NODEID.
///
/// Call this at the start of every CANopen test. The stack keeps its state in
/// globals, so a half finished transfer would otherwise leak into the next test.
void canopen_test_env_reset(void);


/// @brief: Returns the number of frames the stack has sent since the last reset
uint32_t canopen_test_tx_count(void);


/// @brief: Returns the *index*th frame the stack has sent, or NULL if there is
/// no such frame. Index 0 is the oldest.
const uv_can_message_st *canopen_test_tx_at(uint32_t index);


/// @brief: Returns the frame the stack sent most recently, or NULL if it has
/// sent nothing since the last reset.
const uv_can_message_st *canopen_test_tx_last(void);


/// @brief: Discards the captured frames without touching anything else.
/// Useful for ignoring the frames of a setup phase.
void canopen_test_tx_clear(void);


/* ---------------------------------------------------------------------------
 * The test object dictionary
 *
 * One entry per case the SDO server has to handle: each integer width, a
 * read-only and a write-only object, a string long enough to force a segmented
 * transfer, and an array.
 * ------------------------------------------------------------------------ */

#define TEST_OBJ_U8			0x2000
#define TEST_OBJ_U16		0x2001
#define TEST_OBJ_U32		0x2002
#define TEST_OBJ_RO			0x2003
#define TEST_OBJ_WO			0x2004
#define TEST_OBJ_STRING		0x2005
#define TEST_OBJ_ARRAY32	0x2006
#define TEST_OBJ_ARRAY8		0x2007
/// @brief: An index that is deliberately absent from the dictionary
#define TEST_OBJ_MISSING	0x2FFF

#define TEST_STRING_LEN		32
#define TEST_ARRAY_LEN		4


/// @brief: Storage backing the test object dictionary. Tests read and write
/// these directly to set up a scenario or to check what an SDO write landed on.
typedef struct {
	uint8_t u8;
	uint16_t u16;
	uint32_t u32;
	uint32_t ro;
	uint32_t wo;
	char string[TEST_STRING_LEN];
	uint32_t array32[TEST_ARRAY_LEN];
	uint8_t array8[TEST_ARRAY_LEN];
} canopen_test_data_st;

extern canopen_test_data_st canopen_test_data;


/// @brief: Returns the number of times the SDO server's write callback has been
/// invoked since the last reset, and through the out parameters the main and sub
/// index it last reported. Pass NULL for either index if it is not needed.
uint32_t canopen_test_write_callb_count(uint16_t *mindex, uint8_t *sindex);


/// @brief: As canopen_test_write_callb_count, for the read callback
uint32_t canopen_test_read_callb_count(uint16_t *mindex, uint8_t *sindex);


#endif /* UV_HAL_TESTS_CANOPEN_TEST_ENV_H_ */
