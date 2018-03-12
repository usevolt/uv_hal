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


#include "uv_eeprom.h"
#include "uv_rtos.h"

#if CONFIG_EEPROM

#if CONFIG_TARGET_LPC1549
#include "chip.h"
#include "eeprom.h"
#include "iap.h"
#endif


typedef struct {
	// address for the newest data
	int16_t back_addr;
	// address for the oldest data
	int16_t front_addr;
	// size of the entry in bytes plus 2 bytes (index field)
	uint16_t entry_len;
} this_st;

static this_st _this;
#define this (&_this)

uv_errors_e _uv_eeprom_init(void) {
#if CONFIG_TARGET_LPC1785
	// enable power
	LPC_EEPROM->PWRDWN = 0;

	// set clock frequency to 375 kHz
	LPC_EEPROM->CLKDIV = SystemCoreClock / 375000 - 1;

	// set the required timing register values
	LPC_EEPROM->WSTATE = (unsigned long int) 15 * SystemCoreClock / 1000000000 |	// min 15 ns
			(((unsigned long int) 55 * SystemCoreClock / 1000000000) << 8) |		// min 55 ns
			(((unsigned long int) 35 * SystemCoreClock / 1000000000) << 16);		// min 35 ns

	// clear eeprom int bits. For some reason leaving these may cause IAP programming to
	// end with a BUSY error code
	LPC_EEPROM->INT_CLR_STATUS = ((1 << 26) | (1 << 28));

#elif CONFIG_TARGET_LPC1549
	Chip_SYSCTL_PowerUp(SYSCTL_POWERDOWN_EEPROM_PD);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_EEPROM);
	Chip_SYSCTL_PeriphReset(RESET_EEPROM);

#endif
	return ERR_NONE;
}


uv_errors_e uv_eeprom_write(const void *data, uint16_t len, uint16_t eeprom_addr) {
	uv_errors_e ret = ERR_NONE;
#if CONFIG_TARGET_LPC1549
	// top 64 bytes are reserved
	eeprom_addr += 64;
#endif

	// out of EEPROM memory
	if (eeprom_addr + len > _UV_EEPROM_SIZE) {
		ret = ERR_NOT_ENOUGH_MEMORY;
	}
	else {
#if CONFIG_TARGET_LPC1785

		__disable_irq();

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
			LPC_EEPROM->WDATA = ((const uint8_t*)data)[i];
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
		// clear eeprom int bits. For some reason leaving these may cause IAP programming to
		// end with a BUSY error code
		LPC_EEPROM->INT_CLR_STATUS = ((1 << 26) | (1 << 28));

		__enable_irq();

#elif CONFIG_TARGET_LPC1549
		if (Chip_EEPROM_Write(eeprom_addr, (void*) data, len) != IAP_CMD_SUCCESS) {
			ret = ERR_HARDWARE_NOT_SUPPORTED;
		}
#endif
	}
	return ret;
}


uv_errors_e uv_eeprom_read(void *dest, uint16_t len, uint16_t eeprom_addr) {
	uv_errors_e ret = ERR_NONE;
#if CONFIG_TARGET_LPC1549
	// top 64 bytes are reserved
	eeprom_addr += 64;
#endif

	if (eeprom_addr + len > _UV_EEPROM_SIZE) {
		ret = ERR_NOT_ENOUGH_MEMORY;
	}
	else {

#if CONFIG_TARGET_LPC1785

		__disable_irq();
		// set the reading address
		LPC_EEPROM->ADDR = eeprom_addr;
		LPC_EEPROM->INT_CLR_STATUS = (1 << 26);
		// read the data
		uint16_t i;
		for (i = 0; i < len; i++) {
			LPC_EEPROM->CMD = 0;
			while (!(LPC_EEPROM->INT_STATUS & (1 << 26))) {
				uv_rtos_task_yield();
			}
			*dest++ = LPC_EEPROM->RDATA;
			LPC_EEPROM->INT_CLR_STATUS = (1 << 26);
		}

		// clear eeprom int bits. For some reason leaving these may cause IAP programming to
		// end with a BUSY error code
		LPC_EEPROM->INT_CLR_STATUS = ((1 << 26) | (1 << 28));

		__enable_irq();

#elif CONFIG_TARGET_LPC1549
		if (Chip_EEPROM_Read(eeprom_addr, dest, len) != IAP_CMD_SUCCESS) {
			ret = ERR_HARDWARE_NOT_SUPPORTED;
		}
#endif
	}
	return ret;
}


#if CONFIG_EEPROM_RING_BUFFER

void uv_eeprom_init_circular_buffer(const uint16_t entry_len) {
	uint16_t i;
	uint16_t min_index = -1;
	uint16_t max_index = 1;

	this->entry_len = entry_len + sizeof(uint16_t);

	this->back_addr = this->front_addr = 0;
	uint16_t index;
	for (i = 0; i <= CONFIG_EEPROM_RING_BUFFER_END_ADDR - this->entry_len; i += this->entry_len) {
		uv_eeprom_read((unsigned char*) &index, sizeof(uint16_t), i + this->entry_len - sizeof(uint16_t));
		if (index == 0) {
			continue;
		}
		// smallest index is at the front
		if (index < min_index) {
			this->front_addr = i;
			min_index = index;
		}
		// biggest index is at the back
		if (index > max_index) {
			this->back_addr = i;
			max_index = index;
		}
	}

}


