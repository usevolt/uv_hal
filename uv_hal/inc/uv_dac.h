/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
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


/// @brief: Initializes the dac. For HAL library inner usage only
void _uv_dac_init(void);

#endif

#endif /* UV_HAL_INC_UV_DAC_H_ */
