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


#ifndef UV_HAL_INC_UV_DAC_H_
#define UV_HAL_INC_UV_DAC_H_

#include <uv_hal_config.h>
#include "uv_utilities.h"

/// @file: Digital to analog converter module

#if CONFIG_DAC

#if !defined(CONFIG_DAC_VREFP_MV)
#define CONFIG_DAC_VREFP_MV		3300
#endif
#if !defined(CONFIG_DAC_VREFN_MV)
#define CONFIG_DAC_VREFN_MV		0
#elif (CONFIG_DAC_VREFN_MV != 0)
#error "CONFIG_DAC_VREFN_MV doesnt yet support other values than 0"
#endif



#define DAC_MAX_VALUE		0xFFF


/// @brief: Returns the DAC value from millivolts. Millivolts should be between
/// CONFIG_DAC_VREFN_MV ... CONFIG_DAC_CREFP_MV
#define DAC_FROM_MV(mv)		((((int32_t) (mv) - CONFIG_DAC_VREFN_MV) * (DAC_MAX_VALUE + 1)) / \
		(CONFIG_DAC_VREFP_MV - CONFIG_DAC_VREFN_MV))


/// @brief: Sets the output value to DAC module.
void uv_dac_set(uint32_t value);



#endif

/// @brief: Initializes the dac. For HAL library inner usage only
void _uv_dac_init(void);


#endif /* UV_HAL_INC_UV_DAC_H_ */
