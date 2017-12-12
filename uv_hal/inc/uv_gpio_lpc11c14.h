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

#ifndef UV_HAL_INC_UV_GPIO_LPC11C14_H_
#define UV_HAL_INC_UV_GPIO_LPC11C14_H_



#include <uv_hal_config.h>
#include <uv_utilities.h>


#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"


#define P0_0		1
#define P0_1		2
#define P0_2		3
#define P0_3		4
#define P0_4		5
#define P0_5		6
#define P0_6		7
#define P0_7		8
#define P0_8		9
#define P0_9		10
#define P0_10		11
#define P0_11		12
#define P1_0		13
#define P1_1		14
#define P1_2		15
#define P1_3		16
#define P1_4		17
#define P1_5		18
#define P1_6		19
#define P1_7		20
#define P1_8		21
#define P1_9		22
#define P1_10		23
#define P1_11		24
#define P2_0		25
#define P2_1		26
#define P2_2		27
#define P2_3		28
#define P2_4		29
#define P2_5		30
#define P2_6		31
#define P2_7		32
#define P2_8		33
#define P2_9		34
#define P2_10		35
#define P2_11		36
#define P3_0		37
#define P3_1		38
#define P3_2		39
#define P3_3		40
#define P3_4		41
#define P3_5		42



typedef uint32_t uv_gpios_e;


#define port(port_num)			CAT(LPC_GPIO, port_num)

#define __TABLE1(value) (value)
#define __TABLE2(value) ((value | 1))
#define __TABLE3(value) (value | (1 + (1 << 7)))
#define __TABLE4(value) (value | (1 << 7))



#define GPIO_1_port 			0
#define GPIO_1_config(value)	LPC_IOCON->RESET_PIO0_0 = __TABLE2(value)
#define GPIO_1_pin				0
#define GPIO_2_port 			0
#define GPIO_2_config(value)	LPC_IOCON->PIO0_1 = __TABLE1(value)
#define GPIO_2_pin				1
#define GPIO_3_port 			0
#define GPIO_3_config(value)	LPC_IOCON->PIO0_2 = __TABLE1(value)
#define GPIO_3_pin				2
#define GPIO_4_port 			0
#define GPIO_4_config(value)	LPC_IOCON->PIO0_3 = __TABLE1(value)
#define GPIO_4_pin				3
#define GPIO_5_port 			0
#define GPIO_5_config(value)	LPC_IOCON->PIO0_4 = __TABLE1(value)
#define GPIO_5_pin				4
#define GPIO_6_port 			0
#define GPIO_6_config(value)	LPC_IOCON->PIO0_5 = __TABLE1(value)
#define GPIO_6_pin				5
#define GPIO_7_port 			0
#define GPIO_7_config(value)	LPC_IOCON->PIO0_6 = __TABLE1(value)
#define GPIO_7_pin				6
#define GPIO_8_port 			0
#define GPIO_8_config(value)	LPC_IOCON->PIO0_7 = __TABLE1(value)
#define GPIO_8_pin				7
#define GPIO_9_port 			0
#define GPIO_9_config(value)	LPC_IOCON->PIO0_8 = __TABLE1(value)
#define GPIO_9_pin				8
#define GPIO_10_port 			0
#define GPIO_10_config(value)	LPC_IOCON->PIO0_9 = __TABLE1(value)
#define GPIO_10_pin				9
#define GPIO_11_port 			0
#define GPIO_11_config(value)	LPC_IOCON->SWCLK_PIO0_10 = __TABLE2(value)
#define GPIO_11_pin				10
#define GPIO_12_port 			0
#define GPIO_12_config(value)	LPC_IOCON->R_PIO0_11 = __TABLE3(value)
#define GPIO_12_pin				11


