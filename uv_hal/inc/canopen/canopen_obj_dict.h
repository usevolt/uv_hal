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

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_OBJ_DICT_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_OBJ_DICT_H_


#include <uv_hal_config.h>
#include "canopen/canopen_common.h"

#if CONFIG_CANOPEN


#if !defined(CONFIG_CANOPEN_OBJ_DICT_IN_RISING_ORDER)
#warning "CONFIG_CANOPEN_OBJ_DICT_IN_RISING_ORDER should be defined as 1 or 0 depending if\
 object dictionary main indexes are all in rising order. This makes indexing obj dict faster."
#endif



/// @brief: Returns the CANopen object dictionary object pointed by **main_index**.
/// Subindexes are not supported, since they are assumed to be array indexes. However,
/// subindex can be given to this function, in which case the function checks
/// if the object was an array type and that the object had as many indexes as
/// requested.
///
/// @return: Null if the object couldn't be found.
const canopen_object_st *_uv_canopen_obj_dict_get(uint16_t main_index, uint8_t subindex);




#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_OBJ_DICT_H_ */
