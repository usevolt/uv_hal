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

uw_errors_e uw_emc_init();

/// @brief: Defines the hardware dependent start address of the external memory.
#define EMC_MEM_ADDR		0xA0000000U



#endif


#endif /* INC_UW_EMC_H_ */
