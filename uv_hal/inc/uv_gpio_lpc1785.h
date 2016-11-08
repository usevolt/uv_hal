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


#define 	PIO0_0  	0
#define 	PIO0_1  	1
#define 	PIO0_2  	2
#define 	PIO0_3 		3
#define 	PIO0_4  	4
#define 	PIO0_5  	5
#define 	PIO0_6  	6
#define 	PIO0_7  	7
#define 	PIO0_8  	8
#define 	PIO0_9  	9
#define 	PIO0_10		10
#define 	PIO0_11		11
#define 	PIO0_12		12
#define 	PIO0_13		13
#define 	PIO0_14     14
#define 	PIO0_15     15
#define 	PIO0_16     16
#define 	PIO0_17     17
#define 	PIO0_18     18
#define 	PIO0_19     19
#define 	PIO0_20     20
#define 	PIO0_21     21
#define 	PIO0_22     22
#define 	PIO0_23     23
#define 	PIO0_24     24
#define 	PIO0_25     25
#define 	PIO0_26     26
#define 	PIO0_27     27
#define 	PIO0_28     28
#define 	PIO0_29     29
#define 	PIO0_30     30
#define 	PIO0_31     31
#define 	PIO1_0      32
#define 	PIO1_1      33
#define 	PIO1_2      34
#define 	PIO1_3      35
#define 	PIO1_4      36
#define 	PIO1_5      37
#define 	PIO1_6      38
#define 	PIO1_7      39
#define 	PIO1_8      40
#define 	PIO1_9      41
#define 	PIO1_10     42
#define 	PIO1_11     43
#define 	PIO1_12     44
#define 	PIO1_13     45
#define 	PIO1_14     46
#define 	PIO1_15     47
#define 	PIO1_16     48
#define 	PIO1_17     49
#define 	PIO1_18     50
#define 	PIO1_19     51
#define 	PIO1_20     52
#define 	PIO1_21     53
#define 	PIO1_22     54
#define 	PIO1_23     55
#define 	PIO1_24     56
#define 	PIO1_25     57
#define 	PIO1_26     58
#define 	PIO1_27     59
#define 	PIO1_28     60
#define 	PIO1_29     61
#define 	PIO1_30     62
#define 	PIO1_31     63
#define 	PIO2_0      64
#define 	PIO2_1      65
#define 	PIO2_2      66
#define 	PIO2_3      67
#define 	PIO2_4      68
#define 	PIO2_5      69
#define 	PIO2_6      70
#define 	PIO2_7      71
#define 	PIO2_8      72
#define 	PIO2_9      73
#define 	PIO2_10     74
#define 	PIO2_11     75
#define 	PIO2_12     76
#define 	PIO2_13     77
#define 	PIO2_14     78
#define 	PIO2_15     79
#define 	PIO2_16     80
#define 	PIO2_17     81
#define 	PIO2_18     82
#define 	PIO2_19     83
#define 	PIO2_20     84
#define 	PIO2_21     85
#define 	PIO2_22     86
#define 	PIO2_23     87
#define 	PIO2_24     88
#define 	PIO2_25     89
#define 	PIO2_26     90
#define 	PIO2_27     91
#define 	PIO2_28     92
#define 	PIO2_29     93
#define 	PIO2_30     94
#define 	PIO2_31     95
#define 	PIO3_0      96
#define 	PIO3_1      97
#define 	PIO3_2      98
#define 	PIO3_3      99
#define 	PIO3_4      100
#define 	PIO3_5      101
#define 	PIO3_6      102
#define 	PIO3_7      103
#define 	PIO3_8      104
#define 	PIO3_9      105
#define 	PIO3_10     106
#define 	PIO3_11     107
#define 	PIO3_12     108
#define 	PIO3_13     109
#define 	PIO3_14     110
#define 	PIO3_15     111
#define 	PIO3_16     112
#define 	PIO3_17     113
#define 	PIO3_18     114
#define 	PIO3_19     115
#define 	PIO3_20     116
#define 	PIO3_21     117
#define 	PIO3_22     118
#define 	PIO3_23     119
#define 	PIO3_24     120
#define 	PIO3_25     121
#define 	PIO3_26     122
#define 	PIO3_27     123
#define 	PIO3_28     124
#define 	PIO3_29     125
#define 	PIO3_30     126
#define 	PIO3_31     127
#define 	PIO4_0      128
#define 	PIO4_1      129
#define 	PIO4_2      130
#define 	PIO4_3      131
#define 	PIO4_4      132
#define 	PIO4_5      133
#define 	PIO4_6      134
#define 	PIO4_7      135
#define 	PIO4_8      136
#define 	PIO4_9      137
#define 	PIO4_10     138
#define 	PIO4_11     139
#define 	PIO4_12     140
#define 	PIO4_13     141
#define 	PIO4_14     142
#define 	PIO4_15     143
#define 	PIO4_16     144
#define 	PIO4_17     145
#define 	PIO4_18     146
#define 	PIO4_19     147
#define 	PIO4_20     148
#define 	PIO4_21     149
#define 	PIO4_22     150
#define 	PIO4_23     151
#define 	PIO4_24     152
#define 	PIO4_25     153
#define 	PIO4_26     154
#define 	PIO4_27     155
#define 	PIO4_28     156
#define 	PIO4_29     157
#define 	PIO4_30     158
#define 	PIO4_31     159
#define 	PIO5_0      160
#define 	PIO5_1      161
#define 	PIO5_2      162
#define 	PIO5_3      163
#define 	PIO5_4      164

