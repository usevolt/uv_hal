/*
 * hal_reset_controller.c
 *
 *  Created on: Feb 7, 2015
 *      Author: usenius
 */

#ifdef LPC11C14
#include "LPC11xx.h"
#elif defined(LPC1785)
#include "LPC177x_8x.h"
#endif
#include "uw_reset.h"
#include "uw_wdt.h"


static uw_reset_sources_e reset_source = UW_RESET_COUNT;

uw_reset_sources_e hal_get_reset_source(void) {
#ifdef LPC11C14
	//POR reset
	//external reset pin
	if ((LPC_SYSCON->SYSRSTSTAT & (1 << 1))) {
		reset_source = UW_RESET_EXTERNAL_PIN;
	}
	else if (LPC_SYSCON->SYSRSTSTAT & (1 << 2)) {
		reset_source = UW_RESET_WATCHDOG;
	}
	else if (LPC_SYSCON->SYSRSTSTAT & (1 << 3)) {
		reset_source = UW_RESET_BROWN_OUT;
	}
	else if (LPC_SYSCON->SYSRSTSTAT & (1 << 4)) {
		reset_source = UW_RESET_SOFTWARE;
	}
	else if (LPC_SYSCON->SYSRSTSTAT & (1 << 0)) {
		reset_source = UW_RESET_POR;
	}
	//clear all reset data
	LPC_SYSCON->SYSRSTSTAT = 0xF;
	return reset_source;
#elif defined(LPC1785)

#warning "Implementation not defined"

#endif
}



void hal_system_reset(bool hard_reset) {
	if (hard_reset) {
		//todo: reset with watchdog timer
		uw_wdt_init(1, 40000000);
		uw_wdt_reset();
	}
	else {
		//Cortex M0 reset
		NVIC_SystemReset();
	}
}
