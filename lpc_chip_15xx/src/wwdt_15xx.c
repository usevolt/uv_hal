/*
 * @brief LPC15xx WWDT chip driver
 *
 * @note
 * Copyright(C) NXP Semiconductors, 2013
 * All rights reserved.
 *
 * @par
 * Software that is described herein is for illustrative purposes only
 * which provides customers with programming information regarding the
 * LPC products.  This software is supplied "AS IS" without any warranties of
 * any kind, and NXP Semiconductors and its licensor disclaim any and
 * all warranties, express or implied, including all implied warranties of
 * merchantability, fitness for a particular purpose and non-infringement of
 * intellectual property rights.  NXP Semiconductors assumes no responsibility
 * or liability for the use of the software, conveys no license or rights under any
 * patent, copyright, mask work right, or any other intellectual property rights in
 * or to any products. NXP Semiconductors reserves the right to make changes
 * in the software without notification. NXP Semiconductors also makes no
 * representation or warranty that such application will be suitable for the
 * specified use without further testing or modification.
 *
 * @par
 * Permission to use, copy, modify, and distribute this software and its
 * documentation is hereby granted, under NXP Semiconductors' and its
 * licensor's relevant copyrights in the software, without fee, provided that it
 * is used in conjunction with NXP Semiconductors microcontrollers.  This
 * copyright, permission, and disclaimer notice must appear in all copies of
 * this code.
 */
#include <uv_hal_config.h>

#if CONFIG_TARGET_LPC15XX


#include "chip.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Initialize the Watchdog timer */
void Chip_WWDT_Init(LPC_WWDT_T *pWWDT)
{
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_WDT);

	/* Disable watchdog */
	pWWDT->MOD       = 0;
	pWWDT->TC        = 0xFF;
	pWWDT->WARNINT   = 0x3FF;
	pWWDT->WINDOW    = 0xFFFFFF;
}

/* Clear WWDT interrupt status flags */
void Chip_WWDT_ClearStatusFlag(LPC_WWDT_T *pWWDT, uint32_t status)
{
	if (status & WWDT_WDMOD_WDTOF) {
		pWWDT->MOD &= (~WWDT_WDMOD_WDTOF) & WWDT_WDMOD_BITMASK;
	}

	if (status & WWDT_WDMOD_WDINT) {
		pWWDT->MOD |= WWDT_WDMOD_WDINT;
	}
}

#endif
