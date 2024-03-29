/*
 * @brief LPC15xx specific stopwatch implementation
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
#include "stopwatch.h"

/*****************************************************************************
 * Private types/enumerations/variables
 ****************************************************************************/

/* Precompute these to optimize runtime */
static uint32_t ticksPerSecond;
static uint32_t ticksPerMs;
static uint32_t ticksPerUs;

/*****************************************************************************
 * Public types/enumerations/variables
 ****************************************************************************/

/*****************************************************************************
 * Private functions
 ****************************************************************************/

/*****************************************************************************
 * Public functions
 ****************************************************************************/

/* Initialize stopwatch */
void StopWatch_Init(void)
{
	/* Use Repetitive Interrupt Timer (RIT) */
	Chip_RIT_Init(LPC_RITIMER);
	Chip_RIT_SetCompareValue(LPC_RITIMER, (uint32_t) 0xFFFFFFFF);
	Chip_RIT_EnableCompClear(LPC_RITIMER);
	Chip_RIT_Enable(LPC_RITIMER);

	/* Pre-compute tick rate. */
	ticksPerSecond = Chip_Clock_GetSystemClockRate();
	ticksPerMs = ticksPerSecond / 1000;
	ticksPerUs = ticksPerSecond / 1000000;
}

/* Start a stopwatch */
uint32_t StopWatch_Start(void)
{
	/* Return the current timer count. */
	return (uint32_t) Chip_RIT_GetCounter(LPC_RITIMER);
}

/* Returns number of ticks per second of the stopwatch timer */
uint32_t StopWatch_TicksPerSecond(void)
{
	return ticksPerSecond;
}

/* Converts from stopwatch ticks to mS. */
uint32_t StopWatch_TicksToMs(uint32_t ticks)
{
	return ticks / ticksPerMs;
}

/* Converts from stopwatch ticks to uS. */
uint32_t StopWatch_TicksToUs(uint32_t ticks)
{
	return ticks / ticksPerUs;
}

/* Converts from mS to stopwatch ticks. */
uint32_t StopWatch_MsToTicks(uint32_t mS)
{
	return mS * ticksPerMs;
}

/* Converts from uS to stopwatch ticks. */
uint32_t StopWatch_UsToTicks(uint32_t uS)
{
	return uS * ticksPerUs;
}

#endif
