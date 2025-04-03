/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 *
 * MIT License
 *
 * Copyright (c) 2019 usevolt
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>

#if !defined(PRINT)
#define PRINT(...) printf(__VA_ARGS__)
#endif


#if defined(__UV_PROGRAM_VERSION)
#undef __UV_PROGRAM_VERSION
#define __UV_PROGRAM_VERSION 1
#endif
const char uv_projname[] = STRINGIFY(__UV_PROJECT_NAME);
const char uv_datetime[] = __DATE__ " " __TIME__;
const uint32_t uv_prog_version = __UV_PROGRAM_VERSION;
#if defined(CONFIG_SAVE_CALLBACK)
extern void CONFIG_SAVE_CALLBACK (void);
#endif
#if defined(CONFIG_LOAD_CALLBACK)
extern void CONFIG_LOAD_CALLBACK (void);
#endif

char nonvol_filepath[128] = "./" STRINGIFY(__UV_PROJECT_NAME) ".nvconf";



void uv_get_device_serial(unsigned int dest[4]) {
	memset(dest, 0, sizeof(dest[0]) * 4);
}

uv_errors_e uv_memory_save(void) {
	uv_errors_e ret = ERR_NONE;

#if defined(CONFIG_SAVE_CALLBACK)
		CONFIG_SAVE_CALLBACK ();
#endif


	uint32_t len = (unsigned long int) &CONFIG_NON_VOLATILE_END -
			(unsigned long int) &CONFIG_NON_VOLATILE_START - sizeof(uv_data_start_t);
	uint16_t crc = uv_memory_calc_crc(((uint8_t*) &CONFIG_NON_VOLATILE_START) +
			sizeof(uv_data_start_t), len);
	uint16_t hal_crc = uv_memory_calc_crc(&CONFIG_NON_VOLATILE_START, sizeof(uv_data_start_t));


	int32_t length = (((unsigned long int) &CONFIG_NON_VOLATILE_END) + sizeof(uv_data_end_t)) -
			((unsigned long int) &CONFIG_NON_VOLATILE_START);

	PRINT("Flashing %u bytes\n", (int) length);
	if (length < 0) {
		ret = ERR_END_ADDR_LESS_THAN_START_ADDR;
	}
	if (ret == ERR_NONE) {

		// add the right value to data checksum
		CONFIG_NON_VOLATILE_END.hal_crc = hal_crc;
		CONFIG_NON_VOLATILE_END.crc = crc;

		// open the file and write
		FILE *file = fopen(nonvol_filepath, "wb");
		if (file == NULL) {
			PRINT("Creating the non-volatile memory file '%s' failed\n",
					nonvol_filepath);
			ret = ERR_INTERNAL;
		}
		else {
			fwrite(& CONFIG_NON_VOLATILE_START, 1, length, file);
			fclose(file);
		}
	}

	return ret;
}


uv_errors_e uv_memory_load(memory_scope_e scope) {
	uv_errors_e ret = ERR_NOT_INITIALIZED;

	char *scope_str;
	if (scope == MEMORY_ALL_PARAMS) {
		scope_str = "ALL_PARAMS";
	}
	else if (scope == MEMORY_APP_PARAMS) {
		scope_str = "APP_PARAMS";
	}
	else {
		scope_str = "COM_PARAMS";
	}
	PRINT("Loading non-volatile %s data from '%s'\n",
			scope_str, nonvol_filepath);

	// try to open the nonvolatile file
	FILE* file = fopen(nonvol_filepath, "r");
	if (file != NULL) {
		void* d;
		int32_t length;

		if (scope & MEMORY_COM_PARAMS) {
			d = (uint8_t*) & CONFIG_NON_VOLATILE_START;
			// source is the start of the file
		}
		else {
			d = ((uint8_t*) & CONFIG_NON_VOLATILE_START) + sizeof(uv_data_start_t);
			// source is the start of application data
			fseek(file, sizeof(uv_data_start_t), SEEK_SET);
		}

		if (scope & MEMORY_APP_PARAMS) {
			length = ((unsigned long int) & CONFIG_NON_VOLATILE_END + sizeof(uv_data_end_t)) -
					(unsigned long int) d;
		}
		else {
			length = sizeof(uv_data_start_t);
		}

		// copy the data
		int size = fread(d, 1, length, file);

		if (size >= length) {
			// copy the end structure if it was not copied yet
			if (!(scope & MEMORY_APP_PARAMS)) {
				fseek(file,
						(unsigned long int) & CONFIG_NON_VOLATILE_END -
						(unsigned long int) & CONFIG_NON_VOLATILE_START, SEEK_SET);
				size = fread(& CONFIG_NON_VOLATILE_END, 1, sizeof(uv_data_end_t), file);
			}
			ret = ERR_NONE;
		}
	}
	else {
		PRINT("Failed to open the non-volatile data file.\n");
	}


	if (ret == ERR_NONE) {
		//check crc
		if (scope & MEMORY_APP_PARAMS) {
			uint32_t len = (unsigned long int) & CONFIG_NON_VOLATILE_END -
					(unsigned long int) & CONFIG_NON_VOLATILE_START - sizeof(uv_data_start_t);
			uint16_t crc = uv_memory_calc_crc(((uint8_t*) & CONFIG_NON_VOLATILE_START) +
					sizeof(uv_data_start_t), len);

			if (CONFIG_NON_VOLATILE_END.crc != crc) {
				ret = ERR_END_CHECKSUM_NOT_MATCH;
			}
		}
		if (scope & MEMORY_COM_PARAMS) {
			// calculate the HAL checksum and compare it to the loaded value
			uint16_t crc = uv_memory_calc_crc(& CONFIG_NON_VOLATILE_START, sizeof(uv_data_start_t));
			if (crc != CONFIG_NON_VOLATILE_END.hal_crc) {
				PRINT("calculated crc 0x%x don't match with 0x%x\n",
						crc,
						CONFIG_NON_VOLATILE_END.hal_crc);
				// hal crc didn't match, which means that we have loaded invalid settings.
				// Revert the HAL system defaults
				_uv_rtos_hal_reset();
				ret = ERR_START_CHECKSUM_NOT_MATCH;
			}
		}
	}

	if (ret == ERR_NONE) {
#if defined(CONFIG_LOAD_CALLBACK)
		CONFIG_LOAD_CALLBACK ();
#endif
	}

	return ret;
}


void uv_memory_set_can_baudrate(uint32_t baudrate) {
	uv_can_set_baudrate(CONFIG_CANOPEN_CHANNEL, baudrate);
}




uv_errors_e uv_memory_clear(memory_scope_e scope) {
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



void uv_memory_set_nonvol_filepath(char *filepath) {
	strcpy(nonvol_filepath, filepath);
}


char *uv_memory_get_nonvol_filepath(void) {
	return nonvol_filepath;
}


