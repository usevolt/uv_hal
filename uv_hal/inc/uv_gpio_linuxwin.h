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

#ifndef UV_HAL_INC_UV_GPIO_LINUXWIN_H_
#define UV_HAL_INC_UV_GPIO_LINUXWIN_H_

#include <uv_hal_config.h>
#include <uv_utilities.h>


#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN

typedef uint32_t uv_gpios_e;


#define 	P0_0  		0
#define 	P0_1  		0
#define 	P0_2  		0
#define 	P0_3 		0
#define 	P0_4  		0
#define 	P0_5  		0
#define 	P0_6  		0
#define 	P0_7  		0
#define 	P0_8  		0
#define 	P0_9  		0
#define 	P0_10		0
#define 	P0_11		0
#define 	P0_12		0
#define 	P0_13		0
#define 	P0_14       0
#define 	P0_15       0
#define 	P0_16       0
#define 	P0_17       0
#define 	P0_18       0
#define 	P0_19       0
#define 	P0_20       0
#define 	P0_21       0
#define 	P0_22       0
#define 	P0_23       0
#define 	P0_24       0
#define 	P0_25       0
#define 	P0_26       0
#define 	P0_27       0
#define 	P0_28       0
#define 	P0_29       0
#define 	P0_30       0
#define 	P0_31       0
#define 	P1_0        0
#define 	P1_1        0
#define 	P1_2        0
#define 	P1_3        0
#define 	P1_4        0
#define 	P1_5        0
#define 	P1_6        0
#define 	P1_7        0
#define 	P1_8        0
#define 	P1_9        0
#define 	P1_10       0
#define 	P1_11       0


#define __TABLE1(value) (value)



#define GPIO_1_port 			0
#define GPIO_1_config(value)	0
#define GPIO_1_pin				0
#define GPIO_2_port 			0
#define GPIO_2_config(value)	0
#define GPIO_2_pin				0
#define GPIO_3_port 			0
#define GPIO_3_config(value)	0
#define GPIO_3_pin				0
#define GPIO_4_port 			0
#define GPIO_4_config(value)	0
#define GPIO_4_pin				0
#define GPIO_5_port 			0
#define GPIO_5_config(value)	0
#define GPIO_5_pin				0
#define GPIO_6_port 			0
#define GPIO_6_config(value)	0
#define GPIO_6_pin				0
#define GPIO_7_port 			0
#define GPIO_7_config(value)	0
#define GPIO_7_pin				0
#define GPIO_8_port 			0
#define GPIO_8_config(value)	0
#define GPIO_8_pin				0
#define GPIO_9_port 			0
#define GPIO_9_config(value)	0
#define GPIO_9_pin				0
#define GPIO_10_port 			0
#define GPIO_10_config(value)	0
#define GPIO_10_pin				0
#define GPIO_11_port 			0
#define GPIO_11_config(value)	0
#define GPIO_11_pin				0
#define GPIO_12_port 			0
#define GPIO_12_config(value)	0
#define GPIO_12_pin				0
#define GPIO_13_port 			0
#define GPIO_13_config(value)	0
#define GPIO_13_pin				0
#define GPIO_14_port 			0
#define GPIO_14_config(value)	0
#define GPIO_14_pin				0
#define GPIO_15_port 			0
#define GPIO_15_config(value)	0
#define GPIO_15_pin				0
#define GPIO_16_port 			0
#define GPIO_16_config(value)	0
#define GPIO_16_pin				0
#define GPIO_17_port 			0
#define GPIO_17_config(value)	0
#define GPIO_17_pin				0
#define GPIO_18_port 			0
#define GPIO_18_config(value)	0
#define GPIO_18_pin				0
#define GPIO_18_port 			0
#define GPIO_19_port 			0
#define GPIO_19_config(value)	0
#define GPIO_19_pin				0
#define GPIO_20_port 			0
#define GPIO_20_config(value)	0
#define GPIO_20_pin				0
#define GPIO_21_port 			0
#define GPIO_21_config(value)	0
#define GPIO_21_pin				0
#define GPIO_22_port 			0
#define GPIO_22_config(value)	0
#define GPIO_22_pin				0
#define GPIO322_port 			0
#define GPIO322_config(value)	0
#define GPIO322_pin				0
#define GPIO423_port 			0
#define GPIO423_config(value)	0
#define GPIO423_pin				0
#define GPIO_25_port 			0
#define GPIO_25_config(value)	0
#define GPIO_25_pin				0
#define GPIO_26_port 			0
#define GPIO_26_config(value)	0
#define GPIO_26_pin				0
#define GPIO_27_port 			0
#define GPIO_27_config(value)	0
#define GPIO_27_pin				0
#define GPIO_28_port 			0
#define GPIO_28_config(value)	0
#define GPIO_28_pin				0
#define GPIO_29_port 			0
#define GPIO_29_config(value)	0
#define GPIO_29_pin				0
#define GPIO_30_port 			0
#define GPIO_30_config(value)	0
#define GPIO_30_pin				0
#define GPIO_31_port 			0
#define GPIO_31_config(value)	0
#define GPIO_31_pin				0
#define GPIO_32_port 			0
#define GPIO_32_config(value)	0
#define GPIO_32_pin				0

#define GPIO_33_port 			0
#define GPIO_33_config(value)	0
#define GPIO_33_pin				0
#define GPIO_34_port 			0
#define GPIO_34_config(value)	0
#define GPIO_34_pin				0
#define GPIO_35_port 			0
#define GPIO_35_config(value)	0
#define GPIO_35_pin				0
#define GPIO_36_port 			0
#define GPIO_36_config(value)	0
#define GPIO_36_pin				0
#define GPIO_37_port 			0
#define GPIO_37_config(value)	0
#define GPIO_37_pin				0
#define GPIO_38_port 			0
#define GPIO_38_config(value)	0
#define GPIO_38_pin				0
#define GPIO_39_port 			0
#define GPIO_39_config(value)	0
#define GPIO_39_pin				0
#define GPIO_40_port 			0
#define GPIO_40_config(value)	0
#define GPIO_40_pin				0
#define GPIO_41_port 			0
#define GPIO_41_config(value)	0
#define GPIO_41_pin				0
#define GPIO_42_port 			0
#define GPIO_42_config(value)	0
#define GPIO_42_pin				0
#define GPIO_43_port 			0
#define GPIO_43_config(value)	0
#define GPIO_43_pin				0
#define GPIO_44_port 			0
#define GPIO_44_config(value)	0
#define GPIO_44_pin				0


#endif

#endif /* UV_HAL_INC_UV_GPIO_LPC1549_H_ */
