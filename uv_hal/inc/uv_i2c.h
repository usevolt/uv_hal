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

#if !defined(CONFIG_I2C_MODE)
#error "CONFIG_I2C_MODE should be defined as I2C_MASTER or I2C_SLAVE."
#endif
#if !defined(CONFIG_I2C_BAUDRATE)
#error "CONFIG_I2C_BAUDRATE should be defined as the desired baudrate for the I2C module."
#elif (CONFIG_I2C_BAUDRATE > 100000)
#error "CONFIG_I2C_BAUDRATE maximum value is 100000. uv_hal library doesn't support higher baudrates."
#endif

typedef enum {
	I2C0 = 0,
	i2C_COUNT
} i2c_e;


/// @brief: Different modes for the I2C
#define I2C_MASTER	1
#define I2C_SLAVE	0




/// @brief: Initializes the I2C module
/// This should be called before any other function
uv_errors_e _uv_i2c_init(void);




/// @brief: Sends or reads data to/from i2c device synchronously.
/// The data transmission is started with a START condition
/// and ended with STOP condition.
///
/// @param dev_addr: The 7-bit addres of the device which should be read or written
/// @param tx_buffer: Pointer to the buffer which holds the write data
/// @param tx_len: The length of the tx buffer in bytes
/// @param rx_buffer: Pointer to the buffer which holds the read data
/// @param rx_len: The length of the rx buffer in bytes
uv_errors_e uv_i2cm_readwrite(i2c_e i2c, uint8_t dev_addr, uint8_t *tx_buffer, uint16_t tx_len,
		uint8_t *rx_buffer, uint16_t rx_len);





#endif

#endif /* UV_HAL_INC_UV_I2C_H_ */
