/*
 * uw_emc.c
 *
 *  Created on: Mar 29, 2016
 *      Author: usevolt
 */



#include "uw_emc.h"
#include "uw_gpio.h"

#if CONFIG_TARGET_LPC178X


uw_errors_e uw_emc_init( void ) {

	// mode: EMC,
	// hysteresis enabled,
	// Slew mode disabled
	unsigned int iocon_value = 0b10001001;
	// set IOCON registers
#if CONFIG_EMC_STATIC_RAM
	LPC_IOCON->P2_14 = iocon_value;		// EMC_CS2
	LPC_IOCON->P2_15 = iocon_value;		// EMC_CS3
#endif
#if CONFIG_EMC_DYNAMIC_RAM
	LPC_IOCON->P2_16 = iocon_value;		// EMC_CAS
	LPC_IOCON->P2_17 = iocon_value;		// EMC_RAS
	LPC_IOCON->P2_18 = iocon_value;		// EMC_CLK0
	LPC_IOCON->P2_19 = iocon_value;		// EMC_CLK1
	LPC_IOCON->P2_20 = iocon_value;		// EMC_DYCS0
	LPC_IOCON->P2_21 = iocon_value;		// EMC_DYCS1
	LPC_IOCON->P2_22 = iocon_value;		// EMC_DYCS2
	LPC_IOCON->P2_23 = iocon_value;		// EMC_DYCS3
	LPC_IOCON->P2_24 = iocon_value;		// EMC_CKE0
	LPC_IOCON->P2_25 = iocon_value;		// EMC_CKE1
	LPC_IOCON->P2_26 = iocon_value;		// EMC_CKE2
	LPC_IOCON->P2_27 = iocon_value;		// EMC_CKE3
#endif
	LPC_IOCON->P2_28 = iocon_value;		// EMC_DQM0
	LPC_IOCON->P2_29 = iocon_value;		// EMC_DQM1
	LPC_IOCON->P2_30 = iocon_value;		// EMC_DQM2
	LPC_IOCON->P2_31 = iocon_value;		// EMC_DQM3
	LPC_IOCON->P3_0 = iocon_value;		// EMC_D0
	LPC_IOCON->P3_1 = iocon_value;		// EMC_D1
	LPC_IOCON->P3_2 = iocon_value;		// EMC_D2
	LPC_IOCON->P3_3 = iocon_value;		// EMC_D3
	LPC_IOCON->P3_4 = iocon_value;		// EMC_D4
	LPC_IOCON->P3_5 = iocon_value;		// EMC_D5
	LPC_IOCON->P3_6 = iocon_value;		// EMC_D6
	LPC_IOCON->P3_7 = iocon_value;		// EMC_D7
	LPC_IOCON->P3_8 = iocon_value;		// EMC_D8
	LPC_IOCON->P3_9 = iocon_value;		// EMC_D9
	LPC_IOCON->P3_10 = iocon_value;		// EMC_D10
	LPC_IOCON->P3_11 = iocon_value;		// EMC_D11
	LPC_IOCON->P3_12 = iocon_value;		// EMC_D12
	LPC_IOCON->P3_13 = iocon_value;		// EMC_D13
	LPC_IOCON->P3_14 = iocon_value;		// EMC_D14
	LPC_IOCON->P3_15 = iocon_value;		// EMC_D15
	LPC_IOCON->P3_16 = iocon_value;		// EMC_D16
	LPC_IOCON->P3_17 = iocon_value;		// EMC_D17
	LPC_IOCON->P3_18 = iocon_value;		// EMC_D18
	LPC_IOCON->P3_19 = iocon_value;		// EMC_D19
	LPC_IOCON->P3_20 = iocon_value;		// EMC_D20
	LPC_IOCON->P3_21 = iocon_value;		// EMC_D21
	LPC_IOCON->P3_22 = iocon_value;		// EMC_D22
	LPC_IOCON->P3_23 = iocon_value;		// EMC_D23
	LPC_IOCON->P3_24 = iocon_value;		// EMC_D24
	LPC_IOCON->P3_25 = iocon_value;		// EMC_D25
	LPC_IOCON->P3_26 = iocon_value;		// EMC_D26
	LPC_IOCON->P3_27 = iocon_value;		// EMC_D27
	LPC_IOCON->P3_28 = iocon_value;		// EMC_D28
	LPC_IOCON->P3_29 = iocon_value;		// EMC_D29
	LPC_IOCON->P3_30 = iocon_value;		// EMC_D30
	LPC_IOCON->P3_31 = iocon_value;		// EMC_D31
	LPC_IOCON->P4_0 = iocon_value;		// EMC_A0
	LPC_IOCON->P4_1 = iocon_value;		// EMC_A1
	LPC_IOCON->P4_2 = iocon_value;		// EMC_A2
	LPC_IOCON->P4_3 = iocon_value;		// EMC_A3
	LPC_IOCON->P4_4 = iocon_value;		// EMC_A4
	LPC_IOCON->P4_5 = iocon_value;		// EMC_A5
	LPC_IOCON->P4_6 = iocon_value;		// EMC_A6
	LPC_IOCON->P4_7 = iocon_value;		// EMC_A7
	LPC_IOCON->P4_8 = iocon_value;		// EMC_A8
	LPC_IOCON->P4_9 = iocon_value;		// EMC_A9
	LPC_IOCON->P4_10 = iocon_value;		// EMC_A10
	LPC_IOCON->P4_11 = iocon_value;		// EMC_A11
	LPC_IOCON->P4_12 = iocon_value;		// EMC_A12
	LPC_IOCON->P4_13 = iocon_value;		// EMC_A13
	LPC_IOCON->P4_14 = iocon_value;		// EMC_A14
#if CONFIG_EMC_STATIC_RAM
	LPC_IOCON->P4_15 = iocon_value;		// EMC_A15
	LPC_IOCON->P4_16 = iocon_value;		// EMC_A16
	LPC_IOCON->P4_17 = iocon_value;		// EMC_A17
	LPC_IOCON->P4_18 = iocon_value;		// EMC_A18
	LPC_IOCON->P4_19 = iocon_value;		// EMC_A19
	LPC_IOCON->P4_20 = iocon_value;		// EMC_A20
	LPC_IOCON->P4_21 = iocon_value;		// EMC_A21
	LPC_IOCON->P4_22 = iocon_value;		// EMC_A22
	LPC_IOCON->P4_23 = iocon_value;		// EMC_A23
	LPC_IOCON->P4_24 = iocon_value;		// EMC_OE
#endif
	LPC_IOCON->P4_25 = iocon_value;		// EMC_WE
#if CONFIG_EMC_STATIC_RAM
	LPC_IOCON->P4_26 = iocon_value;		// EMC_BLS0
	LPC_IOCON->P4_27 = iocon_value;		// EMC_BLS1
	LPC_IOCON->P4_28 = iocon_value;		// EMC_BLS2
	LPC_IOCON->P4_29 = iocon_value;		// EMC_BLS3
	LPC_IOCON->P4_30 = iocon_value;		// EMC_CS0
	LPC_IOCON->P4_31 = iocon_value;		// EMC_CS1
	LPC_IOCON->P5_0 = iocon_value;		// EMC_A24
	LPC_IOCON->P5_1 = iocon_value;		// EMC_A25
#endif


	return uw_err(ERR_NONE);
}


#endif
