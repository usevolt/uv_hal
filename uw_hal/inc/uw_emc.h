/*
 * uw_emc.h
 *
 *  Created on: Mar 29, 2016
 *      Author: usevolt
 */

#ifndef INC_UW_EMC_H_
#define INC_UW_EMC_H_


/// @file: External  memory controller HAL interface.
/// NOTE: The EMC is not available on LPC11Cxx controllers.

#include "uw_hal_config.h"
#include "uw_errors.h"

#if (CONFIG_TARGET_LPC178X)

uw_errors_e uw_emc_init( void );

/// @brief: Defines the hardware dependent start address of the external memory.
#define EMC_MEM_ADDR		0xA0000000U


/// @brief: IOCON EMC Values for LPC1785
#define EMC_CS2		(0b001)
#define EMC_CS3		(0b001)
#define EMC_CAS		(0b001)
#define EMC_RAS		(0b001)
#define EMC_CLK0	(0b001)
#define EMC_CLK1	(0b001)
#define EMC_DYCS0	(0b001)
#define EMC_DYCS1	(0b001)
#define EMC_DYCS2	(0b001)
#define EMC_DYCS3	(0b001)
#define EMC_CKE0	(0b001)
#define EMC_CKE1	(0b001)
#define EMC_CKE2	(0b001)
#define EMC_CKE3	(0b001)
#define EMC_DQM0	(0b001)
#define EMC_DQM1	(0b001)
#define EMC_DQM2	(0b001)
#define EMC_DQM3	(0b001)
#define EMC_D0		(0b001)
#define EMC_D1		(0b001)
#define EMC_D2		(0b001)
#define EMC_D3		(0b001)
#define EMC_D4		(0b001)
#define EMC_D5		(0b001)
#define EMC_D6		(0b001)
#define EMC_D7		(0b001)
#define EMC_D8		(0b001)
#define EMC_D9		(0b001)
#define EMC_D10		(0b001)
#define EMC_D11		(0b001)
#define EMC_D12		(0b001)
#define EMC_D13		(0b001)
#define EMC_D14		(0b001)
#define EMC_D15		(0b001)
#define EMC_D16		(0b001)
#define EMC_D17		(0b001)
#define EMC_D18		(0b001)
#define EMC_D19		(0b001)
#define EMC_D20		(0b001)
#define EMC_D21		(0b001)
#define EMC_D22		(0b001)
#define EMC_D23		(0b001)
#define EMC_D24		(0b001)
#define EMC_D25		(0b001)
#define EMC_D26		(0b001)
#define EMC_D27		(0b001)
#define EMC_D28		(0b001)
#define EMC_D29		(0b001)
#define EMC_D30		(0b001)
#define EMC_D31		(0b001)
#define EMC_A0		(0b001)
#define EMC_A1		(0b001)
#define EMC_A2		(0b001)
#define EMC_A3		(0b001)
#define EMC_A4		(0b001)
#define EMC_A5		(0b001)
#define EMC_A6		(0b001)
#define EMC_A7		(0b001)
#define EMC_A8		(0b001)
#define EMC_A9		(0b001)
#define EMC_A10		(0b001)
#define EMC_A11		(0b001)
#define EMC_A12		(0b001)
#define EMC_A13		(0b001)
#define EMC_A14		(0b001)
#define EMC_A15		(0b001)
#define EMC_A16		(0b001)
#define EMC_A17		(0b001)
#define EMC_A18		(0b001)
#define EMC_A19		(0b001)
#define EMC_A20		(0b001)
#define EMC_A21		(0b001)
#define EMC_A22		(0b001)
#define EMC_A23		(0b001)
#define EMC_A24		(0b001)
#define EMC_A25		(0b001)
#define EMC_OE		(0b001)
#define EMC_WE		(0b001)
#define EMC_BLS0	(0b001)
#define EMC_BLS1	(0b001)
#define EMC_BLS2	(0b001)
#define EMC_BLS3	(0b001)
#define EMC_CS0		(0b001)
#define EMC_CS1		(0b001)

#endif


#endif /* INC_UW_EMC_H_ */
