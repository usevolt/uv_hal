/*
 * uv_eeprom.h
 *
 *  Created on: Aug 25, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_EEPROM_H_
#define UV_HAL_INC_UV_EEPROM_H_

#include "uv_hal_config.h"
#include "uv_utilities.h"
#if CONFIG_EEPROM

#ifndef CONFIG_EEPROM_RING_BUFFER_END_ADDR
#error "CONFIG_EEPROM_RING_BUFFER_END_ADDR should define the end EEPROM address of the ring buffer.\
 Ring buffer starts from address 0."
#endif

#if CONFIG_TARGET_LPC11C14
#error "LPC11C14 doesn't have an EEPROM memory. Please undefine CONFIG_EEPROM or set it to 0."
#elif CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"

#define _UV_EEPROM_SIZE			4032
#define _UV_EEPROM_PAGE_SIZE	64

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
uv_errors_e uv_eeprom_write(unsigned char *data, uint16_t len, uint16_t eeprom_addr);


/// @brief: Reads data from the EEPROM memory
///
/// @return ERR_NONE if successful. Error if reading the data failed. Refer to uv_errors for more details.
///
/// @param dest: Pointer to the destination address where the read data is copied
/// @param len: Indicates how many bytes of data should be read
/// @param eeprom_addr: The EEPROM address from which forward the data is read
uv_errors_e uv_eeprom_read(unsigned char *dest, uint16_t len, uint16_t eeprom_addr);






/*
 *
 * EEPROM CIRCULAR BUFFER FUNCTIONS
 *
 */


/// @brief: Sets the entry length in bytes. This needs to be called only if
/// EEPROM ring buffer functions are used
void uv_eeprom_init_circular_buffer(uint16_t entry_len);



/// @brief: Pushes new data to the end of the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was full and couln't push any more
uv_errors_e uv_eeprom_push_back(unsigned char *src);


/// @brief: Deletes the newest entry from the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was empty and couln't remove anything
uv_errors_e uv_eeprom_pop_back(unsigned char *dest);



/// @brief: Deletes the oldest entry from the EEPROM circular buffer
///
/// @note: This takes ownership of the whole EEPROM memory and it shouldn't be used
/// for anything else
///
/// @return Error if the memory was empty and couln't remove anything
uv_errors_e uv_eeprom_pop_front(unsigned char *dest);



/// @brief: Returns the index'th recent data. 0 means the newest.
uv_errors_e uv_eeprom_at(unsigned char *dest, uint16_t index);


/// @brief: Clears the whole EEPROM memory to zeroes
void uv_eeprom_clear();


#endif


#endif
#endif /* UV_HAL_INC_UV_EEPROM_H_ */
