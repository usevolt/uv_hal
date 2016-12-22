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


void uv_system_reset(bool hard_reset) {
	if (hard_reset) {
#if CONFIG_WDT
		_uv_wdt_init();
		uv_wdt_reset();
#endif
	}
	else {
		NVIC_SystemReset();
	}
}



uv_reset_sources_e uv_reset_get_source() {
#if CONFIG_TARGET_LPC1785
	uv_reset_sources_e e = LPC_SC->RSID;
	return e;
#elif CONFIG_TARGET_LPC11C14
	// note: this hasn't been confirmed, might cause a compile error
//	uv_reset_sources_e e = LPC_SYSCON->RSTSTAT;
	uv_reset_sources_e e = UV_RESET_POR;
	return e;
#endif
}
