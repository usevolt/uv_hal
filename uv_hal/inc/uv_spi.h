/*
 * uv_spi.h
 *
 *  Created on: Aug 21, 2016
 *      Author: usevolt
 */

#ifndef UV_HAL_INC_UV_SPI_H_
#define UV_HAL_INC_UV_SPI_H_

#include "uv_hal_config.h"
#if CONFIG_SPI

#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#else
#error "SPI interface not yet implemented on this target"
#endif



void uv_spi_init(void);


#endif
#endif /* UV_HAL_INC_UV_SPI_H_ */
