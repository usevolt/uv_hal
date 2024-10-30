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

#ifndef UV_HAL_INC_UV_SPI_H_
#define UV_HAL_INC_UV_SPI_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"

#if ((CONFIG_SPI0 || CONFIG_SPI1 || CONFIG_SPI2) && !defined(CONFIG_SPI))
#define CONFIG_SPI			1
#endif


#if CONFIG_SPI

#if CONFIG_SPI0
#if !CONFIG_SPI0_BAUDRATE
#error "CONFIG_SPI0_BAUDRATE should define the baudrate used for SPI0"
#endif
#if (CONFIG_SPI1_BAUDRATE > 17000000)
#error "CONFIG_SPI1_BAUDRATE Cannot be greater than 17000000"
#endif
#if !defined(CONFIG_SPI0_MOSI_IO)
#error "CONFIG_SPI0_MOSI_IO should define the I/O pin used for SPI0 MOSI"
#endif
#if !defined(CONFIG_SPI0_MISO_IO)
#error "CONFIG_SPI0_MISO_IO should define the I/O pin used for SPI0_MISO"
#endif
#if !defined(CONFIG_SPI0_SCK_IO)
#error "CONFIG_SPI0_SCK_IO should define the I/O pin used for SPI0_CLK"
#endif
#if !CONFIG_SPI0_SLAVE_COUNT
#error "CONFIG_SPI0_SLAVE_COUNT should define the number of slave devices on SPI0"
#endif
#if (CONFIG_SPI0_SLAVE_COUNT > 0)
#if !defined(CONFIG_SPI0_SSEL0_IO)
#error "CONFIG_SPI0_SSEL0_IO should define the I/O pin used for slave 0 select"
#endif
#endif
#if !defined(CONFIG_SPI0_CLOCK_POL)
#error "CONFIG_SPI0_CLOCK_POL should define the clock rest state"
#endif
#if !defined(CONFIG_SPI0_CLOCK_PHASE)
#error "CONFIG_SPI0_CLOCK_PHASE should be defined as 0 if SPI captures data on first clock transition, or\
 as 1 if SPI should change the serial data on first clock transition."
#endif
#endif
#if CONFIG_SPI1
#if !CONFIG_SPI1_BAUDRATE
#error "CONFIG_SPI1_BAUDRATE should define the baudrate used for SPI1"
#endif
#if (CONFIG_SPI1_BAUDRATE > 17000000)
#error "CONFIG_SPI1_BAUDRATE Cannot be greater than 17000000"
#endif
#if !defined(CONFIG_SPI1_MOSI_IO)
#error "CONFIG_SPI1_MOSI_IO should define the I/O pin used for SPI1 MOSI"
#endif
#if !defined(CONFIG_SPI1_MISO_IO)
#error "CONFIG_SPI1_MISO_IO should define the I/O pin used for SPI1_MISO"
#endif
#if !defined(CONFIG_SPI1_SCK_IO)
#error "CONFIG_SPI1_SCK_IO should define the I/O pin used for SPI1_CLK"
#endif
#if !CONFIG_SPI1_SLAVE_COUNT
#error "CONFIG_SPI1_SLAVE_COUNT should define the number of slave devices on SPI1"
#endif
#if (CONFIG_SPI1_SLAVE_COUNT > 0)
#if !defined(CONFIG_SPI1_SSEL0_IO)
#error "CONFIG_SPI1_SSEL0_IO should define the I/O pin used for slave 0 select"
#endif
#endif
#if !defined(CONFIG_SPI1_CLOCK_POL)
#error "CONFIG_SPI1_CLOCK_POL should define the clock rest state"
#endif
#if !defined(CONFIG_SPI1_CLOCK_PHASE)
#error "CONFIG_SPI1_CLOCK_PHASE should be defined as 0 if SPI captures data on first clock transition, or\
 1 if SPI should change the serial data on first clock transition."
