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


#include "uv_emc.h"
#include "uv_utilities.h"
#include "uv_gpio.h"
#include <stdio.h>
#include <string.h>
#include <uv_terminal.h>
#if CONFIG_TARGET_LPC1785
#include "LPC177x_8x.h"
#endif
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#endif

#if CONFIG_TARGET_LPC1785



#if CONFIG_EMC_SDRAM_2 || CONFIG_EMC_SDRAM_3 || CONFIG_EMC_SDRAM_4
#warning "uv_emc module not tested on multiple SDRAM chips!"
#endif


uv_errors_e _uv_emc_init( void ) {


	LPC_SC->EMCCLKSEL = CONFIG_EMC_CLOCK_DIV_HALF;

	LPC_SC->PCONP      |= 0x00000800;
	LPC_SC->EMCDLYCTL   = 0x00001010;
	LPC_EMC->Control    = 0x00000001;
	LPC_EMC->Config     = 0x00000000;

	LPC_IOCON->P2_16 |= 0x201; // CAS
	LPC_IOCON->P2_17 |= 0x201; // RAS
	LPC_IOCON->P2_18 |= 0x201; // RAM clk
#if CONFIG_EMC_SDRAM_1
	LPC_IOCON->P2_20 |= 0x201; // DYSC0
	LPC_IOCON->P2_24 |= 0x201; // CLKEN0
#endif
#if CONFIG_EMC_SDRAM_2
	LPC_IOCON->P2_21 |= 0x201; // DYSC1
	LPC_IOCON->P2_25 |= 0x201; // CLKEN1
#endif
#if CONFIG_EMC_SDRAM_3
	LPC_IOCON->P2_22 |= 0x201; // DYSC2
	LPC_IOCON->P2_26 |= 0x201; // CLKEN2
#endif
#if CONFIG_EMC_SDRAM_4
	LPC_IOCON->P2_23 |= 0x201; // DYSC3
	LPC_IOCON->P2_27 |= 0x201; // CLKEN3
#endif
	LPC_IOCON->P2_28 |= 0x201; // LDQM
	LPC_IOCON->P2_29 |= 0x201; // UDQM
	// D0-15
	LPC_IOCON->P3_0 |= 0x201;
	LPC_IOCON->P3_1 |= 0x201;
	LPC_IOCON->P3_2 |= 0x201;
	LPC_IOCON->P3_3 |= 0x201;
	LPC_IOCON->P3_4 |= 0x201;
	LPC_IOCON->P3_5 |= 0x201;
	LPC_IOCON->P3_6 |= 0x201;
	LPC_IOCON->P3_7 |= 0x201;
	LPC_IOCON->P3_8 |= 0x201;
	LPC_IOCON->P3_9 |= 0x201;
	LPC_IOCON->P3_10 |= 0x201;
	LPC_IOCON->P3_11 |= 0x201;
	LPC_IOCON->P3_12 |= 0x201;
	LPC_IOCON->P3_13 |= 0x201;
	LPC_IOCON->P3_14 |= 0x201;
	LPC_IOCON->P3_15 |= 0x201;
	//A0-13
	LPC_IOCON->P4_0 |= 0x201;
	LPC_IOCON->P4_1 |= 0x201;
	LPC_IOCON->P4_2 |= 0x201;
	LPC_IOCON->P4_3 |= 0x201;
	LPC_IOCON->P4_4 |= 0x201;
	LPC_IOCON->P4_5 |= 0x201;
	LPC_IOCON->P4_6 |= 0x201;
	LPC_IOCON->P4_7 |= 0x201;
	LPC_IOCON->P4_8 |= 0x201;
	LPC_IOCON->P4_9 |= 0x201;
	LPC_IOCON->P4_10 |= 0x201;
	LPC_IOCON->P4_11 |= 0x201;
	LPC_IOCON->P4_13 |= 0x201; // BA0
	LPC_IOCON->P4_14 |= 0x201; // BA1

	LPC_IOCON->P4_25 |= 0x201; // WE

	LPC_EMC->DynamicReadConfig = 1;

	LPC_EMC->DynamicConfig0 = (CONFIG_EMC_SDRAM_AM0 << 7) + (CONFIG_EMC_SDRAM_AM1 << 14);
	LPC_EMC->DynamicRasCas0 = CONFIG_EMC_SDRAM_RAS + (CONFIG_EMC_SDRAM_CAS << 8);
	LPC_EMC->DynamicRP         = 0x00000003;
	LPC_EMC->DynamicRAS        = 0x00000006;
	LPC_EMC->DynamicSREX       = 0x0000000A;
	LPC_EMC->DynamicAPR        = 0x00000005;
	LPC_EMC->DynamicDAL        = 0x00000006;
	LPC_EMC->DynamicWR         = 0x00000002;
	LPC_EMC->DynamicRC         = 0x00000008;
	LPC_EMC->DynamicRFC        = 0x00000008;
	LPC_EMC->DynamicXSR        = 0x0000000A;
	LPC_EMC->DynamicRRD        = 0x00000002;
	LPC_EMC->DynamicMRD        = 0x00000003;

	_delay_ms(100);   /* wait 100ms */
	LPC_EMC->DynamicControl    = 0x00000183; /* Issue NOP command */
	_delay_ms(200);   /* wait 200ms */
	LPC_EMC->DynamicControl    = 0x00000103; /* Issue PALL command */
	LPC_EMC->DynamicRefresh    = 0x00000002; /* ( n * 16 ) -> 32 clock cycles */

	volatile uint32_t i, dwtemp;
	for(i = 0; i < 0x80; i++);           /* wait 128 AHB clock cycles */

	LPC_EMC->DynamicRefresh = CONFIG_EMC_SDRAM_REFRESH;
	LPC_EMC->DynamicControl    = 0x00000083; /* Issue MODE command */

	dwtemp = *((volatile uint32_t *)(EMC_SDRAM_1 | CONFIG_EMC_SDRAM_MODE_REGISTER));
	if (dwtemp) {}
	LPC_EMC->DynamicControl    = 0x00000000; /* Issue NORMAL command */

	//[re]enable buffers
	LPC_EMC->DynamicConfig0    |= 0x00080000; // Buffer enable
	_delay_ms(100);

	return ERR_NONE;
}




#endif
