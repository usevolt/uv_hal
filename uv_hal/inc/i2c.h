/*
 * i2c.h
 *
 *  Created on: Jul 22, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_I2C_H_
#define UV_HAL_INC_I2C_H_
#include "uv_hal_config.h"
#include "uv_utilities.h"

#if I2C
#if !defined(CONFIG_I2C_TX_BUFFER_LEN)
#error "CONFIG_I2C_TX_BUFFER_LEN not defined. It should define the maximum length of the I2C transmit buffer in bytes."
#endif
#if !defined(CONFIG_I2C_RX_BUFFER_LEN)
#error "CONFIG_I2C_RX_BUFFER_LEN not defined. It should define the maximum length of the I2C receive buffer in bytes."
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


/// @brief: Describes the special bytes for the I2C.
/// The values start from 256 forward, because only 8 bit data is sent
/// through the I2C
typedef enum {
	I2C_START = 0x100,
	i2C_STOP = 0x101
} i2c_bytes_e;


/// @brief: Struct for I2C communication
typedef struct {
	/// @brief: base address from which forward the data resides
	uint16_t base_addr;
	/// @brief: The length of the data in bytes
	uint16_t data_len;
	/// @brief: ID of the device
	uint8_t id;
	/// @brief: The actual data
	uint8_t *data;
} i2c_com_st;


/// @brief: Initializes the I2C module
/// This should be called before any other function
uv_errors_e i2c_init(i2c_e i2c);



/// @brief: Sends a character to i2c module.
///
/// @param data: The data to be sent. Node that I2C operates on 8 bit data, thus only 1 byte
/// can be sent at once. Only bigger values than 255 which are allowed is the I2C_START and I2C_STOP bits.
uv_errors_e i2c_send(i2c_e i2c, uint16_t data);


/// @brief: Reads the oldest received byte from the I2C module
///
/// @return: Error if the buffer was empty
uv_errors_e i2c_read(i2c_e i2c, uint8_t *data);


/// @brief: Returns true if I2C module is ready to send or receive data
bool i2c_ready(i2c_e i2c);




#endif

#endif /* UV_HAL_INC_I2C_H_ */
