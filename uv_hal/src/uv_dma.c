/*
 * uv_dma.c
 *
 *  Created on: Apr 2, 2017
 *      Author: usevolt
 */


#include "uv_dma.h"
#include "uv_rtos.h"


#if CONFIG_DMA

#define DMA_CHANNEL_COUNT	8


void _uv_dma_init(void) {
	// set up power to DMA
	LPC_SC->PCONP |= (1 << 29);
	// enable DMA
	LPC_GPDMA->Config = 1;
}

/// @brief: Finds and returns the biggest priority channel which is inactive.
///
/// @note: Interrupts should be disabled while calling this, to ensure
/// that no other threads can catch found free channel and make it busy
static volatile LPC_GPDMACH_TypeDef* get_free_channel(void);



static volatile LPC_GPDMACH_TypeDef* get_free_channel(void) {

	// if the lowest priority transfer is enabled, any other channel cannot be
	// enabled since transfer order shouldn't be messed
		// wait until the transfer completes
	while (LPC_GPDMA->EnbldChns & (1 << (DMA_CHANNEL_COUNT - 1))) {
//		uv_rtos_task_yield();
	}

	// cycle trough all DMA channels in priority increasing order and
	// find out the biggest priority channel of the lowest free channels
	int8_t free_chn;
	// note: 2nd smallest priority channel is the one we check,
	// since at this point the least priority channel is known to be free
	for (free_chn = DMA_CHANNEL_COUNT - 2; free_chn >= 0; free_chn--) {
		if (LPC_GPDMA->EnbldChns & (1 << free_chn)) {
			// found a busy channel
			break;
		}
	}
	// increment the channel by one (to point to a free channel)
	free_chn++;

	return (LPC_GPDMACH_TypeDef*) (LPC_GPDMACH0_BASE + 0x20 * free_chn);
}



bool uv_dma_memcpy(void *dest, const void *src, uint32_t len) {
	bool ret = true;
	if ((dest == NULL) || (src == NULL)) {
		ret = false;
	}
	if (ret) {
		uint8_t *d = dest;
		const uint8_t *s = src;
		while (d < ((uint8_t*) dest + len)) {
			volatile LPC_GPDMACH_TypeDef* chn = get_free_channel();
			chn->CSrcAddr = (uint32_t) s;
			chn->CDestAddr = (uint32_t) d;
			chn->CLLI = 0;
			chn->CControl = ((len / 4) & 0xFFF) |
					(0b111 << 12) |
					(0b111 << 15) |
					(0b010 << 18) |
					(0b010 << 21) |
					(1 << 26) |
					(1 << 27);
			chn->CConfig = 1;
			d += ((len / 4) & 0xFFF);
			s += ((len / 4) & 0xFFF);
		}
	}
	return ret;
}



bool uv_dma_memset(void *dest, int32_t value, uint32_t len) {
	bool ret = true;
	if ((len % 4 != 0) || (dest == NULL)) {
		ret = false;
	}
	if (ret) {
		uint8_t *d = dest;
		while (d < ((uint8_t*) dest + len)) {
			volatile LPC_GPDMACH_TypeDef* chn = get_free_channel();
			chn->CSrcAddr = (uint32_t) &value;
			chn->CDestAddr = (uint32_t) d;
			chn->CLLI = 0;
			chn->CControl = ((len / 4) & 0xFFF) |
					(0b111 << 12) |
					(0b111 << 15) |
					(0b010 << 18) |
					(0b010 << 21) |
					(1 << 27);
			chn->CConfig = 1;
			d += ((len / 4) & 0xFFF);
		}
	}
	return ret;
}


#endif
