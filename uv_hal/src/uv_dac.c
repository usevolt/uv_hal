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



#include "uv_dac.h"

#if CONFIG_TARGET_LPC1549
#include "chip.h"
#include "dac_15xx.h"
#endif

#if CONFIG_DAC

#if CONFIG_TARGET_LPC1549

void _uv_dac_init(void) {
	Chip_DAC_Init(LPC_DAC);
}


void uv_dac_set(uint32_t value) {
	if (value > DAC_MAX_VALUE) {
		value = DAC_MAX_VALUE;
	}
	Chip_DAC_UpdateValue(LPC_DAC, value);
}


#endif

#endif