typedef uint32_t uv_gpios_e;


#define port(port_num)			CAT(LPC_GPIO, port_num)


#define GPIO_0_port 			0
#define GPIO_0_config(value)	LPC_IOCON->P0_0 = __TABLE81(value)
#define GPIO_0_pin				0
#define GPIO_1_port 			0
#define GPIO_1_config(value)	LPC_IOCON->P0_1 = __TABLE81(value)
#define GPIO_1_pin				1
#define GPIO_2_port 			0
#define GPIO_2_config(value)	LPC_IOCON->P0_2 = __TABLE81(value)
#define GPIO_2_pin				2
#define GPIO_3_port 			0
#define GPIO_3_config(value)	LPC_IOCON->P0_3 = __TABLE81(value)
#define GPIO_3_pin				3
#define GPIO_4_port 			0
#define GPIO_4_config(value)	LPC_IOCON->P0_4 = __TABLE81(value)
#define GPIO_4_pin				4
#define GPIO_5_port 			0
#define GPIO_5_config(value)	LPC_IOCON->P0_5 = __TABLE81(value)
#define GPIO_5_pin				5
#define GPIO_6_port 			0
#define GPIO_6_config(value)	LPC_IOCON->P0_6 = __TABLE81(value)
#define GPIO_6_pin				6
#define GPIO_7_port 			0
#define GPIO_7_config(value)	LPC_IOCON->P0_7 = __TABLE89(value)
#define GPIO_7_pin				7
#define GPIO_8_port 			0
#define GPIO_8_config(value)	LPC_IOCON->P0_8 = __TABLE89(value)
#define GPIO_8_pin				8
#define GPIO_9_port 			0
#define GPIO_9_config(value)	LPC_IOCON->P0_9 = __TABLE89(value)
#define GPIO_9_pin				9
#define GPIO_10_port 			0
#define GPIO_10_config(value)	LPC_IOCON->P0_10 = __TABLE81(value)
#define GPIO_10_pin				10
#define GPIO_11_port 			0
#define GPIO_11_config(value)	LPC_IOCON->P0_11 = __TABLE81(value)
#define GPIO_11_pin				11
#define GPIO_12_port 			0
#define GPIO_12_config(value)	LPC_IOCON->P0_12 = __TABLE83(value)
#define GPIO_12_pin				12
#define GPIO_13_port 			0
#define GPIO_13_config(value)	LPC_IOCON->P0_13 = __TABLE83(value)
#define GPIO_13_pin				13
#define GPIO_14_port 			0
#define GPIO_14_config(value)	LPC_IOCON->P0_14 = __TABLE81(value)
#define GPIO_14_pin				14
#define GPIO_15_port 			0
#define GPIO_15_config(value)	LPC_IOCON->P0_15 = __TABLE81(value)
#define GPIO_15_pin				15
#define GPIO_16_port 			0
#define GPIO_16_config(value)	LPC_IOCON->P0_16 = __TABLE81(value)
#define GPIO_16_pin				16
#define GPIO_17_port 			0
#define GPIO_17_config(value)	LPC_IOCON->P0_17 = __TABLE81(value)
#define GPIO_17_pin				17
#define GPIO_18_port 			0
#define GPIO_18_config(value)	LPC_IOCON->P0_18 = __TABLE81(value)
#define GPIO_18_pin				18
#define GPIO_19_port 			0
#define GPIO_19_config(value)	LPC_IOCON->P0_19 = __TABLE81(value)
#define GPIO_19_pin				19
#define GPIO_20_port 			0
#define GPIO_20_config(value)	LPC_IOCON->P0_20 = __TABLE81(value)
#define GPIO_20_pin				20
#define GPIO_21_port 			0
#define GPIO_21_config(value)	LPC_IOCON->P0_21 = __TABLE81(value)
#define GPIO_21_pin				21
#define GPIO_22_port 			0
#define GPIO_22_config(value)	LPC_IOCON->P0_22 = __TABLE81(value)
#define GPIO_22_pin				22
#define GPIO_23_port 			0
#define GPIO_23_config(value)	LPC_IOCON->P0_23 = __TABLE83(value)
#define GPIO_23_pin				23
#define GPIO_24_port 			0
#define GPIO_24_config(value)	LPC_IOCON->P0_24 = __TABLE83(value)
#define GPIO_24_pin				24
#define GPIO_25_port 			0
#define GPIO_25_config(value)	LPC_IOCON->P0_25 = __TABLE83(value)
#define GPIO_25_pin				25
#define GPIO_26_port 			0
#define GPIO_26_config(value)	LPC_IOCON->P0_26 = __TABLE83(value)
#define GPIO_26_pin				26
#define GPIO_27_port 			0
#define GPIO_27_config(value)	LPC_IOCON->P0_27 = __TABLE87(value)
#define GPIO_27_pin				27
#define GPIO_28_port 			0
#define GPIO_28_config(value)	LPC_IOCON->P0_28 = __TABLE87(value)
#define GPIO_28_pin				28
#define GPIO_29_port 			0
#define GPIO_29_config(value)	LPC_IOCON->P0_29 = __TABLE85(value)
#define GPIO_29_pin				29
#define GPIO_30_port 			0
#define GPIO_30_config(value)	LPC_IOCON->P0_30 = __TABLE85(value)
#define GPIO_30_pin				30
#define GPIO_31_port 			0
#define GPIO_31_config(value)	LPC_IOCON->P0_31 = __TABLE85(value)
#define GPIO_31_pin				31




