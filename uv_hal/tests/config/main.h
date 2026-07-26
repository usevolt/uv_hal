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

#ifndef UV_HAL_TESTS_MAIN_H_
#define UV_HAL_TESTS_MAIN_H_

/// @file: Stand-in for the application's main.h.
///
/// Several uv_hal sources - the CANopen stack in particular - `#include
/// CONFIG_MAIN_H` to reach the application's global device struct, and the
/// object dictionary places the standard CANopen objects (node id, baudrate,
/// heartbeat times, PDO communication and mapping parameters) inside the
/// application's non-volatile region.
///
/// This supplies the minimum shape those sources need: a device struct with a
/// non-volatile section. The instance itself lives in stubs/canopen_stubs.c.
///
/// If a module under test ever starts requiring real *application* fields from
/// here, that is a sign it is reaching into application state and is worth a
/// second look rather than a bigger stub.


#include <uv_utilities.h>
#include <uv_memory.h>


/// @brief: Placeholder for the application's device struct.
///
/// The tag name matters: uv_utilities.h declares the global as
/// `extern CONFIG_APP_ST;` without including this header, so CONFIG_APP_ST
/// refers to `struct _dev_st` as an incomplete type. That is the same
/// arrangement the real applications use.
typedef struct _dev_st {
	/// @brief: Start of the non-volatile region. uv_hal's own persistent state -
	/// including the CANopen node id, baudrate and PDO parameters - lives here.
	uv_data_start_t data_start;

	/// @brief: End of the non-volatile region.
	uv_data_end_t data_end;
} dev_st;


extern dev_st dev;


#endif /* UV_HAL_TESTS_MAIN_H_ */
