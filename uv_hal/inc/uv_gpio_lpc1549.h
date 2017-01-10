/*
 * uv_gpio_lpc1549.h
 *
 *  Created on: Jan 8, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_GPIO_LPC1549_H_
#define UV_HAL_INC_UV_GPIO_LPC1549_H_

#include <uv_hal_config.h>
#include <uv_utilities.h>


#if CONFIG_TARGET_LPC1549

typedef uint32_t uv_gpios_e;


#define 	P0_0  		0
#define 	P0_1  		1
#define 	P0_2  		2
#define 	P0_3 		3
#define 	P0_4  		4
#define 	P0_5  		5
#define 	P0_6  		6
#define 	P0_7  		7
#define 	P0_8  		8
#define 	P0_9  		9
#define 	P0_10		10
#define 	P0_11		11
#define 	P0_12		12
#define 	P0_13		13
#define 	P0_14       14
#define 	P0_15       15
#define 	P0_16       16
#define 	P0_17       17
#define 	P0_18       18
#define 	P0_19       19
#define 	P0_20       20
#define 	P0_21       21
//#define 	P0_22       22	For I2C
//#define 	P0_23       23	For I2C
#define 	P0_24       24
#define 	P0_25       25
#define 	P0_26       26
#define 	P0_27       27
#define 	P0_28       28
#define 	P0_29       29
#define 	P0_30       30
#define 	P0_31       31
#define 	P1_0        32
#define 	P1_1        33
#define 	P1_2        34
#define 	P1_3        35
#define 	P1_4        36
#define 	P1_5        37
#define 	P1_6        38
#define 	P1_7        39
#define 	P1_8        40
#define 	P1_9        41
#define 	P1_10       42
#define 	P1_11       43


#define __TABLE1(value) (value | (1 << 7))



#define GPIO_0_port 			0
#define GPIO_0_config(value)	LPC_IOCON->PIO[0][0] = __TABLE1(value)
#define GPIO_0_pin				0
#define GPIO_1_port 			0
#define GPIO_1_config(value)	LPC_IOCON->PIO[0][1] = __TABLE1(value)
#define GPIO_1_pin				1
#define GPIO_2_port 			0
#define GPIO_2_config(value)	LPC_IOCON->PIO[0][2] = __TABLE1(value)
#define GPIO_2_pin				2
#define GPIO_3_port 			0
#define GPIO_3_config(value)	LPC_IOCON->PIO[0][3] = __TABLE1(value)
#define GPIO_3_pin				3
#define GPIO_4_port 			0
#define GPIO_4_config(value)	LPC_IOCON->PIO[0][4] = __TABLE1(value)
#define GPIO_4_pin				4
#define GPIO_5_port 			0
#define GPIO_5_config(value)	LPC_IOCON->PIO[0][5] = __TABLE1(value)
#define GPIO_5_pin				5
#define GPIO_6_port 			0
#define GPIO_6_config(value)	LPC_IOCON->PIO[0][6] = __TABLE1(value)
#define GPIO_6_pin				6
#define GPIO_7_port 			0
#define GPIO_7_config(value)	LPC_IOCON->PIO[0][7] = __TABLE1(value)
#define GPIO_7_pin				7
#define GPIO_8_port 			0
#define GPIO_8_config(value)	LPC_IOCON->PIO[0][8] = __TABLE1(value)
#define GPIO_8_pin				8
#define GPIO_9_port 			0
#define GPIO_9_config(value)	LPC_IOCON->PIO[0][9] = __TABLE1(value)
#define GPIO_9_pin				9
#define GPIO_10_port 			0
#define GPIO_10_config(value)	LPC_IOCON->PIO[0][10] = __TABLE1(value)
#define GPIO_10_pin				10
#define GPIO_11_port 			0
#define GPIO_11_config(value)	LPC_IOCON->PIO[0][11] = __TABLE1(value)
#define GPIO_11_pin				11
#define GPIO_12_port 			0
#define GPIO_12_config(value)	LPC_IOCON->PIO[0][12] = __TABLE1(value)
#define GPIO_12_pin				12
#define GPIO_13_port 			0
#define GPIO_13_config(value)	LPC_IOCON->PIO[0][13] = __TABLE1(value)
#define GPIO_13_pin				13
#define GPIO_14_port 			0
#define GPIO_14_config(value)	LPC_IOCON->PIO[0][14] = __TABLE1(value)
#define GPIO_14_pin				14
#define GPIO_15_port 			0
#define GPIO_15_config(value)	LPC_IOCON->PIO[0][15] = __TABLE1(value)
#define GPIO_15_pin				15
#define GPIO_16_port 			0
#define GPIO_16_config(value)	LPC_IOCON->PIO[0][16] = __TABLE1(value)
#define GPIO_16_pin				16
#define GPIO_17_port 			0
#define GPIO_17_config(value)	LPC_IOCON->PIO[0][17] = __TABLE1(value)
#define GPIO_17_pin				17
#define GPIO_18_port 			0
#define GPIO_18_config(value)	LPC_IOCON->PIO[0][18] = __TABLE1(value)
#define GPIO_18_pin				18
#define GPIO_19_port 			0
#define GPIO_19_config(value)	LPC_IOCON->PIO[0][19] = __TABLE1(value)
#define GPIO_19_pin				19
#define GPIO_20_port 			0
#define GPIO_20_config(value)	LPC_IOCON->PIO[0][20] = __TABLE1(value)
#define GPIO_20_pin				20
#define GPIO_21_port 			0
#define GPIO_21_config(value)	LPC_IOCON->PIO[0][21] = __TABLE1(value)
#define GPIO_21_pin				21
//#define GPIO_22_port 			0
//#define GPIO_22_config(value)	LPC_IOCON->PIO[0][22] = __TABLE1(value)
//#define GPIO_22_pin				22
//#define GPIO_23_port 			0
//#define GPIO_23_config(value)	LPC_IOCON->PIO[0][23] = __TABLE1(value)
//#define GPIO_23_pin				23
#define GPIO_24_port 			0
#define GPIO_24_config(value)	LPC_IOCON->PIO[0][24] = __TABLE1(value)
#define GPIO_24_pin				24
#define GPIO_25_port 			0
#define GPIO_25_config(value)	LPC_IOCON->PIO[0][25] = __TABLE1(value)
#define GPIO_25_pin				25
#define GPIO_26_port 			0
#define GPIO_26_config(value)	LPC_IOCON->PIO[0][26] = __TABLE1(value)
#define GPIO_26_pin				26
#define GPIO_27_port 			0
#define GPIO_27_config(value)	LPC_IOCON->PIO[0][27] = __TABLE1(value)
#define GPIO_27_pin				27
#define GPIO_28_port 			0
#define GPIO_28_config(value)	LPC_IOCON->PIO[0][28] = __TABLE1(value)
#define GPIO_28_pin				28
#define GPIO_29_port 			0
#define GPIO_29_config(value)	LPC_IOCON->PIO[0][29] = __TABLE1(value)
#define GPIO_29_pin				29
#define GPIO_30_port 			0
#define GPIO_30_config(value)	LPC_IOCON->PIO[0][30] = __TABLE1(value)
#define GPIO_30_pin				30
#define GPIO_31_port 			0
#define GPIO_31_config(value)	LPC_IOCON->PIO[0][31] = __TABLE1(value)
#define GPIO_31_pin				31

#define GPIO_32_port 			1
#define GPIO_32_config(value)	LPC_IOCON->PIO[1][0] = __TABLE1(value)
#define GPIO_32_pin				0
#define GPIO_33_port 			1
#define GPIO_33_config(value)	LPC_IOCON->PIO[1][1] = __TABLE1(value)
#define GPIO_33_pin				1
#define GPIO_34_port 			1
#define GPIO_34_config(value)	LPC_IOCON->PIO[1][2] = __TABLE1(value)
#define GPIO_34_pin				2
#define GPIO_35_port 			1
#define GPIO_35_config(value)	LPC_IOCON->PIO[1][3] = __TABLE1(value)
#define GPIO_35_pin				3
#define GPIO_36_port 			1
#define GPIO_36_config(value)	LPC_IOCON->PIO[1][4] = __TABLE1(value)
#define GPIO_36_pin				4
#define GPIO_37_port 			1
#define GPIO_37_config(value)	LPC_IOCON->PIO[1][5] = __TABLE1(value)
#define GPIO_37_pin				5
#define GPIO_38_port 			1
#define GPIO_38_config(value)	LPC_IOCON->PIO[1][6] = __TABLE1(value)
#define GPIO_38_pin				6
#define GPIO_39_port 			1
#define GPIO_39_config(value)	LPC_IOCON->PIO[1][7] = __TABLE1(value)
#define GPIO_39_pin				7
#define GPIO_40_port 			1
#define GPIO_40_config(value)	LPC_IOCON->PIO[1][8] = __TABLE1(value)
#define GPIO_40_pin				8
#define GPIO_41_port 			1
#define GPIO_41_config(value)	LPC_IOCON->PIO[1][9] = __TABLE1(value)
#define GPIO_41_pin				9
#define GPIO_42_port 			1
#define GPIO_42_config(value)	LPC_IOCON->PIO[1][10] = __TABLE1(value)
#define GPIO_42_pin				10
#define GPIO_43_port 			1
#define GPIO_43_config(value)	LPC_IOCON->PIO[1][11] = __TABLE1(value)
#define GPIO_43_pin				11


#endif

#endif /* UV_HAL_INC_UV_GPIO_LPC1549_H_ */
