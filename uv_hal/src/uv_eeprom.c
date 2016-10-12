/*
 * uv_eeprom.c
 *
 *  Created on: Aug 25, 2016
 *      Author: usevolt
 */


#include "uv_eeprom.h"

#if CONFIG_EEPROM

uv_errors_e uv_eeprom_init(void) {
	// enable power
	LPC_EEPROM->PWRDWN = 0;

	// set clock frequency to 375 kHz
	LPC_EEPROM->CLKDIV = SystemCoreClock / 375000 - 1;

	// set the required timing register values
	LPC_EEPROM->WSTATE = (unsigned long int) 15 * SystemCoreClock / 1000000000 |	// min 15 ns
			(((unsigned long int) 55 * SystemCoreClock / 1000000000) << 8) |		// min 55 ns
			(((unsigned long int) 35 * SystemCoreClock / 1000000000) << 16);		// min 35 ns

	return uv_err(ERR_NONE);
}


uv_errors_e uv_eeprom_write(unsigned char *data, uint16_t len, uint16_t eeprom_addr) {
	// out of EEPROM memory
	if (eeprom_addr + len >= _UV_EEPROM_SIZE) {
		return uv_err(ERR_NOT_ENOUGH_MEMORY |HAL_MODULE_EEPROM);
	}
	// clear pending interrupt flag
	LPC_EEPROM->INT_CLR_STATUS = (1 << 28);

	// for simplicity every 8 byte is written
	uint8_t page = eeprom_addr % _UV_EEPROM_PAGE_SIZE;

	uint16_t i;
	// write the actual data
	for (i = 0; i < len; i++) {
		// clear the write/read status bit
		LPC_EEPROM->INT_CLR_STATUS = (1 << 26);
		// write 8-byte
		LPC_EEPROM->CMD = 0x3;
		// set the write address
		LPC_EEPROM->ADDR = eeprom_addr + i;
		// write data to the page register
		LPC_EEPROM->WDATA = data[i];
		// increase page counter
		page++;

		// if the page is full or the last byte was written, write the data to the EEPROM
		if (page >= _UV_EEPROM_PAGE_SIZE || i == len - 1) {
			while(!(LPC_EEPROM->INT_STATUS & (1 << 26)));
			// set the address. (6 LSB bits are don't care).
			LPC_EEPROM->ADDR = eeprom_addr + i;
			// flash the page register to EEPROM
			LPC_EEPROM->CMD = 0x6;
			// wait for the transaction to finish
			while (!(LPC_EEPROM->INT_STATUS & (1 << 28)));
			// clear the interrupt flag
			LPC_EEPROM->INT_CLR_STATUS = (1 << 28);
			// clear page counter to start the next page programming
			page = 0;
		}
	}

	return uv_err(ERR_NONE);
}


uv_errors_e uv_eeprom_read(unsigned char *dest, uint16_t len, uint16_t eeprom_addr) {
	if (eeprom_addr + len > _UV_EEPROM_SIZE) {
		return uv_err(ERR_NOT_ENOUGH_MEMORY | HAL_MODULE_EEPROM);
	}
	// set the reading address
	LPC_EEPROM->ADDR = eeprom_addr;
	LPC_EEPROM->INT_CLR_STATUS = (1 << 26);
	// read the data
	uint16_t i;
	for (i = 0; i < len; i++) {
		LPC_EEPROM->CMD = 0;
		while (!(LPC_EEPROM->INT_STATUS & (1 << 26)));
		*dest++ = LPC_EEPROM->RDATA;
		LPC_EEPROM->INT_CLR_STATUS = (1 << 26);
	}

	return uv_err(ERR_NONE);
}



#endif


