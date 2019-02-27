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




uv_errors_e _uv_i2c_init(void) {

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, IOCON_DIGMODE_EN | IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, IOCON_DIGMODE_EN | IOCON_STDI2C_EN);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SCL);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SDA);

	// Enable I2C clock and reset I2C peripheral - the boot ROM does not do this
	Chip_I2C_Init(LPC_I2C0);

	// Setup clock rate for I2C
	// This comes directly from lpcopen examples
	Chip_I2C_SetClockDiv(LPC_I2C0, 40);

	// Setup I2CM transfer rate
	Chip_I2CM_SetBusSpeed(LPC_I2C0, CONFIG_I2C_BAUDRATE);

#if (CONFIG_I2C_MODE == I2C_MASTER)
	// Enable Master Mode
	Chip_I2CM_Enable(LPC_I2C0);
#else
#error "I2C Slave mode not yet implemented"
#endif


	return ERR_NONE;
}

uv_errors_e uv_i2c_masterstart(i2c_e i2c) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uv_errors_e uv_i2c_masterstop(i2c_e i2c) {
	uv_errors_e ret = ERR_NONE;

	return ret;
}





#endif
