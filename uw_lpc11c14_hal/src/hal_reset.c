/*
 * hal_reset_controller.c
 *
 *  Created on: Feb 7, 2015
 *      Author: usenius
 */


#include "LPC11xx.h"
#include "hal_reset_controller.h"
#include "hal_wdt_controller.h"


static uw_reset_sources_e reset_source = UW_RESET_COUNT;

uw_reset_sources_e hal_get_reset_source(void) {
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
}



void hal_system_reset(bool hard_reset) {
	if (hard_reset) {
		//todo: reset with watchdog timer
		hal_init_wdt(1, 40000000);
		hal_reset_wdt();
	}
	else {
		//Cortex M0 reset
		NVIC_SystemReset();
	}
}