#define GPIO_32_port 			1
#define GPIO_32_config(value)	LPC_IOCON->P1_0 = __TABLE81(value)
#define GPIO_32_pin				0
#define GPIO_33_port 			1
#define GPIO_33_config(value)	LPC_IOCON->P1_1 = __TABLE81(value)
#define GPIO_33_pin				1
#define GPIO_34_port 			1
#define GPIO_34_config(value)	LPC_IOCON->P1_2 = __TABLE81(value)
#define GPIO_34_pin				2
#define GPIO_35_port 			1
#define GPIO_35_config(value)	LPC_IOCON->P1_3 = __TABLE81(value)
#define GPIO_35_pin				3
#define GPIO_36_port 			1
#define GPIO_36_config(value)	LPC_IOCON->P1_4 = __TABLE81(value)
#define GPIO_36_pin				4
#define GPIO_37_port 			1
#define GPIO_37_config(value)	LPC_IOCON->P1_5 = __TABLE89(value)
#define GPIO_37_pin				5
#define GPIO_38_port 			1
#define GPIO_38_config(value)	LPC_IOCON->P1_6 = __TABLE89(value)
#define GPIO_38_pin				6
#define GPIO_39_port 			1
#define GPIO_39_config(value)	LPC_IOCON->P1_7 = __TABLE89(value)
#define GPIO_39_pin				7
#define GPIO_40_port 			1
#define GPIO_40_config(value)	LPC_IOCON->P1_8 = __TABLE81(value)
#define GPIO_40_pin				8
#define GPIO_41_port 			1
#define GPIO_41_config(value)	LPC_IOCON->P1_9 = __TABLE81(value)
#define GPIO_41_pin				9
#define GPIO_42_port 			1
#define GPIO_42_config(value)	LPC_IOCON->P1_10 = __TABLE81(value)
#define GPIO_42_pin				10
#define GPIO_43_port 			1
#define GPIO_43_config(value)	LPC_IOCON->P1_11 = __TABLE81(value)
#define GPIO_43_pin				11
#define GPIO_44_port 			1
#define GPIO_44_config(value)	LPC_IOCON->P1_12 = __TABLE81(value)
#define GPIO_44_pin				12
#define GPIO_45_port 			1
#define GPIO_45_config(value)	LPC_IOCON->P1_13 = __TABLE81(value)
#define GPIO_45_pin				13
#define GPIO_46_port 			1
#define GPIO_46_config(value)	LPC_IOCON->P1_14 = __TABLE89(value)
#define GPIO_46_pin				14
#define GPIO_47_port 			1
#define GPIO_47_config(value)	LPC_IOCON->P1_15 = __TABLE81(value)
#define GPIO_47_pin				15
#define GPIO_48_port 			1
#define GPIO_48_config(value)	LPC_IOCON->P1_16 = __TABLE89(value)
#define GPIO_48_pin				16
#define GPIO_49_port 			1
#define GPIO_49_config(value)	LPC_IOCON->P1_17 = __TABLE89(value)
#define GPIO_49_pin				17
#define GPIO_50_port 			1
#define GPIO_50_config(value)	LPC_IOCON->P1_18 = __TABLE81(value)
#define GPIO_50_pin				18
#define GPIO_51_port 			1
#define GPIO_51_config(value)	LPC_IOCON->P1_19 = __TABLE81(value)
#define GPIO_51_pin				19
#define GPIO_52_port 			1
#define GPIO_52_config(value)	LPC_IOCON->P1_20 = __TABLE81(value)
#define GPIO_52_pin				20
#define GPIO_53_port 			1
#define GPIO_53_config(value)	LPC_IOCON->P1_21 = __TABLE81(value)
#define GPIO_53_pin				21
#define GPIO_54_port 			1
#define GPIO_54_config(value)	LPC_IOCON->P1_22 = __TABLE81(value)
#define GPIO_54_pin				22
#define GPIO_55_port 			1
#define GPIO_55_config(value)	LPC_IOCON->P1_23 = __TABLE81(value)
#define GPIO_55_pin				23
#define GPIO_56_port 			1
#define GPIO_56_config(value)	LPC_IOCON->P1_24 = __TABLE81(value)
#define GPIO_56_pin				24
#define GPIO_57_port 			1
#define GPIO_57_config(value)	LPC_IOCON->P1_25 = __TABLE81(value)
#define GPIO_57_pin				25
#define GPIO_58_port 			1
#define GPIO_58_config(value)	LPC_IOCON->P1_26 = __TABLE81(value)
#define GPIO_58_pin				26
#define GPIO_59_port 			1
#define GPIO_59_config(value)	LPC_IOCON->P1_27 = __TABLE81(value)
#define GPIO_59_pin				27
#define GPIO_60_port 			1
#define GPIO_60_config(value)	LPC_IOCON->P1_28 = __TABLE81(value)
#define GPIO_60_pin				28
#define GPIO_61_port 			1
#define GPIO_61_config(value)	LPC_IOCON->P1_29 = __TABLE81(value)
#define GPIO_61_pin				29
#define GPIO_62_port 			1
#define GPIO_62_config(value)	LPC_IOCON->P1_30 = __TABLE83(value)
#define GPIO_62_pin				30
#define GPIO_63_port 			1
#define GPIO_63_config(value)	LPC_IOCON->P1_31 = __TABLE83(value)
#define GPIO_63_pin				31




