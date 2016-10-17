/*
 * uv_spi.h
 *
 *  Created on: Aug 21, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_SPI_H_
#define UV_HAL_INC_UV_SPI_H_


#include "uv_hal_config.h"
#if (CONFIG_SPIO0 || CONFIG_SPIO1 || CONFIG_SPIO2)
#define CONFIG_SPI			1

#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#else
#error "SPI interface not yet implemented on this target"
#endif

#if CONFIG_TARGET_LPC1785

#if CONFIG_SPIO0
#if !CONFIG_SPIO0_RXBUF_LEN
#error "CONFIG_SPIO0_RXBUF_LEN should define the length in bytes of the receive buffer"
#endif
#if !CONFIG_SPIO0_BAUDRATE
#error "CONFIG_SPIO0_BAUDRATE should define the desired baudrate in Hz"
#endif

#endif
#if CONFIG_SPIO1
#if !CONFIG_SPIO1_RXBUF_LEN
#error "CONFIG_SPIO1_RXBUF_LEN should define the length in bytes of the receive buffer"
#endif
#if !CONFIG_SPIO1_BAUDRATE
#error "CONFIG_SPIO1_BAUDRATE should define the desired baudrate in Hz"
#endif

#endif
#if CONFIG_SPIO2
#if !CONFIG_SPIO2_RXBUF_LEN
#error "CONFIG_SPIO2_RXBUF_LEN should define the length in bytes of the receive buffer"
#endif
#if !CONFIG_SPIO2_BAUDRATE
#error "CONFIG_SPIO2_BAUDRATE should define the desired baudrate in Hz"
#endif


typedef enum {
	SPI0,
	SPI1,
	SPI2
} spi_e;


#endif

/// @brief: Initializes the SPI interface(s)
void uv_spi_init();


/// @brief: Sends the data to the selected slave device
///
/// @param spi: The SPI peripheral which sends the data
/// @param data: Pointer to the data  which should be sent
/// @param len: The length of the data to be sent
void uv_spi_send(spi_e spi, void *data, uint16_t len);


/// @brief: step function should be called every step cycle
void uv_spi_step(unsigned int step_ms);

#endif

#endif

#endif /* UV_HAL_INC_UV_SPI_H_ */