uv_errors_e uv_eeprom_push_back(const void *src) {
	uv_errors_e ret = ERR_NONE;

	// check for overflow
	int16_t t = this->back_addr + this->entry_len;
	if (t + this->entry_len > CONFIG_EEPROM_RING_BUFFER_END_ADDR) {
		t = 0;
	}
	if (t == this->front_addr) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {

		uint16_t index;
		// read the last index number
		uv_eeprom_read((unsigned char*) &index, sizeof(uint16_t),
				this->back_addr + this->entry_len - sizeof(uint16_t));

		// if index number was zero, first data was empty
		if (index) {
			this->back_addr += this->entry_len;
		}
		if (this->back_addr + this->entry_len > CONFIG_EEPROM_RING_BUFFER_END_ADDR) {
			this->back_addr = 0;
		}
		uv_eeprom_write(src, this->entry_len - sizeof(uint16_t), this->back_addr);

		// check that index number cannot overflow
		if (index == 0xFFFF) {
			// there is a possibility that the index number could overflow.
			// cycle trough all entries and reduce their index numbers
			uint16_t i;
			uint16_t new_index;
			for (i = 0; i <= CONFIG_EEPROM_RING_BUFFER_END_ADDR - this->entry_len; i += this->entry_len) {
				uv_eeprom_read((unsigned char*) &new_index,  sizeof(uint16_t),
						i + this->entry_len - sizeof(uint16_t));
				if (new_index) {
					new_index -= (0xFFFF - CONFIG_EEPROM_RING_BUFFER_END_ADDR);
				}
				uv_eeprom_write((unsigned char *) &new_index, sizeof(uint16_t),
						i + this->entry_len - sizeof(uint16_t));
			}
		}

		// increase the index and write it at the end of the data
		index++;
		uv_eeprom_write((unsigned char*) &index, sizeof(uint16_t),
				this->back_addr + this->entry_len - sizeof(uint16_t));
	}

	return ret;
}


uv_errors_e uv_eeprom_pop_back(void *dest) {
	uv_errors_e ret = ERR_NONE;
	uint8_t d = 0;
	uint16_t i;
	uv_eeprom_read((unsigned char *) &i, sizeof(uint16_t),
			this->back_addr + this->entry_len - sizeof(uint16_t));
	if (i == 0) {
		ret = ERR_BUFFER_EMPTY;
	}
	else {
		if (dest) {
			uv_eeprom_read(dest, this->entry_len - sizeof(uint16_t), this->back_addr);
		}
		for (i = 0; i < this->entry_len; i++) {
			uv_eeprom_write(&d, 1, this->back_addr + i);
		}
		if (this->back_addr != this->front_addr) {
			this->back_addr = (this->back_addr - this->entry_len);
			if (this->back_addr < 0) {
				this->back_addr = CONFIG_EEPROM_RING_BUFFER_END_ADDR -
						this->entry_len - (CONFIG_EEPROM_RING_BUFFER_END_ADDR % this->entry_len);
			}
		}
	}

	return ret;
}



uv_errors_e uv_eeprom_pop_front(void *dest) {
	uv_errors_e ret = ERR_NONE;
	uint8_t d = 0;
	uint16_t i;
	uv_eeprom_read((unsigned char *) &i, sizeof(uint16_t),
			this->front_addr + this->entry_len - sizeof(uint16_t));
	if (i == 0) {
		ret = ERR_BUFFER_EMPTY;
	}
	else {
		if (dest) {
			uv_eeprom_read(dest, this->entry_len - sizeof(uint16_t), this->front_addr);
		}
		for (i = 0; i < this->entry_len; i++) {
			uv_eeprom_write(&d, 1, this->front_addr + i);
		}
		if (this->back_addr != this->front_addr) {
			this->front_addr += this->entry_len;
			if (this->front_addr + this->entry_len > CONFIG_EEPROM_RING_BUFFER_END_ADDR) {
				this->front_addr = 0;
			}
		}
	}

	return ret;
}


uv_errors_e uv_eeprom_at(void *dest, uint16_t *eeprom_addr, uint16_t index) {
	uv_errors_e ret = ERR_NONE;
	uint16_t i;

	uv_eeprom_read((unsigned char *) &i, sizeof(uint16_t),
			this->back_addr + this->entry_len - sizeof(uint16_t));
	if (!i) {
		ret = ERR_BUFFER_EMPTY;
	}
	else {
		int16_t addr = this->back_addr;
		bool err = false;
		for (i = index; i > 0; i--) {
			if (addr == this->front_addr) {
				ret = ERR_BUFFER_OVERFLOW;
				err = true;
				break;
			}
			else {
				addr -= this->entry_len;
				if (addr < 0) {
					addr = CONFIG_EEPROM_RING_BUFFER_END_ADDR -
							(CONFIG_EEPROM_RING_BUFFER_END_ADDR % this->entry_len);
				}
			}
		}

		if (!err) {
			if (dest) {
				uv_eeprom_read(dest, this->entry_len - sizeof(uint16_t), addr);
			}
			if (eeprom_addr) {
				*eeprom_addr = addr;
			}
		}
	}

	return ret;
}

#endif


void uv_eeprom_clear(void) {
	uint32_t i, p = 0;
	for (i = 0; i < uv_eeprom_size() / sizeof(p); i++) {
		uv_eeprom_write((unsigned char *) &p, sizeof(p), i * sizeof(p));
	}
	this->back_addr = 0;
	this->front_addr = 0;
}



#endif


