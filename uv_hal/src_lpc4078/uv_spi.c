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

#if CONFIG_TARGET_LPC40XX

#include "chip.h"
#include "uv_rtos.h"

LPC_SSP_T *spi[3] = {
		LPC_SSP0,
		LPC_SSP1,
		LPC_SSP2
};
static const CHIP_SSP_CLOCK_MODE_T clocks[3] = {
 #if CONFIG_SPI0_CLOCK_PHASE == 0
#if CONFIG_SPI0_CLOCK_POL == 0
		SSP_CLOCK_CPHA0_CPOL0,
#else
		SSP_CLOCK_CPHA1_CPOL0,
#endif
#else
#if CONFIG_SPI0_CLOCK_POL == 0
		SSP_CLOCK_CPHA1_CPOL0,
#else
		SSP_CLOCK_CPHA1_CPOL1,
#endif
#endif
#if CONFIG_SPI1_CLOCK_PHASE == 0
#if CONFIG_SPI1_CLOCK_POL == 0
		SSP_CLOCK_CPHA0_CPOL0,
#else
		SSP_CLOCK_CPHA1_CPOL0,
#endif
#else
#if CONFIG_SPI1_CLOCK_POL == 0
		SSP_CLOCK_CPHA1_CPOL0,
#else
		SSP_CLOCK_CPHA1_CPOL1,
#endif
#endif
#if CONFIG_SPI2_CLOCK_PHASE == 0
#if CONFIG_SPI2_CLOCK_POL == 0
		SSP_CLOCK_CPHA0_CPOL0
#else
		SSP_CLOCK_CPHA1_CPOL0
#endif
#else
#if CONFIG_SPI2_CLOCK_POL == 0
		SSP_CLOCK_CPHA1_CPOL0
#else
		SSP_CLOCK_CPHA1_CPOL1
#endif
#endif
};


#if CONFIG_SPI0_SSEL0_INV || CONFIG_SPI1_SSEL0_INV || CONFIG_SPI2_SSEL0_INV
#error "CONFIG_SPIX_SSEL0_INV disabled on LPV4078, defaults to 0."
#endif

void _uv_spi_init(void) {

	#if CONFIG_SPI0
	{
		Chip_SSP_DeInit(spi[0]);
		Chip_SSP_Init(spi[0]);

#if CONFIG_SPI0_SCK_IO == P0_15
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 15, FUNC2);
#elif CONFIG_SPI0_SCK_IO == P1_20
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 20, FUNC5);
#elif CONFIG_SPI0_SCK_IO == P2_22
		Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 22, FUNC5);
#error "No suitable pin for CONFIG_SPI0_SCK_IO"
#endif
#if CONFIG_SPI0_SSEL0_IO == P0_16
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 16, FUNC2);
#elif CONFIG_SPI0_SSEL0_IO == P1_21
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 21, FUNC3);
#elif CONFIG_SPIO0_SSEL0_IO == P1_28
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 28, FUNC5);
#elif CONFIG_SPI0_SEL0_IO == P2_23
		Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 23, FUNC2);
#error "No suitable pin for CONFIG_SPI0_SSEL0_IO"
#endif
#if CONFIG_SPI0_MISO_IO == P0_17
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 17, FUNC2);
#elif CONFIG_SPI0_MISO_IO == P1_23
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 23, FUNC5);
#elif CONFIG_SPI0_MISO_IO == P2_26
		Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 26, FUNC2);
#error "No suitable pin for CONFIG_SPI0_MISO_IO"
#endif
#if CONFIG_SPI0_MOSI_IO == P0_18
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 18, FUNC2);
#elif CONFIG_SPI0_MOSI_IO == P1_24
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 24, FUNC5);
#elif CONFIG_SPI0_MOSI_IO == P2_27
		Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 27, FUNC2);
#else
#error "No suitable pin for CONFIG_SPI0_MOSI_IO"
#endif
#if CONFIG_SPI0_SLAVE_COUNT > 1
#error "LPC4078 SSP interface directly supports only 1 slave. Multiple slaves \
 need to be implemented with gpio pins."
#endif
		Chip_SSP_Set_Mode(spi[0], SSP_MODE_MASTER);
		Chip_SSP_SetFormat(spi[0], SSP_BITS_8, SSP_FRAMEFORMAT_SPI, clocks[0]);
		Chip_SSP_SetBitRate(spi[0], CONFIG_SPI0_BAUDRATE);
		Chip_SSP_Enable(spi[0]);
	}
#endif
#if CONFIG_SPI1
	{
		Chip_SSP_DeInit(spi[1]);
		Chip_SSP_Init(spi[1]);

#if CONFIG_SPI1_SCK_IO == P0_7
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 7, FUNC2 | MD_ANA_DIS | MD_FAST_SLEW_RATE);
#elif CONFIG_SPI1_SCK_IO == P1_19
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 19, FUNC5);
#elif CONFIG_SPI1_SCK_IO == P1_31
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 31, FUNC2 | MD_ANA_DIS);
#elif CONFIG_SPI1_SCK_IO == P4_20
		Chip_IOCON_PinMuxSet(LPC_IOCON, 4, 20, FUNC3);
#error "No suitable pin for CONFIG_SPI1_SCK_IO"
#endif
#if CONFIG_SPI1_SSEL0_IO == P0_6
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 6, FUNC2);
#elif CONFIG_SPI1_SSEL0_IO == P0_14
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 14, FUNC2);
#elif CONFIG_SPIO0_SSEL0_IO == P1_26
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 26, FUNC5);
#elif CONFIG_SPI1_SEL0_IO == P4_21
		Chip_IOCON_PinMuxSet(LPC_IOCON, 4, 21, FUNC3);
