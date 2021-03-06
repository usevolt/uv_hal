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
