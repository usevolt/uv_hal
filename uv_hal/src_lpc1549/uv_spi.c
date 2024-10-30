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


#include "uv_spi.h"

#if CONFIG_SPI

#if CONFIG_SPI0
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
#if !defined(CONFIG_SPI0_MISO_PULL_UP) && !defined(CONFIG_SPI0_MISO_PULL_DOWN)
#error "CONFIG_SPI0_MISO_PULL_UP or CONFIG_SPI0_MISO_PULL_DOWN should be defined as 1 depending if the MISO \
line should have a pull up resistor or pull down resistor enabled."
#endif
#if CONFIG_SPI0_MISO_PULL_UP && CONFIG_SPI0_MISO_PULL_DOWN
#error "CONFIG_SPI0_MISO_PULL_UP and CONFIG_SPI0_MISO_PULL_DOWN cannot both be enabled."
#endif
#endif

#if CONFIG_SPI1
#if (CONFIG_SPI1_SLAVE_COUNT > 4)
#error "Maximum slave count is 4 for SPI1"
#elif (CONFIG_SPI1_SLAVE_COUNT > 3)
#if !defined(CONFIG_SPI1_SSEL3_IO)
#error "CONFIG_SPI1_SSEL3_IO should define the I/O pin used for slave 3 select"
#endif
#if !defined(CONFIG_SPI1_SSEL3_INV)
#error "CONFIG_SPI1_SSEL3_INV should be defined as 1 or 0, depending if the SSEL3\
 logic polarity should be inverted. (when 0, SSEL3 is active low.)"
#endif
#endif
#if (CONFIG_SPI1_SLAVE_COUNT > 2)
#if !defined(CONFIG_SPI1_SSEL2_IO)
#error "CONFIG_SPI1_SSEL2_IO should define the I/O pin used for slave 2 select"
#endif
#if !defined(CONFIG_SPI1_SSEL2_INV)
#error "CONFIG_SPI1_SSEL2_INV should be defined as 1 or 0, depending if the SSEL2\
 logic polarity should be inverted. (when 0, SSEL2 is active low.)"
#endif
#endif
#if (CONFIG_SPI1_SLAVE_COUNT > 1)
#if !defined(CONFIG_SPI1_SSEL1_IO)
#error "CONFIG_SPI1_SSEL1_IO should define the I/O pin used for slave 1 select"
#endif
#if !defined(CONFIG_SPI1_SSEL1_INV)
#error "CONFIG_SPI1_SSEL1_INV should be defined as 1 or 0, depending if the SSEL1\
 logic polarity should be inverted. (when 0, SSEL1 is active low.)"
#endif
#endif
#if (CONFIG_SPI1_SLAVE_COUNT > 0)
#if !defined(CONFIG_SPI1_SSEL0_IO)
#error "CONFIG_SPI1_SSEL0_IO should define the I/O pin used for slave 0 select"
#endif
#if !defined(CONFIG_SPI1_SSEL0_INV)
#error "CONFIG_SPI1_SSEL0_INV should be defined as 1 or 0, depending if the SSEL0\
 logic polarity should be inverted. (when 0, SSEL0 is active low.)"
#endif
#endif
#if !defined(CONFIG_SPI1_MSB_FIRST)
#error "CONFIG_SPI1_MSB_FIRST should be set to 1 or 0 depending if the most\
 significant bit should be transmitted first"
#endif
#if !defined(CONFIG_SPI1_PREDELAY)
#error "CONFIG_SPI1_PREDELAY should define the delay between SSEL and first bit"
#endif
#if !defined(CONFIG_SPI1_POSTDELAY)
#error "CONFIG_SPI1_POSTDELAY should define the delay between last bit and SSEL"
#endif
#if !defined(CONFIG_SPI1_FRAMEDELAY)
#error "CONFIG_SPI1_FRAMEDELAY should define the delay between frames"
#endif
#if !defined(CONFIG_SPI1_TRANSFERDELAY)
#error "CONFIG_SPI1_TRANSFERDELAY should define the minimum amount of time that SSEL's are\
 deasserted between frames."