#define GPIO_64_port 			2
#define GPIO_64_config(value)	LPC_IOCON->P2_0 = __TABLE81(value)
#define GPIO_64_pin				0
#define GPIO_65_port 			2
#define GPIO_65_config(value)	LPC_IOCON->P2_1 = __TABLE81(value)
#define GPIO_65_pin				1
#define GPIO_66_port 			2
#define GPIO_66_config(value)	LPC_IOCON->P2_2 = __TABLE81(value)
#define GPIO_66_pin				2
#define GPIO_67_port 			2
#define GPIO_67_config(value)	LPC_IOCON->P2_3 = __TABLE81(value)
#define GPIO_67_pin				3
#define GPIO_68_port 			2
#define GPIO_68_config(value)	LPC_IOCON->P2_4 = __TABLE81(value)
#define GPIO_68_pin				4
#define GPIO_69_port 			2
#define GPIO_69_config(value)	LPC_IOCON->P2_5 = __TABLE81(value)
#define GPIO_69_pin				5
#define GPIO_70_port 			2
#define GPIO_70_config(value)	LPC_IOCON->P2_6 = __TABLE81(value)
#define GPIO_70_pin				6
#define GPIO_71_port 			2
#define GPIO_71_config(value)	LPC_IOCON->P2_7 = __TABLE81(value)
#define GPIO_71_pin				7
#define GPIO_72_port 			2
#define GPIO_72_config(value)	LPC_IOCON->P2_8 = __TABLE81(value)
#define GPIO_72_pin				8
#define GPIO_73_port 			2
#define GPIO_73_config(value)	LPC_IOCON->P2_9 = __TABLE81(value)
#define GPIO_73_pin				9
#define GPIO_74_port 			2
#define GPIO_74_config(value)	LPC_IOCON->P2_10 = __TABLE81(value)
#define GPIO_74_pin				10
#define GPIO_75_port 			2
#define GPIO_75_config(value)	LPC_IOCON->P2_11 = __TABLE81(value)
#define GPIO_75_pin				11
#define GPIO_76_port 			2
#define GPIO_76_config(value)	LPC_IOCON->P2_12 = __TABLE81(value)
#define GPIO_76_pin				12
#define GPIO_77_port 			2
#define GPIO_77_config(value)	LPC_IOCON->P2_13 = __TABLE81(value)
#define GPIO_77_pin				13
#define GPIO_78_port 			2
#define GPIO_78_config(value)	LPC_IOCON->P2_14 = __TABLE81(value)
#define GPIO_78_pin				14
#define GPIO_79_port 			2
#define GPIO_79_config(value)	LPC_IOCON->P2_15 = __TABLE81(value)
#define GPIO_79_pin				15
#define GPIO_80_port 			2
#define GPIO_80_config(value)	LPC_IOCON->P2_16 = __TABLE81(value)
#define GPIO_80_pin				16
#define GPIO_81_port 			2
#define GPIO_81_config(value)	LPC_IOCON->P2_17 = __TABLE81(value)
#define GPIO_81_pin				17
#define GPIO_82_port 			2
#define GPIO_82_config(value)	LPC_IOCON->P2_18 = __TABLE81(value)
#define GPIO_82_pin				18
#define GPIO_83_port 			2
#define GPIO_83_config(value)	LPC_IOCON->P2_19 = __TABLE81(value)
#define GPIO_83_pin				19
#define GPIO_84_port 			2
#define GPIO_84_config(value)	LPC_IOCON->P2_20 = __TABLE81(value)
#define GPIO_84_pin				20
#define GPIO_85_port 			2
#define GPIO_85_config(value)	LPC_IOCON->P2_21 = __TABLE81(value)
#define GPIO_85_pin				21
#define GPIO_86_port 			2
#define GPIO_86_config(value)	LPC_IOCON->P2_22 = __TABLE81(value)
#define GPIO_86_pin				22
#define GPIO_87_port 			2
#define GPIO_87_config(value)	LPC_IOCON->P2_23 = __TABLE81(value)
#define GPIO_87_pin				23
#define GPIO_88_port 			2
#define GPIO_88_config(value)	LPC_IOCON->P2_24 = __TABLE81(value)
#define GPIO_88_pin				24
#define GPIO_89_port 			2
#define GPIO_89_config(value)	LPC_IOCON->P2_25 = __TABLE81(value)
#define GPIO_89_pin				25
#define GPIO_90_port 			2
#define GPIO_90_config(value)	LPC_IOCON->P2_26 = __TABLE81(value)
#define GPIO_90_pin				26
#define GPIO_91_port 			2
#define GPIO_91_config(value)	LPC_IOCON->P2_27 = __TABLE81(value)
#define GPIO_91_pin				27
#define GPIO_92_port 			2
#define GPIO_92_config(value)	LPC_IOCON->P2_28 = __TABLE81(value)
#define GPIO_92_pin				28
#define GPIO_93_port 			2
#define GPIO_93_config(value)	LPC_IOCON->P2_29 = __TABLE81(value)
#define GPIO_93_pin				29
#define GPIO_94_port 			2
#define GPIO_94_config(value)	LPC_IOCON->P2_30 = __TABLE81(value)
#define GPIO_94_pin				30
#define GPIO_95_port 			2
#define GPIO_95_config(value)	LPC_IOCON->P2_31 = __TABLE81(value)
#define GPIO_95_pin				31