#define GPIO_13_port 			1
#define GPIO_13_config(value)	LPC_IOCON->R_PIO1_0 = __TABLE3(value)
#define GPIO_13_pin				0
#define GPIO_14_port 			1
#define GPIO_14_config(value)	LPC_IOCON->R_PIO1_1 = __TABLE3(value)
#define GPIO_14_pin				1
#define GPIO_15_port 			1
#define GPIO_15_config(value)	LPC_IOCON->R_PIO1_2 = __TABLE3(value)
#define GPIO_15_pin				2
#define GPIO_16_port 			1
#define GPIO_16_config(value)	LPC_IOCON->SWDIO_PIO1_3 = __TABLE3(value)
#define GPIO_16_pin				3
#define GPIO_17_port 			1
#define GPIO_17_config(value)	LPC_IOCON->PIO1_4 = __TABLE4(value)
#define GPIO_17_pin				4
#define GPIO_18_port 			1
#define GPIO_18_config(value)	LPC_IOCON->PIO1_5 = __TABLE1(value)
#define GPIO_18_pin				5
#define GPIO_19_port 			1
#define GPIO_19_config(value)	LPC_IOCON->PIO1_6 = __TABLE1(value)
#define GPIO_19_pin				6
#define GPIO_20_port 			1
#define GPIO_20_config(value)	LPC_IOCON->PIO1_7 = __TABLE1(value)
#define GPIO_20_pin				7
#define GPIO_21_port 			1
#define GPIO_21_config(value)	LPC_IOCON->PIO1_8 = __TABLE1(value)
#define GPIO_21_pin				8
#define GPIO_22_port 			1
#define GPIO_22_config(value)	LPC_IOCON->PIO1_9 = __TABLE1(value)
#define GPIO_22_pin				9
#define GPIO_23_port 			1
#define GPIO_23_config(value)	LPC_IOCON->PIO1_10 = __TABLE1(value)
#define GPIO_23_pin				10
#define GPIO_24_port 			1
#define GPIO_24_config(value)	LPC_IOCON->PIO1_11 = __TABLE4(value)
#define GPIO_24_pin				11


#define GPIO_25_port 			2
#define GPIO_25_config(value)	LPC_IOCON->PIO2_0 = __TABLE1(value)
#define GPIO_25_pin				0
#define GPIO_26_port 			2
#define GPIO_26_config(value)	LPC_IOCON->PIO2_1 = __TABLE1(value)
#define GPIO_26_pin				1
#define GPIO_27_port 			2
#define GPIO_27_config(value)	LPC_IOCON->PIO2_2 = __TABLE1(value)
#define GPIO_27_pin				2
#define GPIO_28_port 			2
#define GPIO_28_config(value)	LPC_IOCON->PIO2_3 = __TABLE1(value)
#define GPIO_28_pin				3
#define GPIO_29_port 			2
#define GPIO_29_config(value)	LPC_IOCON->PIO2_4 = __TABLE1(value)
#define GPIO_29_pin				4
#define GPIO_30_port 			2
#define GPIO_30_config(value)	LPC_IOCON->PIO2_5 = __TABLE1(value)
#define GPIO_30_pin				5
#define GPIO_31_port 			2
#define GPIO_31_config(value)	LPC_IOCON->PIO2_6 = __TABLE1(value)
#define GPIO_31_pin				6
#define GPIO_32_port 			2
#define GPIO_32_config(value)	LPC_IOCON->PIO2_7 = __TABLE1(value)
#define GPIO_32_pin				7
#define GPIO_33_port 			2
#define GPIO_33_config(value)	LPC_IOCON->PIO2_8 = __TABLE1(value)
#define GPIO_33_pin				8
#define GPIO_34_port 			2
#define GPIO_34_config(value)	LPC_IOCON->PIO2_9 = __TABLE1(value)
#define GPIO_34_pin				9
#define GPIO_35_port 			2
#define GPIO_35_config(value)	LPC_IOCON->PIO2_10 = __TABLE1(value)
#define GPIO_35_pin				10
#define GPIO_36_port 			2
#define GPIO_36_config(value)	LPC_IOCON->PIO2_11 = __TABLE1(value)
#define GPIO_36_pin				11



#define GPIO_37_port 			3
#define GPIO_37_config(value)	LPC_IOCON->PIO3_0 = __TABLE1(value)
#define GPIO_37_pin				0
#define GPIO_38_port 			3
#define GPIO_38_config(value)	LPC_IOCON->PIO3_1 = __TABLE1(value)
#define GPIO_38_pin				1
#define GPIO_39_port 			3
#define GPIO_39_config(value)	LPC_IOCON->PIO3_2 = __TABLE1(value)
#define GPIO_39_pin				2
#define GPIO_40_port 			3
#define GPIO_40_config(value)	LPC_IOCON->PIO3_3 = __TABLE1(value)
#define GPIO_40_pin				3
#define GPIO_41_port 			3
#define GPIO_41_config(value)	LPC_IOCON->PIO3_4 = __TABLE1(value)
#define GPIO_41_pin				4
#define GPIO_42_port 			3
#define GPIO_42_config(value)	LPC_IOCON->PIO3_5 = __TABLE1(value)
#define GPIO_42_pin				5




#endif

#endif /* UV_HAL_INC_UV_GPIO_LPC11C14_H_ */