#endif
#if !defined(CONFIG_SPI1_MISO_PULL_UP) && !defined(CONFIG_SPI1_MISO_PULL_DOWN)
#error "CONFIG_SPI1_MISO_PULL_UP or CONFIG_SPI1_MISO_PULL_DOWN should be defined as 1 depending if the MISO \
line should have a pull up resistor or pull down resistor enabled."
#endif
#if CONFIG_SPI1_MISO_PULL_UP && CONFIG_SPI1_MISO_PULL_DOWN
#error "CONFIG_SPI1_MISO_PULL_UP and CONFIG_SPI1_MISO_PULL_DOWN cannot both be enabled."
#endif
#endif



#if CONFIG_TARGET_LPC15XX


#include "chip.h"
#include "spi_15xx.h"
#include "uv_rtos.h"


void _uv_spi_init(void) {

#if CONFIG_SPI0
	{
		SPI_CFG_T cfg;
		SPI_DELAY_CONFIG_T delaycfg;
		Chip_SPI_DeInit(LPC_SPI0);
		Chip_SPI_Init(LPC_SPI0);

		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_SCK_IO),
				UV_GPIO_PIN(CONFIG_SPI0_SCK_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_SCK_IO,
				UV_GPIO_PORT(CONFIG_SPI0_SCK_IO), UV_GPIO_PIN(CONFIG_SPI0_SCK_IO));

		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_MOSI_IO),
				UV_GPIO_PIN(CONFIG_SPI0_MOSI_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_MOSI_IO,
				UV_GPIO_PORT(CONFIG_SPI0_MOSI_IO), UV_GPIO_PIN(CONFIG_SPI0_MOSI_IO));

		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_MISO_IO),
					UV_GPIO_PIN(CONFIG_SPI0_MISO_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_MISO_IO,
				UV_GPIO_PORT(CONFIG_SPI0_MISO_IO), UV_GPIO_PIN(CONFIG_SPI0_MISO_IO));
#if CONFIG_SPI0_MISO_PULL_UP
		LPC_IOCON->PIO[UV_GPIO_PORT(CONFIG_SPI0_MISO_IO)]
					   [UV_GPIO_PIN(CONFIG_SPI0_MISO_IO)] = PULL_UP_ENABLED;
#elif CONFIG_SPI0_MISO_PULL_DOWN
		LPC_IOCON->PIO[UV_GPIO_PORT(CONFIG_SPI0_MISO_IO)]
					   [UV_GPIO_PIN(CONFIG_SPI0_MISO_IO)] = PULL_DOWN_ENABLED;
#endif

		cfg.ClkDiv = Chip_SPI_CalClkRateDivider(LPC_SPI0, CONFIG_SPI0_BAUDRATE);

		cfg.Mode = SPI_MODE_MASTER;
		cfg.ClockMode = (CONFIG_SPI0_CLOCK_PHASE << 4) | (CONFIG_SPI0_CLOCK_POL << 5);
		// MSB or LSB first
		cfg.DataOrder = CONFIG_SPI0_MSB_FIRST ?
				SPI_DATA_MSB_FIRST : SPI_DATA_LSB_FIRST;
		// slave select polarity
		cfg.SSELPol = (CONFIG_SPI0_SSEL0_INV ? SPI_CFG_SPOL0_HI : SPI_CFG_SPOL0_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_SSEL0_IO),
				UV_GPIO_PIN(CONFIG_SPI0_SSEL0_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_SSELSN_0_IO,
				UV_GPIO_PORT(CONFIG_SPI0_SSEL0_IO), UV_GPIO_PIN(CONFIG_SPI0_SSEL0_IO));
#if (CONFIG_SPI0_SLAVE_COUNT > 1)
		cfg.SSELPol |= (CONFIG_SPI0_SSEL1_INV ? SPI_CFG_SPOL1_HI : SPI_CFG_SPOL1_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_SSEL1_IO),
				UV_GPIO_PIN(CONFIG_SPI0_SSEL1_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_SSELSN_1_IO,
				UV_GPIO_PORT(CONFIG_SPI0_SSEL1_IO), UV_GPIO_PIN(CONFIG_SPI0_SSEL1_IO));
#endif
#if (CONFIG_SPI0_SLAVE_COUNT > 2)
		cfg.SSELPol |= (CONFIG_SPI0_SSEL2_INV ? SPI_CFG_SPOL2_HI : SPI_CFG_SPOL2_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_SSEL2_IO),
				UV_GPIO_PIN(CONFIG_SPI0_SSEL2_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_SSELSN_2_IO,
				UV_GPIO_PORT(CONFIG_SPI0_SSEL2_IO), UV_GPIO_PIN(CONFIG_SPI0_SSEL2_IO));
