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
#elif (CONFIG_I2C_BAUDRATE > 1000000)
#error "CONFIG_I2C_BAUDRATE maximum value is 1000000. uv_hal library doesn't support higher baudrates."
#endif
#if CONFIG_I2C_ASYNC
#if !CONFIG_I2C_ASYNC_MAX_BYTE_LEN
#error "CONFIG_I2C_ASYNC_MAX_BYTE_LEN has to define the maximum amount\
 of bytes in I2C asynchronous transmission"
#endif
#if !CONFIG_I2C_ASYNC_BUFFER_LEN
#error "CONFIG_I2C_ASYN_BUFFER_LEN has to define the maximum number of\
 I2C messages in the transmit buffer."
#endif
#endif

typedef enum {
	I2C0 = 0,
	I2C_COUNT
} i2c_e;


/// @brief: Different modes for the I2C
#define I2C_MASTER	1
#define I2C_SLAVE	0


#define I2C_WRITE	0
#define I2C_READ	1




/// @brief: Initializes the I2C module
/// This should be called before any other function
uv_errors_e _uv_i2c_init(void);




/// @brief: Reads reads data to/from i2c device synchronously.
/// The data transmission is started with a START condition
/// and ended with STOP condition. First data in the *tx_buffer* is sent
/// to the device, following a RESTART condition and then data is read
/// to *rx_buffer*.
///
/// @note: First byte of *rx_buffer* always contains slave address and read bit!,
/// otherwise the function fails with NACK.
///
/// @param tx_buffer: Pointer to the buffer which holds the write data. Note
/// that 1st byte is used for device address and R/W bit.
/// R/W bit should be set (1) for this function.
/// @param tx_len: The length of the tx buffer in bytes
/// @param rx_buffer: Pointer to the buffer which holds the read data
/// @param rx_len: The length of the rx buffer in bytes
uv_errors_e uv_i2cm_read(i2c_e channel, uint8_t *tx_buffer, uint16_t tx_len,
		uint8_t *rx_buffer, uint16_t rx_len);



/// @brief: Sends or reads data to/from i2c device synchronously.
/// The data transmission is started with a START condition
/// and ended with STOP condition.
///
/// @param tx_buffer: Pointer to the buffer which holds the write data. Note
/// that 1st byte is used for device address and R/W bit.
/// R/W bit should be cleared (0) for this function.
/// @param tx_len: The length of the tx buffer in bytes
uv_errors_e uv_i2cm_write(i2c_e channel, uint8_t *tx_buffer, uint16_t tx_len);



#if CONFIG_I2C_ASYNC
/// @brief Writes data asynchronously to *dev_addr*. The written data
/// is copied to TX buffer and transmitted once the I2C bus is idle.
///
/// @param tx_buffer: Pointer to the buffer which holds the write data. Note
/// that 1st byte is used for device address and R/W bit.
uv_errors_e uv_i2cm_write_async(i2c_e channel,
		uint8_t *tx_buffer, uint16_t tx_len);
#endif


#endif

#endif /* UV_HAL_INC_UV_I2C_H_ */
