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

#ifndef UV_HAL_INC_UV_I2C_H_
#define UV_HAL_INC_UV_I2C_H_
#include "uv_hal_config.h"
#include "uv_utilities.h"

#if CONFIG_I2C

#if !defined(CONFIG_I2C_TX_BUFFER_LEN)
#error "CONFIG_I2C_TX_BUFFER_LEN not defined. It should define the maximum length of the I2C transmit buffer in bytes."
#endif
#if !defined(CONFIG_I2C_MODE)
#error "CONFIG_I2C_MODE should be defined as I2C_MASTER or I2C_SLAVE."
#endif
#if !defined(CONFIG_I2C_BAUDRATE)
#error "CONFIG_I2C_BAUDRATE should be defined as the desired baudrate for the I2C module."
#endif
#if !defined(CONFIG_I2C_RETRY_COUNT)
#error "CONFIG_I2C_RETRY_COUNT should define the maximum number of retrys if the data sending fails."
#endif

typedef enum {
#if CONFIG_TARGET_LPC11C14
	I2C_1 = 0,
#elif CONFIG_TARGET_LPC1785
	I2C_1 = 0,
#endif
	i2C_COUNT
} i2c_e;


/// @brief: Different modes for the I2C
#define I2C_MASTER	1
#define I2C_SLAVE	0




/// @brief: Initializes the I2C module
/// This should be called before any other function
uv_errors_e _uv_i2c_init(i2c_e i2c);



/// @brief: Sends data to i2c device.
///
/// @param id: The ID of the destination slave device
/// @param data_length: The number of bytes being written
/// @param data: A pointer to the data array which will be written
uv_errors_e uv_i2c_write(i2c_e i2c, uint8_t id, uint16_t data_length, uint8_t *data);


/// @brief: Read bytes from a I2C device
///
/// @param i2c: The I2C module which the data is read
/// @param id: The ID of the slave device
/// @param data_length: Indicates how many bytes are to be read
/// @param dest: The data destination pointer. *data_length* bytes should be available
uv_errors_e uv_i2c_read(i2c_e i2c, uint16_t id, uint16_t data_length, uint8_t *dest);


/// @brief: Step function which should be called every step cycle
uv_errors_e uv_i2c_step(i2c_e i2c, uint16_t step_ms);


/// @brief: Returns ERR_NONE if the I2C module is ready for next transmission
uv_errors_e uv_i2c_busy(i2c_e i2c);


#endif

#endif /* UV_HAL_INC_UV_I2C_H_ */