#endif
#if (CONFIG_SPI0_SLAVE_COUNT > 3)
		cfg.SSELPol |= (CONFIG_SPI0_SSEL3_INV ? SPI_CFG_SPOL3_HI : SPI_CFG_SPOL3_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_SSEL3_IO),
				UV_GPIO_PIN(CONFIG_SPI0_SSEL3_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_SSELSN_3_IO,
				UV_GPIO_PORT(CONFIG_SPI0_SSEL3_IO), UV_GPIO_PIN(CONFIG_SPI0_SSEL3_IO));
#endif
						;
		Chip_SPI_SetConfig(LPC_SPI0, &cfg);
		/* Set Delay register */
		delaycfg.PreDelay = CONFIG_SPI0_PREDELAY;
		delaycfg.PostDelay = CONFIG_SPI0_POSTDELAY;
		delaycfg.FrameDelay = CONFIG_SPI0_FRAMEDELAY;
		delaycfg.TransferDelay = CONFIG_SPI0_TRANSFERDELAY;
		Chip_SPI_DelayConfig(LPC_SPI0, &delaycfg);

		Chip_SPI_DisableLoopBack(LPC_SPI0);
		Chip_SPI_Enable(LPC_SPI0);
	}
#endif
#if CONFIG_SPI1
	{
		SPI_CFG_T cfg;
		SPI_DELAY_CONFIG_T delaycfg;
		Chip_SPI_DeInit(LPC_SPI1);
		Chip_SPI_Init(LPC_SPI1);

		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_SCK_IO),
				UV_GPIO_PIN(CONFIG_SPI1_SCK_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_SCK_IO,
				UV_GPIO_PORT(CONFIG_SPI1_SCK_IO), UV_GPIO_PIN(CONFIG_SPI1_SCK_IO));

		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_MOSI_IO),
				UV_GPIO_PIN(CONFIG_SPI1_MOSI_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_MOSI_IO,
				UV_GPIO_PORT(CONFIG_SPI1_MOSI_IO), UV_GPIO_PIN(CONFIG_SPI1_MOSI_IO));

		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_MISO_IO),
					UV_GPIO_PIN(CONFIG_SPI1_MISO_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_MISO_IO,
				UV_GPIO_PORT(CONFIG_SPI1_MISO_IO), UV_GPIO_PIN(CONFIG_SPI1_MISO_IO));
#if CONFIG_SPI1_MISO_PULL_UP
		LPC_IOCON->PIO[UV_GPIO_PORT(CONFIG_SPI1_MISO_IO)]
					   [UV_GPIO_PIN(CONFIG_SPI1_MISO_IO)] = PULL_UP_ENABLED;
#elif CONFIG_SPI1_MISO_PULL_DOWN
		LPC_IOCON->PIO[UV_GPIO_PORT(CONFIG_SPI1_MISO_IO)]
					   [UV_GPIO_PIN(CONFIG_SPI1_MISO_IO)] = PULL_DOWN_ENABLED;
#endif

		cfg.ClkDiv = Chip_SPI_CalClkRateDivider(LPC_SPI1, CONFIG_SPI1_BAUDRATE);
		cfg.Mode = SPI_MODE_MASTER;
		cfg.ClockMode = (CONFIG_SPI1_CLOCK_PHASE << 4) | (CONFIG_SPI1_CLOCK_POL << 5);
		// MSB or LSB first
		cfg.DataOrder = CONFIG_SPI1_MSB_FIRST ?
				SPI_DATA_MSB_FIRST : SPI_DATA_LSB_FIRST;
		// slave select polarity
		cfg.SSELPol = (CONFIG_SPI1_SSEL0_INV ? SPI_CFG_SPOL0_HI : SPI_CFG_SPOL0_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_SSEL0_IO),
				UV_GPIO_PIN(CONFIG_SPI1_SSEL0_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_SSELSN_0_IO,
				UV_GPIO_PORT(CONFIG_SPI1_SSEL0_IO), UV_GPIO_PIN(CONFIG_SPI1_SSEL0_IO));