#endif
#endif
#if CONFIG_SPI2
#if !CONFIG_SPI2_BAUDRATE
#error "CONFIG_SPI2_BAUDRATE should define the baudrate used for SPI2"
#endif
#if (CONFIG_SPI2_BAUDRATE > 17000000)
#error "CONFIG_SPI2_BAUDRATE Cannot be greater than 17000000"
#endif
#if !defined(CONFIG_SPI2_MOSI_IO)
#error "CONFIG_SPI2_MOSI_IO should define the I/O pin used for SPI2 MOSI"
#endif
#if !defined(CONFIG_SPI2_MISO_IO)
#error "CONFIG_SPI2_MISO_IO should define the I/O pin used for SPI2_MISO"
#endif
#if !defined(CONFIG_SPI2_SCK_IO)
#error "CONFIG_SPI2_SCK_IO should define the I/O pin used for SPI2_CLK"
#endif
#if !CONFIG_SPI2_SLAVE_COUNT
#error "CONFIG_SPI2_SLAVE_COUNT should define the number of slave devices on SPI2"
#endif
#if (CONFIG_SPI2_SLAVE_COUNT > 0)
#if !defined(CONFIG_SPI2_SSEL0_IO)
#error "CONFIG_SPI2_SSEL0_IO should define the I/O pin used for slave 0 select"
#endif
#endif
#if !defined(CONFIG_SPI2_CLOCK_POL)
#error "CONFIG_SPI2_CLOCK_POL should define the clock rest state"
#endif
#if !defined(CONFIG_SPI2_CLOCK_PHASE)
#error "CONFIG_SPI2_CLOCK_PHASE should be defined as 0 if SPI captures data on first clock transition, or\
 1 if SPI should change the serial data on first clock transition."
#endif
#endif


/// @brief: SPI modules
#if CONFIG_TARGET_LPC15XX
#define SPI0	LPC_SPI0
#define SPI1	LPC_SPI1
typedef uint16_t spi_data_t;
typedef LPC_SPI_T* spi_e;
#elif CONFIG_TARGET_LPC40XX
#include "ssp_17xx_40xx.h"
typedef enum {
	SPI0 = 0,
	SPI1,
	SPI2
} spi_e;
typedef uint8_t spi_data_t;
#else
typedef void* spi_e;
#define SPI0	NULL
#define SPI1	NULL
typedef uint8_t spi_data_t;
#endif


/// 2brief: Defines the slave selection values
typedef enum {
	SPI_SLAVE0 = (1 << 0),
	SPI_SLAVE1 = (1 << 1),
	SPI_SLAVE2 = (1 << 2),
	SPI_SLAVE3 = (1 << 3)
} spi_slaves_e;


/// @brief: Initializes the SPI interface(s)
void _uv_spi_init(void);



/// @brief: Writes and reads to the SPI bus. Since SPI is duplex serial protocol,
/// reading and writing are done at the same time.
/// The function returns after the transmission finishes.
///
/// @return: True if writing and reading was successful, otherwise false
///
/// @param spi: The SPI channel used
/// @param slaves: Selected slaves to whom the data is sent
/// @param writebuffer: Pointer to a buffer where write data is read. Note:
/// Buffer is of type spi_data_t which depends on TARGET.
/// @param readbuffer: Pointer to a buffer where read data is written. Note:
/// Buffer is of type spi_data_t which depends on TARGET.
/// @byte_len: The length of individual bytes in bits (usually 8)
/// @buffer_len: The length of the read and write buffers in bytes
/// (not local bytes but *byte_len* bytes)
bool uv_spi_readwrite_sync(const spi_e spi, spi_slaves_e slaves,
		const spi_data_t *writebuffer, spi_data_t *readbuffer,
		const uint8_t byte_len, const uint16_t buffer_len);


/// @brief: Writes to the SPI bus, while ignoring the received data.
/// The function returns after the transmission finishes.
///
/// @return: True if writing and reading was successful, otherwise false
///
/// @param spi: The SPI channel used
/// @param slaves: Selected slaves to whom the data is sent
/// @param writebuffer: Pointer to a buffer where write data is read. Note:
/// Buffer has to be of type uint16_t or int16_t.
/// @byte_len: The length of a individual bytes in bits (usually 8)
/// @buffer_len: The length of the write buffer in bytes
/// (not local bytes but *byte_len* bytes)
bool uv_spi_write_sync(const spi_e spi, spi_slaves_e slaves,
		const spi_data_t *writebuffer, const uint8_t byte_len, const uint16_t buffer_len);


#endif

#endif /* UV_HAL_INC_UV_SPI_H_ */
