/*
 * uv_emc.h
 *
 *  Created on: Mar 29, 2016
 *      Author: usevolt
 */

#ifndef INC_UW_EMC_H_
#define INC_UW_EMC_H_


/// @file: External  memory controller HAL interface.
/// NOTE: The EMC is not available on LPC11Cxx controllers.

#include "uv_hal_config.h"
#include "uv_errors.h"

#if (CONFIG_TARGET_LPC1785)

#if !CONFIG_EMC_DYNAMIC_RAM && !CONFIG_EMC_STATIC_RAM
#error "Either CONFIG_EMC_DYNAMIC_RAM or CONFIG_EMC_STATIC_RAM needs to be defined as 1."
#endif
#if !CONFIG_EMC_CHIP_COUNT || CONFIG_EMC_CHIP_COUNT > 4
#error "CONFIG_EMC_CHIP_COUNT either not defined or set to invalid value. It should be between 1...4."
#endif
#if CONFIG_EMC_STATIC_RAM
#error "EMC static RAM interface not implemented!"
#endif
#if !defined(CONFIG_EMC_CLOCK_DIV_HALF)
#error "CONFIG_EMC_CLOCK_DIV_HALF should be defined as 1 or 0, depending if the EMC clock should be\
 the same as CPU clock or half of it."
#endif


uv_errors_e uv_emc_init( void );




#endif


#endif /* INC_UW_EMC_H_ */