#if (CONFIG_SPI1_SLAVE_COUNT > 1)
		cfg.SSELPol |= ((CONFIG_SPI1_SSEL1_INV) ? SPI_CFG_SPOL1_HI : SPI_CFG_SPOL1_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_SSEL1_IO),
				UV_GPIO_PIN(CONFIG_SPI1_SSEL1_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_SSELSN_1_IO,
				UV_GPIO_PORT(CONFIG_SPI1_SSEL1_IO), UV_GPIO_PIN(CONFIG_SPI1_SSEL1_IO));
#endif
#if (CONFIG_SPI1_SLAVE_COUNT > 2)
		cfg.SSELPol |= ((CONFIG_SPI1_SSEL2_INV) ? SPI_CFG_SPOL2_HI : SPI_CFG_SPOL2_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_SSEL2_IO),
				UV_GPIO_PIN(CONFIG_SPI1_SSEL2_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_SSELSN_2_IO,
				UV_GPIO_PORT(CONFIG_SPI1_SSEL2_IO), UV_GPIO_PIN(CONFIG_SPI1_SSEL2_IO));
#endif
#if (CONFIG_SPI1_SLAVE_COUNT > 3)
		cfg.SSELPol |= ((CONFIG_SPI1_SSEL3_INV) ? SPI_CFG_SPOL3_HI : SPI_CFG_SPOL3_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_SSEL3_IO),
				UV_GPIO_PIN(CONFIG_SPI1_SSEL3_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_SSELSN_3_IO,
				UV_GPIO_PORT(CONFIG_SPI1_SSEL3_IO), UV_GPIO_PIN(CONFIG_SPI1_SSEL3_IO));
#endif
						;
		Chip_SPI_SetConfig(LPC_SPI1, &cfg);
		/* Set Delay register */
		delaycfg.PreDelay = CONFIG_SPI1_PREDELAY;
		delaycfg.PostDelay = CONFIG_SPI1_POSTDELAY;
		delaycfg.FrameDelay = CONFIG_SPI1_FRAMEDELAY;
		delaycfg.TransferDelay = CONFIG_SPI1_TRANSFERDELAY;
		Chip_SPI_DelayConfig(LPC_SPI1, &delaycfg);

		Chip_SPI_DisableLoopBack(LPC_SPI1);
		Chip_SPI_Enable(LPC_SPI1);
	}
#endif
}



bool uv_spi_readwrite_sync(const spi_e spi, spi_slaves_e slaves,
		const spi_data_t *writebuffer, spi_data_t *readbuffer,
		const uint8_t byte_len, const uint16_t buffer_len) {
	bool ret = true;

	// note: Make sure to specifically deassert all nodes not used for transmission
	SPI_DATA_SETUP_T setup;
	setup.pTx = (uint16_t*) writebuffer;
	setup.pRx = readbuffer;
	setup.DataSize = byte_len;
	setup.Length = buffer_len;
	setup.ssel = SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_DEASSERT_SSEL1 |
			SPI_TXCTL_DEASSERT_SSEL2 | SPI_TXCTL_DEASSERT_SSEL3;
	setup.ssel &= ~(slaves << 16);
	setup.TxCnt = 0;
	setup.RxCnt = 0;
	// Transfer message as SPI master via polling
	if (Chip_SPI_RWFrames_Blocking(spi, &setup) <= 0) {
		// SPI error
		ret = false;
	}

	return ret;
}


bool uv_spi_write_sync(const spi_e spi, spi_slaves_e slaves,
		const spi_data_t *writebuffer, const uint8_t byte_len, const uint16_t buffer_len) {
	bool ret = true;

	// note: Make sure to specifically deassert all nodes not used for transmission
	SPI_DATA_SETUP_T setup;
	setup.pTx = (uint16_t*) writebuffer;
	setup.pRx = NULL;
	setup.DataSize = byte_len;
	setup.Length = buffer_len;
	setup.ssel = SPI_TXCTL_DEASSERT_SSEL0 | SPI_TXCTL_DEASSERT_SSEL1 |
			SPI_TXCTL_DEASSERT_SSEL2 | SPI_TXCTL_DEASSERT_SSEL3;
	setup.ssel &= ~(slaves << 16);
	setup.TxCnt = 0;
	setup.RxCnt = 0;
	// Transfer message as SPI master via polling
	if (Chip_SPI_WriteFrames_Blocking(spi, &setup) <= 0) {
		// SPI error
		ret = false;
	}

	return ret;

}


#endif

#endif