#define GPIO_96_port 			3
#define GPIO_96_config(value)	LPC_IOCON->P3_0 = __TABLE81(value)
#define GPIO_96_pin				0
#define GPIO_97_port 			3
#define GPIO_97_config(value)	LPC_IOCON->P3_1 = __TABLE81(value)
#define GPIO_97_pin				1
#define GPIO_98_port 			3
#define GPIO_98_config(value)	LPC_IOCON->P3_2 = __TABLE81(value)
#define GPIO_98_pin				2
#define GPIO_99_port 			3
#define GPIO_99_config(value)	LPC_IOCON->P3_3 = __TABLE81(value)
#define GPIO_99_pin				3
#define GPIO_100_port 			3
#define GPIO_100_config(value)	LPC_IOCON->P3_4 = __TABLE81(value)
#define GPIO_100_pin				4
#define GPIO_101_port 			3
#define GPIO_101_config(value)	LPC_IOCON->P3_5 = __TABLE81(value)
#define GPIO_101_pin				5
#define GPIO_102_port 			3
#define GPIO_102_config(value)	LPC_IOCON->P3_6 = __TABLE81(value)
#define GPIO_102_pin				6
#define GPIO_103_port 			3
#define GPIO_103_config(value)	LPC_IOCON->P3_7 = __TABLE81(value)
#define GPIO_103_pin				7
#define GPIO_104_port 			3
#define GPIO_104_config(value)	LPC_IOCON->P3_8 = __TABLE81(value)
#define GPIO_104_pin				8
#define GPIO_105_port 			3
#define GPIO_105_config(value)	LPC_IOCON->P3_9 = __TABLE81(value)
#define GPIO_105_pin				9
#define GPIO_106_port 			3
#define GPIO_106_config(value)	LPC_IOCON->P3_10 = __TABLE81(value)
#define GPIO_106_pin				10
#define GPIO_107_port 			3
#define GPIO_107_config(value)	LPC_IOCON->P3_11 = __TABLE81(value)
#define GPIO_107_pin				11
#define GPIO_108_port 			3
#define GPIO_108_config(value)	LPC_IOCON->P3_12 = __TABLE81(value)
#define GPIO_108_pin				12
#define GPIO_109_port 			3
#define GPIO_109_config(value)	LPC_IOCON->P3_13 = __TABLE81(value)
#define GPIO_109_pin				13
#define GPIO_110_port 			3
#define GPIO_110_config(value)	LPC_IOCON->P3_14 = __TABLE81(value)
#define GPIO_110_pin				14
#define GPIO_111_port 			3
#define GPIO_111_config(value)	LPC_IOCON->P3_15 = __TABLE81(value)
#define GPIO_111_pin				15
#define GPIO_112_port 			3
#define GPIO_112_config(value)	LPC_IOCON->P3_16 = __TABLE81(value)
#define GPIO_112_pin				16
#define GPIO_113_port 			3
#define GPIO_113_config(value)	LPC_IOCON->P3_17 = __TABLE81(value)
#define GPIO_113_pin				17
#define GPIO_114_port 			3
#define GPIO_114_config(value)	LPC_IOCON->P3_18 = __TABLE81(value)
#define GPIO_114_pin				18
#define GPIO_115_port 			3
#define GPIO_115_config(value)	LPC_IOCON->P3_19 = __TABLE81(value)
#define GPIO_115_pin				19
#define GPIO_116_port 			3
#define GPIO_116_config(value)	LPC_IOCON->P3_20 = __TABLE81(value)
#define GPIO_116_pin				20
#define GPIO_117_port 			3
#define GPIO_117_config(value)	LPC_IOCON->P3_21 = __TABLE81(value)
#define GPIO_117_pin				21
#define GPIO_118_port 			3
#define GPIO_118_config(value)	LPC_IOCON->P3_22 = __TABLE81(value)
#define GPIO_118_pin				22
#define GPIO_119_port 			3
#define GPIO_119_config(value)	LPC_IOCON->P3_23 = __TABLE81(value)
#define GPIO_119_pin				23
#define GPIO_120_port 			3
#define GPIO_120_config(value)	LPC_IOCON->P3_24 = __TABLE81(value)
#define GPIO_120_pin				24
#define GPIO_121_port 			3
#define GPIO_121_config(value)	LPC_IOCON->P3_25 = __TABLE81(value)
#define GPIO_121_pin				25
#define GPIO_122_port 			3
#define GPIO_122_config(value)	LPC_IOCON->P3_26 = __TABLE81(value)
#define GPIO_122_pin				26
#define GPIO_123_port 			3
#define GPIO_123_config(value)	LPC_IOCON->P3_27 = __TABLE81(value)
#define GPIO_123_pin				27
#define GPIO_124_port 			3
#define GPIO_124_config(value)	LPC_IOCON->P3_28 = __TABLE81(value)
#define GPIO_124_pin				28
#define GPIO_125_port 			3
#define GPIO_125_config(value)	LPC_IOCON->P3_29 = __TABLE81(value)
#define GPIO_125_pin				29
#define GPIO_126_port 			3
#define GPIO_126_config(value)	LPC_IOCON->P3_30 = __TABLE81(value)
#define GPIO_126_pin				30
#define GPIO_127_port 			3
#define GPIO_127_config(value)	LPC_IOCON->P3_31 = __TABLE81(value)
#define GPIO_127_pin				31






