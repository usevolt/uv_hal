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

/// @file: Interface for external RAM memory.
/// NOTE: Currently only 16-bit SDRAM is supported!
///
/// NOTE: The EMC module might cause unexpected hard faults depending on what optimizations are enabled.
/// The hard faults are usually related to executing terminal commands: The command array pointer is somehow
/// corrupted, it doesnt point to the start of the command array.

#if (CONFIG_TARGET_LPC1785)

#if !CONFIG_EMC_DYNAMIC_RAM && !CONFIG_EMC_STATIC_RAM
#error "Either CONFIG_EMC_DYNAMIC_RAM or CONFIG_EMC_STATIC_RAM needs to be defined as 1."
#endif
#if CONFIG_EMC_STATIC_RAM
#error "EMC static RAM interface not implemented!"
#endif
#if !defined(CONFIG_EMC_CLOCK_DIV_HALF)
#error "CONFIG_EMC_CLOCK_DIV_HALF should be defined as 1 or 0, depending if the EMC clock should be\
 the same as CPU clock or half of it."
#endif
#if CONFIG_EMC_DYNAMIC_RAM
#if !defined(CONFIG_EMC_SDRAM_MODE_REGISTER)
#error "CONFIG_EMC_SDRAM_MODE_REGISTER should define the mode data which is used to configure SDRAM device\
 to the desired settings. Refer to the UM10470 manual, page 177 for correct value."
#endif
#if !defined(CONFIG_EMC_SDRAM_AM0)
#error "CONFIG_EMC_SDRAM_AM0 should define the configuration settings to SDRAM device. Refer to UM10470 manual,\
 page 194 for correct value."
#endif
#if !defined(CONFIG_EMC_SDRAM_AM1)
#error "CONFIG_EMC_SDRAM_AM1 should define the configuration settings to SDRAM device. Refer to UM10470 manual,\
 page 194 for correct value."
#endif
#if !defined(CONFIG_EMC_SDRAM_RAS)
#error "CONFIG_EMC_SDRAM_RAS should define the row access latency in clock cycles."
#endif
#if !defined(CONFIG_EMC_SDRAM_CAS)
#error "CONFIG_EMC_SDRAM_CAS should define the column access latency in clock cycles."
#endif
#if !defined(CONFIG_EMC_SDRAM_REFRESH)
#error "CONFIG_EMC_SDRAM_REFRESH defines the auto refresh rate for SDRAM. Refer to UM10470 manual, \
page 186 for more info."
#endif
#if !defined(CONFIG_EMC_SDRAM_1)
#error "CONFIG_EMC_SDRAM_1 should be defined as 1 or 0, depending if SDRAM chip 1 (0xA000 0000) should be enabled"
#endif
#if !defined(CONFIG_EMC_SDRAM_2)
#error "CONFIG_EMC_SDRAM_2 should be defined as 1 or 0, depending if SDRAM chip 2 (0xB000 0000) should be enabled"
#endif
#if !defined(CONFIG_EMC_SDRAM_3)
#error "CONFIG_EMC_SDRAM_3 should be defined as 1 or 0, depending if SDRAM chip 3 (0xC000 0000) should be enabled"
#endif
#if !defined(CONFIG_EMC_SDRAM_4)
#error "CONFIG_EMC_SDRAM_4 should be defined as 1 or 0, depending if SDRAM chip 4 (0xD000 0000) should be enabled"
#endif

#endif


typedef enum {
#if CONFIG_EMC_DYNAMIC_RAM
	EMC_SDRAM_1 = 0xA0000000,
	EMC_SDRAM_2 = 0xB0000000,
	EMC_SDRAM_3 = 0xC0000000,
	EMC_SDRAM_4 = 0xD0000000
#endif
#if CONFIG_EMC_STATIC_RAM
	EMC_SRAM_1 = 0x80000000,
	EMC_SRAM_2 = 0x90000000,
	EMC_SRAM_3 = 0x98000000,
	EMC_SRAM_4 = 0x9c000000
#endif
} uv_emc_devices_e;



uv_errors_e uv_emc_init( void );




#endif


#endif /* INC_UW_EMC_H_ */
