/*
 * uv_spi.h
 *
 *  Created on: Aug 21, 2016
 *      Author: usevolt
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

typedef enum {
	SPI0
} spi_e;

#endif

void _uv_spi_init(void);



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

#endif /* UV_HAL_INC_UV_SPI_H_ */