#define GPIO_128_port 			4
#define GPIO_128_config(value)	LPC_IOCON->P4_0 = __TABLE81(value)
#define GPIO_128_pin				0
#define GPIO_129_port 			4
#define GPIO_129_config(value)	LPC_IOCON->P4_1 = __TABLE81(value)
#define GPIO_129_pin				1
#define GPIO_130_port 			4
#define GPIO_130_config(value)	LPC_IOCON->P4_2 = __TABLE81(value)
#define GPIO_130_pin				2
#define GPIO_131_port 			4
#define GPIO_131_config(value)	LPC_IOCON->P4_3 = __TABLE81(value)
#define GPIO_131_pin				3
#define GPIO_132_port 			4
#define GPIO_132_config(value)	LPC_IOCON->P4_4 = __TABLE81(value)
#define GPIO_132_pin				4
#define GPIO_133_port 			4
#define GPIO_133_config(value)	LPC_IOCON->P4_5 = __TABLE81(value)
#define GPIO_133_pin				5
#define GPIO_134_port 			4
#define GPIO_134_config(value)	LPC_IOCON->P4_6 = __TABLE81(value)
#define GPIO_134_pin				6
#define GPIO_135_port 			4
#define GPIO_135_config(value)	LPC_IOCON->P4_7 = __TABLE81(value)
#define GPIO_135_pin				7
#define GPIO_136_port 			4
#define GPIO_136_config(value)	LPC_IOCON->P4_8 = __TABLE81(value)
#define GPIO_136_pin				8
#define GPIO_137_port 			4
#define GPIO_137_config(value)	LPC_IOCON->P4_9 = __TABLE81(value)
#define GPIO_137_pin				9
#define GPIO_138_port 			4
#define GPIO_138_config(value)	LPC_IOCON->P4_10 = __TABLE81(value)
#define GPIO_138_pin				10
#define GPIO_139_port 			4
#define GPIO_139_config(value)	LPC_IOCON->P4_11 = __TABLE81(value)
#define GPIO_139_pin				11
#define GPIO_140_port 			4
#define GPIO_140_config(value)	LPC_IOCON->P4_12 = __TABLE81(value)
#define GPIO_140_pin				12
#define GPIO_141_port 			4
#define GPIO_141_config(value)	LPC_IOCON->P4_13 = __TABLE81(value)
#define GPIO_141_pin				13
#define GPIO_142_port 			4
#define GPIO_142_config(value)	LPC_IOCON->P4_14 = __TABLE81(value)
#define GPIO_142_pin				14
#define GPIO_143_port 			4
#define GPIO_143_config(value)	LPC_IOCON->P4_15 = __TABLE81(value)
#define GPIO_143_pin				15
#define GPIO_144_port 			4
#define GPIO_144_config(value)	LPC_IOCON->P4_16 = __TABLE81(value)
#define GPIO_144_pin				16
#define GPIO_145_port 			4
#define GPIO_145_config(value)	LPC_IOCON->P4_17 = __TABLE81(value)
#define GPIO_145_pin				17
#define GPIO_146_port 			4
#define GPIO_146_config(value)	LPC_IOCON->P4_18 = __TABLE81(value)
#define GPIO_146_pin				18
#define GPIO_147_port 			4
#define GPIO_147_config(value)	LPC_IOCON->P4_19 = __TABLE81(value)
#define GPIO_147_pin				19
#define GPIO_148_port 			4
#define GPIO_148_config(value)	LPC_IOCON->P4_20 = __TABLE81(value)
#define GPIO_148_pin				20
#define GPIO_149_port 			4
#define GPIO_149_config(value)	LPC_IOCON->P4_21 = __TABLE81(value)
#define GPIO_149_pin				21
#define GPIO_150_port 			4
#define GPIO_150_config(value)	LPC_IOCON->P4_22 = __TABLE81(value)
#define GPIO_150_pin				22
#define GPIO_151_port 			4
#define GPIO_151_config(value)	LPC_IOCON->P4_23 = __TABLE81(value)
#define GPIO_151_pin				23
#define GPIO_152_port 			4
#define GPIO_152_config(value)	LPC_IOCON->P4_24 = __TABLE81(value)
#define GPIO_152_pin				24
#define GPIO_153_port 			4
#define GPIO_153_config(value)	LPC_IOCON->P4_25 = __TABLE81(value)
#define GPIO_153_pin				25
#define GPIO_154_port 			4
#define GPIO_154_config(value)	LPC_IOCON->P4_26 = __TABLE81(value)
#define GPIO_154_pin				26
#define GPIO_155_port 			4
#define GPIO_155_config(value)	LPC_IOCON->P4_27 = __TABLE81(value)
#define GPIO_155_pin				27
#define GPIO_156_port 			4
#define GPIO_156_config(value)	LPC_IOCON->P4_28 = __TABLE81(value)
#define GPIO_156_pin				28
#define GPIO_157_port 			4
#define GPIO_157_config(value)	LPC_IOCON->P4_29 = __TABLE81(value)
#define GPIO_157_pin				29
#define GPIO_158_port 			4
#define GPIO_158_config(value)	LPC_IOCON->P4_30 = __TABLE81(value)
#define GPIO_158_pin				30
#define GPIO_159_port 			4
#define GPIO_159_config(value)	LPC_IOCON->P4_31 = __TABLE81(value)
#define GPIO_159_pin				31







#define GPIO_160_port 			5
#define GPIO_160_config(value)	LPC_IOCON->P5_0 = __TABLE81(value)
#define GPIO_160_pin				0
#define GPIO_161_port 			5
#define GPIO_161_config(value)	LPC_IOCON->P5_1 = __TABLE81(value)
#define GPIO_161_pin				1
#define GPIO_162_port 			5
#define GPIO_162_config(value)	LPC_IOCON->P5_2 = __TABLE87(value)
#define GPIO_162_pin				2
#define GPIO_163_port 			5
#define GPIO_163_config(value)	LPC_IOCON->P5_3 = __TABLE87(value)
#define GPIO_163_pin				3
#define GPIO_164_port 			5
#define GPIO_164_config(value)	LPC_IOCON->P5_4 = __TABLE81(value)
#define GPIO_164_pin				4


#endif


#endif /* UV_HAL_INC_UV_GPIO_LPC1785_H_ */
