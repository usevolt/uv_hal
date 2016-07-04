/*
 * hal_reset_controller.c
 *
 *  Created on: Feb 7, 2015
 *      Author: usenius
 */




#include "uv_reset.h"

#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif
#include "uv_wdt.h"


static uv_reset_sources_e reset_source = UW_RESET_COUNT;

uv_reset_sources_e uv_get_reset_source(void) {
#if CONFIG_TARGET_LPC11C14
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
#elif CONFIG_TARGET_LPC1785

#warning "Implementation not defined"

#endif
}



void uv_system_reset(bool hard_reset) {
	if (hard_reset) {
		uv_wdt_init(1);
		uv_wdt_reset();
	}
	else {
		NVIC_SystemReset();
	}
}