#error "No suitable pin for CONFIG_SPI1_SSEL0_IO"
#endif
#if CONFIG_SPI1_MISO_IO == P0_8
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 8, FUNC2 | MD_ANA_DIS | MD_FAST_SLEW_RATE);
#elif CONFIG_SPI1_MISO_IO == P0_12
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 12, FUNC2 | MD_ANA_DIS);
#elif CONFIG_SPI1_MISO_IO == P1_18
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 18, FUNC5);
#elif CONFIG_SPI1_MISO_IO == P4_22
		Chip_IOCON_PinMuxSet(LPC_IOCON, 4, 22, FUNC3);
#error "No suitable pin for CONFIG_SPI1_MISO_IO"
#endif
#if CONFIG_SPI1_MOSI_IO == P0_9
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 9, FUNC2 | MD_FILT_DIS | MD_FAST_SLEW_RATE);
#elif CONFIG_SPI1_MOSI_IO == P0_13
		Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 13, FUNC2 | MD_ANA_DIS);
#elif CONFIG_SPI1_MOSI_IO == P1_22
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 22, FUNC5);
#elif CONFIG_SPI1_MOSI_IO == P4_23
		Chip_IOCON_PinMuxSet(LPC_IOCON, 4, 23, FUNC3);
#else
#error "No suitable pin for CONFIG_SPI1_MOSI_IO"
#endif
#if CONFIG_SPI1_SLAVE_COUNT > 1
#error "LPC4078 SSP interface directly supports only 1 slave. Multiple slaves \
 need to be implemented with gpio pins."
#endif
		Chip_SSP_Set_Mode(spi[1], SSP_MODE_MASTER);
		Chip_SSP_SetFormat(spi[1], SSP_BITS_8, SSP_FRAMEFORMAT_SPI, clocks[1]);
		Chip_SSP_SetBitRate(spi[1], CONFIG_SPI1_BAUDRATE);
		Chip_SSP_Enable(spi[1]);
	}
#endif
#if CONFIG_SPI2
	{
		Chip_SSP_DeInit(spi[2]);
		Chip_SSP_Init(spi[2]);

#if CONFIG_SPI2_SCK_IO == P1_0
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 0, FUNC4);
#elif CONFIG_SPI2_SCK_IO == P5_2
		Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 2, FUNC2 | MD_HS_DIS);
#error "No suitable pin for CONFIG_SPI2_SCK_IO"
#endif
#if CONFIG_SPI2_SSEL0_IO == P1_8
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 8, FUNC4);
#elif CONFIG_SPI2_SSEL0_IO == P5_3
		Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 3, FUNC2 | MD_HS_DIS);
#error "No suitable pin for CONFIG_SPI2_SSEL0_IO"
#endif
#if CONFIG_SPI2_MISO_IO == P1_4
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 4, FUNC4);
#elif CONFIG_SPI2_MISO_IO == P5_1
		Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 1, FUNC2);
#error "No suitable pin for CONFIG_SPI2_MISO_IO"
#endif
#if CONFIG_SPI2_MOSI_IO == P1_1
		Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 1, FUNC4);
#elif CONFIG_SPI2_MOSI_IO == P5_0
		Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 0, FUNC2);
#else
#error "No suitable pin for CONFIG_SPI2_MOSI_IO"
#endif
#if CONFIG_SPI2_SLAVE_COUNT > 1
#error "LPC4078 SSP interface directly supports only 1 slave. Multiple slaves \
 need to be implemented with gpio pins."
#endif
		Chip_SSP_Set_Mode(spi[2], SSP_MODE_MASTER);
		Chip_SSP_SetFormat(spi[2], SSP_BITS_8, SSP_FRAMEFORMAT_SPI, clocks[2]);
		Chip_SSP_SetBitRate(spi[2], CONFIG_SPI2_BAUDRATE);
		Chip_SSP_Enable(spi[2]);
	}
#endif
}



uint16_t uv_spi_readwrite_sync(const spi_e s, spi_slaves_e slaves,
		const spi_data_t *writebuffer, spi_data_t *readbuffer,
		const uint8_t byte_len, const uint16_t buffer_len) {
	uint16_t ret;

	Chip_SSP_SetFormat(spi[s], byte_len - 1, SSP_FRAMEFORMAT_SPI, clocks[s]);
	Chip_SSP_DATA_SETUP_T setup = { };
	setup.length = buffer_len;
	setup.rx_cnt = setup.tx_cnt = 0;
	setup.tx_data = (void*) writebuffer;
	setup.rx_data = (void*) readbuffer;

	// Transfer message as SPI master via polling
	ret = Chip_SSP_RWFrames_Blocking(spi[s], &setup);

	return ret;
}


uint16_t uv_spi_write_sync(const spi_e s, spi_slaves_e slaves,
		const spi_data_t *writebuffer, const uint8_t byte_len, const uint16_t buffer_len) {
	uint16_t ret;

	Chip_SSP_SetFormat(spi[s], byte_len - 1, SSP_FRAMEFORMAT_SPI, clocks[s]);

	// Transfer message as SPI master via polling
	ret = Chip_SSP_WriteFrames_Blocking(spi[s], (uint8_t*) writebuffer, buffer_len);

	return ret;

}


#endif

#endif
