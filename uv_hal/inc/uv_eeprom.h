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

#ifndef UV_HAL_INC_UV_EEPROM_H_
#define UV_HAL_INC_UV_EEPROM_H_

#include "uv_hal_config.h"
#include "uv_utilities.h"
#if CONFIG_EEPROM

#if CONFIG_EEPROM_RING_BUFFER
#ifndef CONFIG_EEPROM_RING_BUFFER_END_ADDR
#error "CONFIG_EEPROM_RING_BUFFER_END_ADDR should define the end EEPROM address of the ring buffer.\
 Ring buffer starts from address 0."
#endif
#endif

#if CONFIG_TARGET_LPC11C14
#error "LPC11C14 doesn't have an EEPROM memory. Please undefine CONFIG_EEPROM or set it to 0."
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#define _UV_EEPROM_SIZE			4032
#define _UV_EEPROM_PAGE_SIZE	64
#elif CONFIG_TARGET_LPC1549
#define _UV_EEPROM_SIZE			(4032 - 64)
#endif

/// @brief: Initializes the EEPROM memory
///
/// @param entry_len: Length of the circular buffer entry in bytes.
/// If EEPROM circular buffer is not used, this has no meaning
uv_errors_e _uv_eeprom_init();


/// @brief: Returns the size of the EEPROM data in bytes
static inline uint16_t uv_eeprom_size(void) {
	return _UV_EEPROM_SIZE;
}
/// @brief: Writes data to the EEPROM memory.
///
/// @note: The function returns when the last data was saved to the memory
///
/// @return ERR_NONE if successful. Error if writing the data failed. Refer to uv_errors for more details.
///
/// @param data: Pointer to the data to be saved
/// @param len: The length of the data in bytes
/// @param eeprom_addr: The address of the EEPROM where the data is to be saved
uv_errors_e uv_eeprom_write(const void *data, uint16_t len, uint16_t eeprom_addr);


/// @brief: Reads data from the EEPROM memory
///
/// @return ERR_NONE if successful. Error if reading the data failed. Refer to uv_errors for more details.
///
/// @param dest: Pointer to the destination address where the read data is copied
/// @param len: Indicates how many bytes of data should be read
/// @param eeprom_addr: The EEPROM address from which forward the data is read
uv_errors_e uv_eeprom_read(unsigned char *dest, uint16_t len, uint16_t eeprom_addr);





#if CONFIG_EEPROM_RING_BUFFER


/*
 *
 * EEPROM CIRCULAR BUFFER FUNCTIONS
 *
 */


/// @brief: Sets the entry length in bytes. This needs to be called only if
/// EEPROM ring buffer functions are used
void uv_eeprom_init_circular_buffer(const uint16_t entry_len);



/// @brief: Pushes new data to the end of the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was full and couln't push any more
uv_errors_e uv_eeprom_push_back(const void *src);


/// @brief: Deletes the newest entry from the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was empty and couln't remove anything
uv_errors_e uv_eeprom_pop_back(void *dest);



/// @brief: Deletes the oldest entry from the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was empty and couln't remove anything
uv_errors_e uv_eeprom_pop_front(void *dest);



/// @brief: Returns the index'th recent data. 0 means the newest.
///
/// @param dest: destination where the data is copied
/// @param eeprom_addr: Destination where the actual eeprom address where
/// the **index**'th data was found will be copied
uv_errors_e uv_eeprom_at(void *dest, uint16_t *eeprom_addr, uint16_t index);


/// @brief: Clears the whole EEPROM memory to zeroes
void uv_eeprom_clear(void);


#endif


#endif
#endif /* UV_HAL_INC_UV_EEPROM_H_ */
