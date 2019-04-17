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



#include <uv_i2c.h>

#if CONFIG_I2C

#include <i2cm_15xx.h>
#include <swm_15xx.h>
#include <iocon_15xx.h>
#include <uv_rtos.h>



static LPC_I2C_T *p[i2C_COUNT] = {
		LPC_I2C
};


uv_errors_e _uv_i2c_init(void) {

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, IOCON_DIGMODE_EN | IOCON_SFI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, IOCON_DIGMODE_EN | IOCON_SFI2C_EN);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SCL);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SDA);

	// Enable I2C clock and reset I2C peripheral - the boot ROM does not do this
	Chip_I2C_Init(LPC_I2C0);

	// Setup I2CM duty cycles
	// i2c clock should be 8 times the bus frequency
	Chip_I2CM_SetDutyCycle(LPC_I2C0, 0x4, 0x4);

	// Setup clock rate for I2C
	// This comes directly from lpcopen examples
	Chip_I2C_SetClockDiv(LPC_I2C0, Chip_Clock_GetMainClockRate() / (CONFIG_I2C_BAUDRATE * 8));

#if (CONFIG_I2C_MODE == I2C_MASTER)
	// Enable Master Mode
	Chip_I2CM_Enable(LPC_I2C0);
#else
#error "I2C Slave mode not yet implemented"
#endif


	return ERR_NONE;
}


static I2CM_XFER_T xfer;

uv_errors_e uv_i2cm_readwrite(i2c_e i2c, uint8_t dev_addr, uint8_t *tx_buffer, uint16_t tx_len,
		uint8_t *rx_buffer, uint16_t rx_len) {
	uv_errors_e ret = ERR_NONE;

	while (!Chip_I2CM_IsMasterPending(p[i2c])) {
		uv_rtos_task_yield();
	}

	xfer.slaveAddr = dev_addr;
	xfer.rxBuff = rx_buffer;
	xfer.rxSz = rx_len;
	xfer.txBuff = tx_buffer;
	xfer.txSz = tx_len;
	xfer.status = 0;

	uint32_t r = 0;
	/* start transfer */
	Chip_I2CM_Xfer(p[i2c], &xfer);

	while (r == 0) {
		/* wait for status change interrupt */
		while (!Chip_I2CM_IsMasterPending(p[i2c])) {
			uv_rtos_task_yield();
		}
		/* call state change handler */
		r = Chip_I2CM_XferHandler(p[i2c], &xfer);
	}

	if (xfer.status == I2CM_STATUS_ARBLOST) {
		ret = ERR_ABORTED;
	}

	return ret;
}





#endif
