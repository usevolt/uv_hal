/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/



#include "uv_memory.h"


#include <stdio.h>
#include <string.h>
#include "uv_uart.h"
#include "uv_utilities.h"
#include "uv_wdt.h"
#include CONFIG_MAIN_H
#ifdef CONFIG_RTOS
#include "uv_rtos.h"
#endif

const char uv_projname[] = STRINGIFY(__UV_PROJECT_NAME);
const char uv_datetime[] = __DATE__ " " __TIME__;



void uv_get_device_serial(unsigned int dest[4]) {
	memset(dest, 0, sizeof(dest[0]) * 4);
}

uv_errors_e uv_memory_save(void) {
	uv_errors_e ret = ERR_NONE;

	int64_t length = ((unsigned long long) &CONFIG_NON_VOLATILE_END + sizeof(uv_data_end_t)) -
			(unsigned long long) &CONFIG_NON_VOLATILE_START;

	bool match = true;
	// todo: check if old data doesnt match with new data
	if (match) {
		ret = ERR_NONE;
	}
	else {
		printf("Flashing %u bytes\n", (unsigned int) length);
	}
	return ret;
}


uv_errors_e uv_memory_load(void) {
	// on linux memory cannot be saved for now
	uv_errors_e ret = ERR_HARDWARE_NOT_SUPPORTED;

	//todo: load the data

	//todo: check crc

	return ret;
}




uv_errors_e uv_memory_clear(void) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}

void uv_set_id(uint16_t id) {
	CONFIG_NON_VOLATILE_START.id = id;
}

uint8_t uv_get_id() {
	return 0;
}


uint16_t uv_memory_calc_crc(void *data, int32_t len) {
	uint8_t i;
	uint16_t crc = 0;
	uint8_t *d = data;

    while(--len >= 0)
    {
    	i = 8;
    	crc = crc ^ (((uint16_t)*d++) << 8);

    	do
        {
    		if (crc & 0x8000)
    		{
    			crc = crc << 1 ^ 0x1021;
    		}
    		else
    		{
    			crc = crc << 1;
    		}
        }
    	while(--i);
    }
    return crc;
}

/// @brief: Returns the project name saved in the non-volatile memory
const char *uv_memory_get_project_name() {
	return uv_projname;
}

/// @brief: Returns the project name crc value saved in the non-volatile memory
uint16_t uv_memory_get_project_id(uv_data_start_t *start_ptr) {
	return 0;
}

/// @brief: Returns the project building date saved in the non-volatile memory
const char *uv_memory_get_project_date(uv_data_start_t *start_ptr) {
	return uv_datetime;
}


uv_errors_e _uv_memory_hal_load(void) {

	return uv_memory_load();
}


