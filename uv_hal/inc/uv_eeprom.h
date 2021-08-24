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

#ifndef UV_HAL_INC_UV_EEPROM_H_
#define UV_HAL_INC_UV_EEPROM_H_

#include "uv_hal_config.h"
#include "uv_utilities.h"
#if CONFIG_EEPROM

#define _UV_EEPROM_SIZE			(4032 - 64)


#if CONFIG_EEPROM_RING_BUFFER
#ifndef CONFIG_EEPROM_RING_BUFFER_START_ADDR
#error "CONFIG_EEPROM_RING_BUFFER_START_ADDR should define the start\
 EEPROM address for the ring buffer."
#endif
#ifndef CONFIG_EEPROM_RING_BUFFER_END_ADDR
#error "CONFIG_EEPROM_RING_BUFFER_END_ADDR should define the end\
 EEPROM address of the ring buffer."
#endif
#if (CONFIG_EEPROM_RING_BUFFER_END_ADDR < CONFIG_EEPROM_RING_BUFFER_START_ADDR)
#error "CONFIG_EEPROM_RING_BUFFER_END_ADDR cannot be smaller than\
 CONFIG_EEPROM_RING_BUFFER_START_ADDR"
#endif
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
uv_errors_e uv_eeprom_read(void *dest, uint16_t len, uint16_t eeprom_addr);


#if CONFIG_TARGET_LINUX || CONFIG_TARGET_WIN
/// @brief: Sets the filepath for the eeprom memory location.
/// Defaults to *./$(PROJECT_NAME).nvconf
void uv_eeprom_set_filepath(char *filepath);


/// @brief: Returns the nonvolatile memory file name
char *uv_eeprom_get_filepath(void);


#endif



#if CONFIG_EEPROM_RING_BUFFER


/*
 *
 * EEPROM CIRCULAR BUFFER FUNCTIONS
 *
 */


/// @brief: Sets the entry length in bytes. This needs to be called only if
/// EEPROM ring buffer functions are used
void uv_eeprom_ring_buffer_init(const uint16_t entry_len);



/// @brief: Pushes new data to the end of the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was full and couln't push any more
uv_errors_e uv_eeprom_ring_buffer_push_back(const void *src);

///@brief: Pushes new data to the end of the EEPROM circular buffer. If the buffer
/// is full, calls *uv_eeprom_pop_front* to delete the oldest data and then
/// pushes new data.
uv_errors_e uv_eeprom_ring_buffer_push_back_force(const void *src);


/// @brief: Deletes the newest entry from the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was empty and couln't remove anything
uv_errors_e uv_eeprom_ring_buffer_pop_back(void *dest);



/// @brief: Deletes the oldest entry from the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was empty and couln't remove anything
uv_errors_e uv_eeprom_ring_buffer_pop_front(void *dest);



/// @brief: Returns the index'th recent data. 0 means the newest.
///
/// @param dest: destination where the data is copied
/// @param eeprom_addr: Destination where the actual eeprom address where
/// the **index**'th data was found will be copied
uv_errors_e uv_eeprom_ring_buffer_at(void *dest, uint16_t *eeprom_addr, uint16_t index);


/// @brief: Returns the number of elements in the ring buffer
uint32_t uv_eeprom_ring_buffer_get_count(void);


#endif


/// @brief: Clears the whole EEPROM memory to zeroes
void uv_eeprom_clear(void);

#endif
#endif /* UV_HAL_INC_UV_EEPROM_H_ */
