/*
 * uv_dma.h
 *
 *  Created on: Apr 2, 2017
 *      Author: usevolt
 */

#ifndef UV_HAL_UV_DMA_H_
#define UV_HAL_UV_DMA_H_

/// @brief: Provides an interface to use Direct Memory Access peripheral
/// for microcontrollers which have a DMA controller

#include <uv_hal_config.h>
#include "uv_utilities.h"


#if CONFIG_TARGET_LPC1785

#if !defined(CONFIG_DMA)
#error "CONFIG_DMA should be defined as 1 or 0, depending if the DMA peripheral should be enabled"
#endif
#if CONFIG_DMA

/// @brief: Copies a block of memory from *src* to *dest*.
///
/// @return: True if the transfer could be initiated. False if all DMA channels were
/// busy. Note that returning true doesn't mean that the transfer is done yet.
///
/// @param dest: Destination address. Note that this function doesn't check if the
/// destination address is writable. It is left as the responsibility of the application.
/// @param src: Source address from here the data is copied. Note that this function doesn't check
/// if the source address is readable. It is left as the resposibility of the application.
/// @param len: The length of the copied data in bytes. Note that the DMA controller supports
/// natively only 32-bit transfers. Thus *len* should be 4-byte aligned.
bool uv_dma_memcpy(void *dest, const void *src, uint32_t len);


/// @brief: Sets a block of memory of *dest* address to the value of *value*.
///
/// @return: True if the transfer could be initiated. False if all DMA channels were
/// busy or some other error occurred.
/// Note that returning true doesn't mean that the transfer is done yet.
///
/// @param dest: Destination address. Note that this function doesn't check if the
/// destination address is writable. It is left as the responsibility of the application.
/// @param value: The value to what the destination memory will be set. Note that the value
/// is 32-bit.
/// @param len: The length of the copied data in bytes. Note that the DMA controller supports
/// natively only 32-bit transfers. Thus *len* should be 4-byte aligned.
bool uv_dma_memset(void *dest, int32_t value, uint32_t len);



/// @brief: Initializes the DMA controller. Will be called from the HAL-thread, user application
/// shouldn't call this.
void _uv_dma_init(void);

#endif

#endif


#endif /* UV_HAL_UV_DMA_H_ */
