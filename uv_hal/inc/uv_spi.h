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

#ifndef UV_HAL_INC_UV_SPI_H_
#define UV_HAL_INC_UV_SPI_H_


#include <uv_hal_config.h>
#include "uv_utilities.h"

#if ((CONFIG_SPI0 || CONFIG_SPI1 || CONFIG_SPI2) && !defined(CONFIG_SPI))
#define CONFIG_SPI			1
#endif


#if CONFIG_SPI

#if CONFIG_TARGET_LPC1785

#if CONFIG_SPI0
#if !CONFIG_SPI0_RXBUF_LEN
#error "CONFIG_SPI0_RXBUF_LEN should define the length in bytes of the receive buffer"
#endif
#if !CONFIG_SPI0_BAUDRATE
#error "CONFIG_SPI0_BAUDRATE should define the desired baudrate in Hz"
#endif
#endif

#if CONFIG_SPI1
#if !CONFIG_SPI1_RXBUF_LEN
#error "CONFIG_SPI1_RXBUF_LEN should define the length in bytes of the receive buffer"
#endif
#if !CONFIG_SPI1_BAUDRATE
#error "CONFIG_SPI1_BAUDRATE should define the desired baudrate in Hz"
#endif
#endif

#if CONFIG_SPI2
#if !CONFIG_SPI2_RXBUF_LEN
#error "CONFIG_SPI2_RXBUF_LEN should define the length in bytes of the receive buffer"
#endif
#if !CONFIG_SPI2_BAUDRATE
#error "CONFIG_SPI2_BAUDRATE should define the desired baudrate in Hz"
#endif
#endif

typedef enum {
	SPI0,
	SPI1,
	SPI2
} spi_e;

#elif CONFIG_TARGET_LPC1549

#if !CONFIG_SPI0_BAUDRATE
#error "CONFIG_SPI0_BAUDRATE should define the baudrate used for SPI0"
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
#if (CONFIG_SPI0_SLAVE_COUNT > 4)
#error "Maximum slave count is 4 for SPI0"
#elif (CONFIG_SPI0_SLAVE_COUNT > 3)
#if !defined(CONFIG_SPI0_SSEL3_IO)
#error "CONFIG_SPI0_SSEL3_IO should define the I/O pin used for slave 3 select"
#endif
#if !defined(CONFIG_SPI0_SSEL3_INV)
#error "CONFIG_SPI0_SSEL3_INV should be defined as 1 or 0, depending if the SSEL3\
 logic polarity should be inverted. (when 0, SSEL3 is active low.)"
#endif
#endif
#if (CONFIG_SPI0_SLAVE_COUNT > 2)
#if !defined(CONFIG_SPI0_SSEL2_IO)
#error "CONFIG_SPI0_SSEL2_IO should define the I/O pin used for slave 2 select"
#endif
#if !defined(CONFIG_SPI0_SSEL2_INV)
#error "CONFIG_SPI0_SSEL2_INV should be defined as 1 or 0, depending if the SSEL2\
 logic polarity should be inverted. (when 0, SSEL2 is active low.)"
#endif
#endif
#if (CONFIG_SPI0_SLAVE_COUNT > 1)
#if !defined(CONFIG_SPI0_SSEL1_IO)
#error "CONFIG_SPI0_SSEL1_IO should define the I/O pin used for slave 1 select"
#endif
#if !defined(CONFIG_SPI0_SSEL1_INV)
#error "CONFIG_SPI0_SSEL1_INV should be defined as 1 or 0, depending if the SSEL1\
 logic polarity should be inverted. (when 0, SSEL1 is active low.)"
#endif
#endif
#if (CONFIG_SPI0_SLAVE_COUNT > 0)
#if !defined(CONFIG_SPI0_SSEL0_IO)
#error "CONFIG_SPI0_SSEL0_IO should define the I/O pin used for slave 0 select"
#endif
#if !defined(CONFIG_SPI0_SSEL0_INV)
#error "CONFIG_SPI0_SSEL0_INV should be defined as 1 or 0, depending if the SSEL0\
 logic polarity should be inverted. (when 0, SSEL0 is active low.)"
#endif
#endif
#if !defined(CONFIG_SPI0_MSB_FIRST)
#error "CONFIG_SPI0_MSB_FIRST should be set to 1 or 0 depending if the most\
 significant bit should be transmitted first"
#endif
#if !defined(CONFIG_SPI0_PREDELAY)
#error "CONFIG_SPI0_PREDELAY should define the delay between SSEL and first bit"
#endif
#if !defined(CONFIG_SPI0_POSTDELAY)
#error "CONFIG_SPI0_POSTDELAY should define the delay between last bit and SSEL"
#endif
#if !defined(CONFIG_SPI0_FRAMEDELAY)
#error "CONFIG_SPI0_FRAMEDELAY should define the delay between frames"
#endif
#if !defined(CONFIG_SPI0_TRANSFERDELAY)
#error "CONFIG_SPI0_TRANSFERDELAY should define the minimum amount of time that SSEL's are\
 deasserted between frames."
#endif
#if !defined(CONFIG_SPI0_CLOCK_POL)
#error "CONFIG_SPI0_CLOCK_POL should define the clock rest state"
#endif
#if !defined(CONFIG_SPI0_CLOCK_PHASE)
#error "CONFIG_SPI0_CLOCK_PHASE should be defined as 0 if SPI captures data on first clock transition, or\
 as 1 if SPI should change the serial data on first clock transition."
#endif

/// @brief: SPI modules
#define SPI0	LPC_SPI0
#define SPI1	LPC_SPI1
typedef LPC_SPI_T* spi_e;


/// 2brief: Defines the slave selection values
typedef enum {
	SPI_SLAVE0 = (1 << 0),
	SPI_SLAVE1 = (1 << 1),
	SPI_SLAVE2 = (1 << 2),
	SPI_SLAVE3 = (1 << 3)
} spi_slaves_e;

#endif


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
/// Buffer has to be of type uint16_t or int16_t.
/// @param readbuffer: Pointer to a buffer where read data is written. Note:
/// Buffer has to be of type uint16_t or int16_t.
/// @byte_len: The length of individual bytes in bits (usually 8)
/// @buffer_len: The length of the read and write buffers in bytes
/// (not local bytes but *byte_len* bytes)
bool uv_spi_readwrite_sync(const spi_e spi, spi_slaves_e slaves,
		const uint16_t *writebuffer, uint16_t *readbuffer,
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
		const uint16_t *writebuffer, const uint8_t byte_len, const uint16_t buffer_len);


#endif

#endif /* UV_HAL_INC_UV_SPI_H_ */
