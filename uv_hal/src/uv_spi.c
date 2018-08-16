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


#include "uv_spi.h"

#if CONFIG_SPI

#include "chip.h"
#include "spi_15xx.h"
#include "uv_rtos.h"

#if CONFIG_TARGET_LPC1549



void _uv_spi_init(void) {

#if CONFIG_SPI0
	{
		SPI_CFG_T cfg;
		SPI_DELAY_CONFIG_T delaycfg;
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
		LPC_IOCON->PIO[UV_GPIO_PORT(CONFIG_SPI0_MISO_IO)]
					   [UV_GPIO_PIN(CONFIG_SPI0_MISO_IO)] = PULL_DOWN_ENABLED;

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
		cfg.SSELPol |= ((CONFIG_SPI0_SSEL1_INV ? SPI_CFG_SPOL1_HI : SPI_CFG_SPOL1_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_SSEL1_IO),
				UV_GPIO_PIN(CONFIG_SPI0_SSEL1_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_SSELSN_1_IO,
				UV_GPIO_PORT(CONFIG_SPI0_SSEL1_IO), UV_GPIO_PIN(CONFIG_SPI0_SSEL1_IO));
#endif
#if (CONFIG_SPI0_SLAVE_COUNT > 2)
		cfg.SSELPol |= ((CONFIG_SPI0_SSEL2_INV ? SPI_CFG_SPOL2_HI : SPI_CFG_SPOL2_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI0_SSEL2_IO),
				UV_GPIO_PIN(CONFIG_SPI0_SSEL2_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI0_SSELSN_2_IO,
				UV_GPIO_PORT(CONFIG_SPI0_SSEL2_IO), UV_GPIO_PIN(CONFIG_SPI0_SSEL2_IO));
#endif
#if (CONFIG_SPI0_SLAVE_COUNT > 3)
		cfg.SSELPol |= ((CONFIG_SPI0_SSEL3_INV ? SPI_CFG_SPOL3_HI : SPI_CFG_SPOL3_LO);
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
		LPC_IOCON->PIO[UV_GPIO_PORT(CONFIG_SPI1_MISO_IO)]
					   [UV_GPIO_PIN(CONFIG_SPI1_MISO_IO)] = PULL_DOWN_ENABLED;

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
		cfg.SSELPol |= ((CONFIG_SPI1_SSEL1_INV ? SPI_CFG_SPOL1_HI : SPI_CFG_SPOL1_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_SSEL1_IO),
				UV_GPIO_PIN(CONFIG_SPI1_SSEL1_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_SSELSN_1_IO,
				UV_GPIO_PORT(CONFIG_SPI1_SSEL1_IO), UV_GPIO_PIN(CONFIG_SPI1_SSEL1_IO));
#endif
#if (CONFIG_SPI1_SLAVE_COUNT > 2)
		cfg.SSELPol |= ((CONFIG_SPI1_SSEL2_INV ? SPI_CFG_SPOL2_HI : SPI_CFG_SPOL2_LO);
		Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_SPI1_SSEL2_IO),
				UV_GPIO_PIN(CONFIG_SPI1_SSEL2_IO), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
		Chip_SWM_MovablePortPinAssign(SWM_SPI1_SSELSN_2_IO,
				UV_GPIO_PORT(CONFIG_SPI1_SSEL2_IO), UV_GPIO_PIN(CONFIG_SPI1_SSEL2_IO));
#endif
#if (CONFIG_SPI1_SLAVE_COUNT > 3)
		cfg.SSELPol |= ((CONFIG_SPI1_SSEL3_INV ? SPI_CFG_SPOL3_HI : SPI_CFG_SPOL3_LO);
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
		const uint16_t *writebuffer, uint16_t *readbuffer,
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
		const uint16_t *writebuffer, const uint8_t byte_len, const uint16_t buffer_len) {
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
