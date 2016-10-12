/*
 * uv_gpio_lpc1785.h
 *
 *  Created on: Oct 11, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_GPIO_LPC1785_H_
#define UV_HAL_INC_UV_GPIO_LPC1785_H_


#include <uv_hal_config.h>
#include <uv_utilities.h>


#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"


#define __TABLE81(value) (value & (~(1 << 8)))
#define __TABLE83(value) ((value & (~(1 << 5))) | (0b11 << 7))
#define __TABLE85(value) (0)
#define __TABLE87(value) (value & (~(1 << 9) | (1 << 8)))
#define __TABLE89(value) (value  | (1 << 7))


typedef enum {
	PIO0_0 = 0,
	PIO0_1,
	PIO0_2,
	PIO0_3,
	PIO0_4,
	PIO0_5,
	PIO0_6,
	PIO0_7,
	PIO0_8,
	PIO0_9,
	PIO0_10,
	PIO0_11,
	PIO0_12,
	PIO0_13,
	PIO0_14,
	PIO0_15,
	PIO0_16,
	PIO0_17,
	PIO0_18,
	PIO0_19,
	PIO0_20,
	PIO0_21,
	PIO0_22,
	PIO0_23,
	PIO0_24,
	PIO0_25,
	PIO0_26,
	PIO0_27,
	PIO0_28,
	PIO0_29,
	PIO0_30,
	PIO0_31,
	PIO1_0,
	PIO1_1,
	PIO1_2,
	PIO1_3,
	PIO1_4,
	PIO1_5,
	PIO1_6,
	PIO1_7,
	PIO1_8,
	PIO1_9,
	PIO1_10,
	PIO1_11,
	PIO1_12,
	PIO1_13,
	PIO1_14,
	PIO1_15,
	PIO1_16,
	PIO1_17,
	PIO1_18,
	PIO1_19,
	PIO1_20,
	PIO1_21,
	PIO1_22,
	PIO1_23,
	PIO1_24,
	PIO1_25,
	PIO1_26,
	PIO1_27,
	PIO1_28,
	PIO1_29,
	PIO1_30,
	PIO1_31,
	PIO2_0,
	PIO2_1,
	PIO2_2,
	PIO2_3,
	PIO2_4,
	PIO2_5,
	PIO2_6,
	PIO2_7,
	PIO2_8,
	PIO2_9,
	PIO2_10,
	PIO2_11,
	PIO2_12,
	PIO2_13,
	PIO2_14,
	PIO2_15,
	PIO2_16,
	PIO2_17,
	PIO2_18,
	PIO2_19,
	PIO2_20,
	PIO2_21,
	PIO2_22,
	PIO2_23,
	PIO2_24,
	PIO2_25,
	PIO2_26,
	PIO2_27,
	PIO2_28,
	PIO2_29,
	PIO2_30,
	PIO2_31,
	PIO3_0,
	PIO3_1,
	PIO3_2,
	PIO3_3,
	PIO3_4,
	PIO3_5,
	PIO3_6,
	PIO3_7,
	PIO3_8,
	PIO3_9,
	PIO3_10,
	PIO3_11,
	PIO3_12,
	PIO3_13,
	PIO3_14,
	PIO3_15,
	PIO3_16,
	PIO3_17,
	PIO3_18,
	PIO3_19,
	PIO3_20,
	PIO3_21,
	PIO3_22,
	PIO3_23,
	PIO3_24,
	PIO3_25,
	PIO3_26,
	PIO3_27,
	PIO3_28,
	PIO3_29,
	PIO3_30,
	PIO3_31,
	PIO4_0,
	PIO4_1,
	PIO4_2,
	PIO4_3,
	PIO4_4,
	PIO4_5,
	PIO4_6,
	PIO4_7,
	PIO4_8,
	PIO4_9,
	PIO4_10,
	PIO4_11,
	PIO4_12,
	PIO4_13,
	PIO4_14,
	PIO4_15,
	PIO4_16,
	PIO4_17,
	PIO4_18,
	PIO4_19,
	PIO4_20,
	PIO4_21,
	PIO4_22,
	PIO4_23,
	PIO4_24,
	PIO4_25,
	PIO4_26,
	PIO4_27,
	PIO4_28,
	PIO4_29,
	PIO4_30,
	PIO4_31,
	PIO5_0,
	PIO5_1,
	PIO5_2,
	PIO5_3,
	PIO5_4
} uv_gpios_e;



#define port(port_num)			CAT(LPC_GPIO, port_num)


#define PIO0_0_port 			0
#define PIO0_0_config(value)	LPC_IOCON->P0_0 = __TABLE81(value)
#define PIO0_0_pin				0
#define PIO0_1_port 			0
#define PIO0_1_config(value)	LPC_IOCON->P0_1 = __TABLE81(value)
#define PIO0_1_pin				1
#define PIO0_2_port 			0
#define PIO0_2_config(value)	LPC_IOCON->P0_2 = __TABLE81(value)
#define PIO0_2_pin				2
#define PIO0_3_port 			0
#define PIO0_3_config(value)	LPC_IOCON->P0_3 = __TABLE81(value)
#define PIO0_3_pin				3
#define PIO0_4_port 			0
#define PIO0_4_config(value)	LPC_IOCON->P0_4 = __TABLE81(value)
#define PIO0_4_pin				4
#define PIO0_5_port 			0
#define PIO0_5_config(value)	LPC_IOCON->P0_5 = __TABLE81(value)
#define PIO0_5_pin				5
#define PIO0_6_port 			0
#define PIO0_6_config(value)	LPC_IOCON->P0_6 = __TABLE81(value)
#define PIO0_6_pin				6
#define PIO0_7_port 			0
#define PIO0_7_config(value)	LPC_IOCON->P0_7 = __TABLE89(value)
#define PIO0_7_pin				7
#define PIO0_8_port 			0
#define PIO0_8_config(value)	LPC_IOCON->P0_8 = __TABLE89(value)
#define PIO0_8_pin				8
#define PIO0_9_port 			0
#define PIO0_9_config(value)	LPC_IOCON->P0_9 = __TABLE89(value)
#define PIO0_9_pin				9
#define PIO0_10_port 			0
#define PIO0_10_config(value)	LPC_IOCON->P0_10 = __TABLE81(value)
#define PIO0_10_pin				10
#define PIO0_11_port 			0
#define PIO0_11_config(value)	LPC_IOCON->P0_11 = __TABLE81(value)
#define PIO0_11_pin				11
#define PIO0_12_port 			0
#define PIO0_12_config(value)	LPC_IOCON->P0_12 = __TABLE83(value)
#define PIO0_12_pin				12
#define PIO0_13_port 			0
#define PIO0_13_config(value)	LPC_IOCON->P0_13 = __TABLE83(value)
#define PIO0_13_pin				13
#define PIO0_14_port 			0
#define PIO0_14_config(value)	LPC_IOCON->P0_14 = __TABLE81(value)
#define PIO0_14_pin				14
#define PIO0_15_port 			0
#define PIO0_15_config(value)	LPC_IOCON->P0_15 = __TABLE81(value)
#define PIO0_15_pin				15
#define PIO0_16_port 			0
#define PIO0_16_config(value)	LPC_IOCON->P0_16 = __TABLE81(value)
#define PIO0_16_pin				16
#define PIO0_17_port 			0
#define PIO0_17_config(value)	LPC_IOCON->P0_17 = __TABLE81(value)
#define PIO0_17_pin				17
#define PIO0_18_port 			0
#define PIO0_18_config(value)	LPC_IOCON->P0_18 = __TABLE81(value)
#define PIO0_18_pin				18
#define PIO0_19_port 			0
#define PIO0_19_config(value)	LPC_IOCON->P0_19 = __TABLE81(value)
#define PIO0_19_pin				19
#define PIO0_20_port 			0
#define PIO0_20_config(value)	LPC_IOCON->P0_20 = __TABLE81(value)
#define PIO0_20_pin				20
#define PIO0_21_port 			0
#define PIO0_21_config(value)	LPC_IOCON->P0_21 = __TABLE81(value)
#define PIO0_21_pin				21
#define PIO0_22_port 			0
#define PIO0_22_config(value)	LPC_IOCON->P0_22 = __TABLE81(value)
#define PIO0_22_pin				22
#define PIO0_23_port 			0
#define PIO0_23_config(value)	LPC_IOCON->P0_23 = __TABLE83(value)
#define PIO0_23_pin				23
#define PIO0_24_port 			0
#define PIO0_24_config(value)	LPC_IOCON->P0_24 = __TABLE83(value)
#define PIO0_24_pin				24
#define PIO0_25_port 			0
#define PIO0_25_config(value)	LPC_IOCON->P0_25 = __TABLE83(value)
#define PIO0_25_pin				25
#define PIO0_26_port 			0
#define PIO0_26_config(value)	LPC_IOCON->P0_26 = __TABLE83(value)
#define PIO0_26_pin				26
#define PIO0_27_port 			0
#define PIO0_27_config(value)	LPC_IOCON->P0_27 = __TABLE87(value)
#define PIO0_27_pin				27
#define PIO0_28_port 			0
#define PIO0_28_config(value)	LPC_IOCON->P0_28 = __TABLE87(value)
#define PIO0_28_pin				28
#define PIO0_29_port 			0
#define PIO0_29_config(value)	LPC_IOCON->P0_29 = __TABLE85(value)
#define PIO0_29_pin				29
#define PIO0_30_port 			0
#define PIO0_30_config(value)	LPC_IOCON->P0_30 = __TABLE85(value)
#define PIO0_30_pin				30
#define PIO0_31_port 			0
#define PIO0_31_config(value)	LPC_IOCON->P0_31 = __TABLE85(value)
#define PIO0_31_pin				31





#define PIO1_0_port 			1
#define PIO1_0_config(value)	LPC_IOCON->P1_0 = __TABLE81(value)
#define PIO1_0_pin				0
#define PIO1_1_port 			1
#define PIO1_1_config(value)	LPC_IOCON->P1_1 = __TABLE81(value)
#define PIO1_1_pin				1
#define PIO1_2_port 			1
#define PIO1_2_config(value)	LPC_IOCON->P1_2 = __TABLE81(value)
#define PIO1_2_pin				2
#define PIO1_3_port 			1
#define PIO1_3_config(value)	LPC_IOCON->P1_3 = __TABLE81(value)
#define PIO1_3_pin				3
#define PIO1_4_port 			1
#define PIO1_4_config(value)	LPC_IOCON->P1_4 = __TABLE81(value)
#define PIO1_4_pin				4
#define PIO1_5_port 			1
#define PIO1_5_config(value)	LPC_IOCON->P1_5 = __TABLE89(value)
#define PIO1_5_pin				5
#define PIO1_6_port 			1
#define PIO1_6_config(value)	LPC_IOCON->P1_6 = __TABLE89(value)
#define PIO1_6_pin				6
#define PIO1_7_port 			1
#define PIO1_7_config(value)	LPC_IOCON->P1_7 = __TABLE89(value)
#define PIO1_7_pin				7
#define PIO1_8_port 			1
#define PIO1_8_config(value)	LPC_IOCON->P1_8 = __TABLE81(value)
#define PIO1_8_pin				8
#define PIO1_9_port 			1
#define PIO1_9_config(value)	LPC_IOCON->P1_9 = __TABLE81(value)
#define PIO1_9_pin				9
#define PIO1_10_port 			1
#define PIO1_10_config(value)	LPC_IOCON->P1_10 = __TABLE81(value)
#define PIO1_10_pin				10
#define PIO1_11_port 			1
#define PIO1_11_config(value)	LPC_IOCON->P1_11 = __TABLE81(value)
#define PIO1_11_pin				11
#define PIO1_12_port 			1
#define PIO1_12_config(value)	LPC_IOCON->P1_12 = __TABLE81(value)
#define PIO1_12_pin				12
#define PIO1_13_port 			1
#define PIO1_13_config(value)	LPC_IOCON->P1_13 = __TABLE81(value)
#define PIO1_13_pin				13
#define PIO1_14_port 			1
#define PIO1_14_config(value)	LPC_IOCON->P1_14 = __TABLE89(value)
#define PIO1_14_pin				14
#define PIO1_15_port 			1
#define PIO1_15_config(value)	LPC_IOCON->P1_15 = __TABLE81(value)
#define PIO1_15_pin				15
#define PIO1_16_port 			1
#define PIO1_16_config(value)	LPC_IOCON->P1_16 = __TABLE89(value)
#define PIO1_16_pin				16
#define PIO1_17_port 			1
#define PIO1_17_config(value)	LPC_IOCON->P1_17 = __TABLE89(value)
#define PIO1_17_pin				17
#define PIO1_18_port 			1
#define PIO1_18_config(value)	LPC_IOCON->P1_18 = __TABLE81(value)
#define PIO1_18_pin				18
#define PIO1_19_port 			1
#define PIO1_19_config(value)	LPC_IOCON->P1_19 = __TABLE81(value)
#define PIO1_19_pin				19
#define PIO1_20_port 			1
#define PIO1_20_config(value)	LPC_IOCON->P1_20 = __TABLE81(value)
#define PIO1_20_pin				20
#define PIO1_21_port 			1
#define PIO1_21_config(value)	LPC_IOCON->P1_21 = __TABLE81(value)
#define PIO1_21_pin				21
#define PIO1_22_port 			1
#define PIO1_22_config(value)	LPC_IOCON->P1_22 = __TABLE81(value)
#define PIO1_22_pin				22
#define PIO1_23_port 			1
#define PIO1_23_config(value)	LPC_IOCON->P1_23 = __TABLE81(value)
#define PIO1_23_pin				23
#define PIO1_24_port 			1
#define PIO1_24_config(value)	LPC_IOCON->P1_24 = __TABLE81(value)
#define PIO1_24_pin				24
#define PIO1_25_port 			1
#define PIO1_25_config(value)	LPC_IOCON->P1_25 = __TABLE81(value)
#define PIO1_25_pin				25
#define PIO1_26_port 			1
#define PIO1_26_config(value)	LPC_IOCON->P1_26 = __TABLE81(value)
#define PIO1_26_pin				26
#define PIO1_27_port 			1
#define PIO1_27_config(value)	LPC_IOCON->P1_27 = __TABLE81(value)
#define PIO1_27_pin				27
#define PIO1_28_port 			1
#define PIO1_28_config(value)	LPC_IOCON->P1_28 = __TABLE81(value)
#define PIO1_28_pin				28
#define PIO1_29_port 			1
#define PIO1_29_config(value)	LPC_IOCON->P1_29 = __TABLE81(value)
#define PIO1_29_pin				29
#define PIO1_30_port 			1
#define PIO1_30_config(value)	LPC_IOCON->P1_30 = __TABLE83(value)
#define PIO1_30_pin				30
#define PIO1_31_port 			1
#define PIO1_31_config(value)	LPC_IOCON->P1_31 = __TABLE83(value)
#define PIO1_31_pin				31





#define PIO2_0_port 			2
#define PIO2_0_config(value)	LPC_IOCON->P2_0 = __TABLE81(value)
#define PIO2_0_pin				0
#define PIO2_1_port 			2
#define PIO2_1_config(value)	LPC_IOCON->P2_1 = __TABLE81(value)
#define PIO2_1_pin				1
#define PIO2_2_port 			2
#define PIO2_2_config(value)	LPC_IOCON->P2_2 = __TABLE81(value)
#define PIO2_2_pin				2
#define PIO2_3_port 			2
#define PIO2_3_config(value)	LPC_IOCON->P2_3 = __TABLE81(value)
#define PIO2_3_pin				3
#define PIO2_4_port 			2
#define PIO2_4_config(value)	LPC_IOCON->P2_4 = __TABLE81(value)
#define PIO2_4_pin				4
#define PIO2_5_port 			2
#define PIO2_5_config(value)	LPC_IOCON->P2_5 = __TABLE81(value)
#define PIO2_5_pin				5
#define PIO2_6_port 			2
#define PIO2_6_config(value)	LPC_IOCON->P2_6 = __TABLE81(value)
#define PIO2_6_pin				6
#define PIO2_7_port 			2
#define PIO2_7_config(value)	LPC_IOCON->P2_7 = __TABLE81(value)
#define PIO2_7_pin				7
#define PIO2_8_port 			2
#define PIO2_8_config(value)	LPC_IOCON->P2_8 = __TABLE81(value)
#define PIO2_8_pin				8
#define PIO2_9_port 			2
#define PIO2_9_config(value)	LPC_IOCON->P2_9 = __TABLE81(value)
#define PIO2_9_pin				9
#define PIO2_10_port 			2
#define PIO2_10_config(value)	LPC_IOCON->P2_10 = __TABLE81(value)
#define PIO2_10_pin				10
#define PIO2_11_port 			2
#define PIO2_11_config(value)	LPC_IOCON->P2_11 = __TABLE81(value)
#define PIO2_11_pin				11
#define PIO2_12_port 			2
#define PIO2_12_config(value)	LPC_IOCON->P2_12 = __TABLE81(value)
#define PIO2_12_pin				12
#define PIO2_13_port 			2
#define PIO2_13_config(value)	LPC_IOCON->P2_13 = __TABLE81(value)
#define PIO2_13_pin				13
#define PIO2_14_port 			2
#define PIO2_14_config(value)	LPC_IOCON->P2_14 = __TABLE81(value)
#define PIO2_14_pin				14
#define PIO2_15_port 			2
#define PIO2_15_config(value)	LPC_IOCON->P2_15 = __TABLE81(value)
#define PIO2_15_pin				15
#define PIO2_16_port 			2
#define PIO2_16_config(value)	LPC_IOCON->P2_16 = __TABLE81(value)
#define PIO2_16_pin				16
#define PIO2_17_port 			2
#define PIO2_17_config(value)	LPC_IOCON->P2_17 = __TABLE81(value)
#define PIO2_17_pin				17
#define PIO2_18_port 			2
#define PIO2_18_config(value)	LPC_IOCON->P2_18 = __TABLE81(value)
#define PIO2_18_pin				18
#define PIO2_19_port 			2
#define PIO2_19_config(value)	LPC_IOCON->P2_19 = __TABLE81(value)
#define PIO2_19_pin				19
#define PIO2_20_port 			2
#define PIO2_20_config(value)	LPC_IOCON->P2_20 = __TABLE81(value)
#define PIO2_20_pin				20
#define PIO2_21_port 			2
#define PIO2_21_config(value)	LPC_IOCON->P2_21 = __TABLE81(value)
#define PIO2_21_pin				21
#define PIO2_22_port 			2
#define PIO2_22_config(value)	LPC_IOCON->P2_22 = __TABLE81(value)
#define PIO2_22_pin				22
#define PIO2_23_port 			2
#define PIO2_23_config(value)	LPC_IOCON->P2_23 = __TABLE81(value)
#define PIO2_23_pin				23
#define PIO2_24_port 			2
#define PIO2_24_config(value)	LPC_IOCON->P2_24 = __TABLE81(value)
#define PIO2_24_pin				24
#define PIO2_25_port 			2
#define PIO2_25_config(value)	LPC_IOCON->P2_25 = __TABLE81(value)
#define PIO2_25_pin				25
#define PIO2_26_port 			2
#define PIO2_26_config(value)	LPC_IOCON->P2_26 = __TABLE81(value)
#define PIO2_26_pin				26
#define PIO2_27_port 			2
#define PIO2_27_config(value)	LPC_IOCON->P2_27 = __TABLE81(value)
#define PIO2_27_pin				27
#define PIO2_28_port 			2
#define PIO2_28_config(value)	LPC_IOCON->P2_28 = __TABLE81(value)
#define PIO2_28_pin				28
#define PIO2_29_port 			2
#define PIO2_29_config(value)	LPC_IOCON->P2_29 = __TABLE81(value)
#define PIO2_29_pin				29
#define PIO2_30_port 			2
#define PIO2_30_config(value)	LPC_IOCON->P2_30 = __TABLE81(value)
#define PIO2_30_pin				30
#define PIO2_31_port 			2
#define PIO2_31_config(value)	LPC_IOCON->P2_31 = __TABLE81(value)
#define PIO2_31_pin				31





#define PIO3_0_port 			3
#define PIO3_0_config(value)	LPC_IOCON->P3_0 = __TABLE81(value)
#define PIO3_0_pin				0
#define PIO3_1_port 			3
#define PIO3_1_config(value)	LPC_IOCON->P3_1 = __TABLE81(value)
#define PIO3_1_pin				1
#define PIO3_2_port 			3
#define PIO3_2_config(value)	LPC_IOCON->P3_2 = __TABLE81(value)
#define PIO3_2_pin				2
#define PIO3_3_port 			3
#define PIO3_3_config(value)	LPC_IOCON->P3_3 = __TABLE81(value)
#define PIO3_3_pin				3
#define PIO3_4_port 			3
#define PIO3_4_config(value)	LPC_IOCON->P3_4 = __TABLE81(value)
#define PIO3_4_pin				4
#define PIO3_5_port 			3
#define PIO3_5_config(value)	LPC_IOCON->P3_5 = __TABLE81(value)
#define PIO3_5_pin				5
#define PIO3_6_port 			3
#define PIO3_6_config(value)	LPC_IOCON->P3_6 = __TABLE81(value)
#define PIO3_6_pin				6
#define PIO3_7_port 			3
#define PIO3_7_config(value)	LPC_IOCON->P3_7 = __TABLE81(value)
#define PIO3_7_pin				7
#define PIO3_8_port 			3
#define PIO3_8_config(value)	LPC_IOCON->P3_8 = __TABLE81(value)
#define PIO3_8_pin				8
#define PIO3_9_port 			3
#define PIO3_9_config(value)	LPC_IOCON->P3_9 = __TABLE81(value)
#define PIO3_9_pin				9
#define PIO3_10_port 			3
#define PIO3_10_config(value)	LPC_IOCON->P3_10 = __TABLE81(value)
#define PIO3_10_pin				10
#define PIO3_11_port 			3
#define PIO3_11_config(value)	LPC_IOCON->P3_11 = __TABLE81(value)
#define PIO3_11_pin				11
#define PIO3_12_port 			3
#define PIO3_12_config(value)	LPC_IOCON->P3_12 = __TABLE81(value)
#define PIO3_12_pin				12
#define PIO3_13_port 			3
#define PIO3_13_config(value)	LPC_IOCON->P3_13 = __TABLE81(value)
#define PIO3_13_pin				13
#define PIO3_14_port 			3
#define PIO3_14_config(value)	LPC_IOCON->P3_14 = __TABLE81(value)
#define PIO3_14_pin				14
#define PIO3_15_port 			3
#define PIO3_15_config(value)	LPC_IOCON->P3_15 = __TABLE81(value)
#define PIO3_15_pin				15
#define PIO3_16_port 			3
#define PIO3_16_config(value)	LPC_IOCON->P3_16 = __TABLE81(value)
#define PIO3_16_pin				16
#define PIO3_17_port 			3
#define PIO3_17_config(value)	LPC_IOCON->P3_17 = __TABLE81(value)
#define PIO3_17_pin				17
#define PIO3_18_port 			3
#define PIO3_18_config(value)	LPC_IOCON->P3_18 = __TABLE81(value)
#define PIO3_18_pin				18
#define PIO3_19_port 			3
#define PIO3_19_config(value)	LPC_IOCON->P3_19 = __TABLE81(value)
#define PIO3_19_pin				19
#define PIO3_20_port 			3
#define PIO3_20_config(value)	LPC_IOCON->P3_20 = __TABLE81(value)
#define PIO3_20_pin				20
#define PIO3_21_port 			3
#define PIO3_21_config(value)	LPC_IOCON->P3_21 = __TABLE81(value)
#define PIO3_21_pin				21
#define PIO3_22_port 			3
#define PIO3_22_config(value)	LPC_IOCON->P3_22 = __TABLE81(value)
#define PIO3_22_pin				22
#define PIO3_23_port 			3
#define PIO3_23_config(value)	LPC_IOCON->P3_23 = __TABLE81(value)
#define PIO3_23_pin				23
#define PIO3_24_port 			3
#define PIO3_24_config(value)	LPC_IOCON->P3_24 = __TABLE81(value)
#define PIO3_24_pin				24
#define PIO3_25_port 			3
#define PIO3_25_config(value)	LPC_IOCON->P3_25 = __TABLE81(value)
#define PIO3_25_pin				25
#define PIO3_26_port 			3
#define PIO3_26_config(value)	LPC_IOCON->P3_26 = __TABLE81(value)
#define PIO3_26_pin				26
#define PIO3_27_port 			3
#define PIO3_27_config(value)	LPC_IOCON->P3_27 = __TABLE81(value)
#define PIO3_27_pin				27
#define PIO3_28_port 			3
#define PIO3_28_config(value)	LPC_IOCON->P3_28 = __TABLE81(value)
#define PIO3_28_pin				28
#define PIO3_29_port 			3
#define PIO3_29_config(value)	LPC_IOCON->P3_29 = __TABLE81(value)
#define PIO3_29_pin				29
#define PIO3_30_port 			3
#define PIO3_30_config(value)	LPC_IOCON->P3_30 = __TABLE81(value)
#define PIO3_30_pin				30
#define PIO3_31_port 			3
#define PIO3_31_config(value)	LPC_IOCON->P3_31 = __TABLE81(value)
#define PIO3_31_pin				31






#define PIO4_0_port 			4
#define PIO4_0_config(value)	LPC_IOCON->P3_0 = __TABLE81(value)
#define PIO4_0_pin				0
#define PIO4_1_port 			4
#define PIO4_1_config(value)	LPC_IOCON->P3_1 = __TABLE81(value)
#define PIO4_1_pin				1
#define PIO4_2_port 			4
#define PIO4_2_config(value)	LPC_IOCON->P3_2 = __TABLE81(value)
#define PIO4_2_pin				2
#define PIO4_3_port 			4
#define PIO4_3_config(value)	LPC_IOCON->P3_3 = __TABLE81(value)
#define PIO4_3_pin				3
#define PIO4_4_port 			4
#define PIO4_4_config(value)	LPC_IOCON->P3_4 = __TABLE81(value)
#define PIO4_4_pin				4
#define PIO4_5_port 			4
#define PIO4_5_config(value)	LPC_IOCON->P3_5 = __TABLE81(value)
#define PIO4_5_pin				5
#define PIO4_6_port 			4
#define PIO4_6_config(value)	LPC_IOCON->P3_6 = __TABLE81(value)
#define PIO4_6_pin				6
#define PIO4_7_port 			4
#define PIO4_7_config(value)	LPC_IOCON->P3_7 = __TABLE81(value)
#define PIO4_7_pin				7
#define PIO4_8_port 			4
#define PIO4_8_config(value)	LPC_IOCON->P3_8 = __TABLE81(value)
#define PIO4_8_pin				8
#define PIO4_9_port 			4
#define PIO4_9_config(value)	LPC_IOCON->P3_9 = __TABLE81(value)
#define PIO4_9_pin				9
#define PIO4_10_port 			4
#define PIO4_10_config(value)	LPC_IOCON->P3_10 = __TABLE81(value)
#define PIO4_10_pin				10
#define PIO4_11_port 			4
#define PIO4_11_config(value)	LPC_IOCON->P3_11 = __TABLE81(value)
#define PIO4_11_pin				11
#define PIO4_12_port 			4
#define PIO4_12_config(value)	LPC_IOCON->P3_12 = __TABLE81(value)
#define PIO4_12_pin				12
#define PIO4_13_port 			4
#define PIO4_13_config(value)	LPC_IOCON->P3_13 = __TABLE81(value)
#define PIO4_13_pin				13
#define PIO4_14_port 			4
#define PIO4_14_config(value)	LPC_IOCON->P3_14 = __TABLE81(value)
#define PIO4_14_pin				14
#define PIO4_15_port 			4
#define PIO4_15_config(value)	LPC_IOCON->P3_15 = __TABLE81(value)
#define PIO4_15_pin				15
#define PIO4_16_port 			4
#define PIO4_16_config(value)	LPC_IOCON->P3_16 = __TABLE81(value)
#define PIO4_16_pin				16
#define PIO4_17_port 			4
#define PIO4_17_config(value)	LPC_IOCON->P3_17 = __TABLE81(value)
#define PIO4_17_pin				17
#define PIO4_18_port 			4
#define PIO4_18_config(value)	LPC_IOCON->P3_18 = __TABLE81(value)
#define PIO4_18_pin				18
#define PIO4_19_port 			4
#define PIO4_19_config(value)	LPC_IOCON->P3_19 = __TABLE81(value)
#define PIO4_19_pin				19
#define PIO4_20_port 			4
#define PIO4_20_config(value)	LPC_IOCON->P3_20 = __TABLE81(value)
#define PIO4_20_pin				20
#define PIO4_21_port 			4
#define PIO4_21_config(value)	LPC_IOCON->P3_21 = __TABLE81(value)
#define PIO4_21_pin				21
#define PIO4_22_port 			4
#define PIO4_22_config(value)	LPC_IOCON->P3_22 = __TABLE81(value)
#define PIO4_22_pin				22
#define PIO4_23_port 			4
#define PIO4_23_config(value)	LPC_IOCON->P3_23 = __TABLE81(value)
#define PIO4_23_pin				23
#define PIO4_24_port 			4
#define PIO4_24_config(value)	LPC_IOCON->P3_24 = __TABLE81(value)
#define PIO4_24_pin				24
#define PIO4_25_port 			4
#define PIO4_25_config(value)	LPC_IOCON->P3_25 = __TABLE81(value)
#define PIO4_25_pin				25
#define PIO4_26_port 			4
#define PIO4_26_config(value)	LPC_IOCON->P3_26 = __TABLE81(value)
#define PIO4_26_pin				26
#define PIO4_27_port 			4
#define PIO4_27_config(value)	LPC_IOCON->P3_27 = __TABLE81(value)
#define PIO4_27_pin				27
#define PIO4_28_port 			4
#define PIO4_28_config(value)	LPC_IOCON->P3_28 = __TABLE81(value)
#define PIO4_28_pin				28
#define PIO4_29_port 			4
#define PIO4_29_config(value)	LPC_IOCON->P3_29 = __TABLE81(value)
#define PIO4_29_pin				29
#define PIO4_30_port 			4
#define PIO4_30_config(value)	LPC_IOCON->P3_30 = __TABLE81(value)
#define PIO4_30_pin				30
#define PIO4_31_port 			4
#define PIO4_31_config(value)	LPC_IOCON->P3_31 = __TABLE81(value)
#define PIO4_31_pin				31







#define PIO5_0_port 			5
#define PIO5_0_config(value)	LPC_IOCON->P5_0 = __TABLE81(value)
#define PIO5_0_pin				0
#define PIO5_1_port 			5
#define PIO5_1_config(value)	LPC_IOCON->P5_1 = __TABLE81(value)
#define PIO5_1_pin				1
#define PIO5_2_port 			5
#define PIO5_2_config(value)	LPC_IOCON->P5_2 = __TABLE87(value)
#define PIO5_2_pin				2
#define PIO5_3_port 			5
#define PIO5_3_config(value)	LPC_IOCON->P5_3 = __TABLE87(value)
#define PIO5_3_pin				3
#define PIO5_4_port 			5
#define PIO5_4_config(value)	LPC_IOCON->P5_4 = __TABLE81(value)
#define PIO5_4_pin				4


#endif


#endif /* UV_HAL_INC_UV_GPIO_LPC1785_H_ */
