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


#include "uv_eeprom.h"
#include "uv_rtos.h"

#if CONFIG_EEPROM


static struct {
	// address for the newest data
	int16_t back_addr;
	// address for the oldest data
	int16_t front_addr;
	// size of the entry in bytes plus 2 bytes (index field)
	uint16_t entry_len;
	char filepath[128];
} _this = {
		.filepath = "./" STRINGIFY(__UV_PROJECT_NAME) ".eeprom"
};
#undef this
#define this (&_this)


uv_errors_e _uv_eeprom_init() {
	uv_errors_e ret = ERR_NONE;

	return ret;
}


uv_errors_e uv_eeprom_write(const void *data, uint16_t len, uint16_t eeprom_addr) {
	uv_errors_e ret = ERR_NONE;

	// open the file and write
	FILE *file = fopen(this->filepath, "r+b");
	if (file != NULL) {
		// check that the file size is at least the size of EEPROM memory
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		if (size < _UV_EEPROM_SIZE) {
			int s = _UV_EEPROM_SIZE - size;
			uint8_t *d = malloc(s);
			memset(d, 0, s);
			fwrite(d, 1, sizeof(d), file);
			free(d);
		}
	}
	else {
		// the file might not exists. Try to create one
		file = fopen(this->filepath, "w+b");
		// initialize the eeprom to zero
		uint8_t d[_UV_EEPROM_SIZE] = {};
		fwrite(d, 1, sizeof(d), file);
	}
	if (file == NULL) {
		printf("Opening the EEPROM file '%s' failed\n", this->filepath);
		ret = ERR_INTERNAL;
	}
	else {
		fseek(file, eeprom_addr, SEEK_SET);
		fwrite(data, 1, len, file);
		fclose(file);
	}

	return ret;
}


uv_errors_e uv_eeprom_read(void *dest, uint16_t len, uint16_t eeprom_addr) {
	uv_errors_e ret = ERR_NONE;

	// open the file and write
	FILE *file = fopen(this->filepath, "r+b");
	if (file != NULL) {
		// check that the file size is at least the size of EEPROM memory
		fseek(file, 0, SEEK_END);
		int size = ftell(file);
		if (size < _UV_EEPROM_SIZE) {
			int s = _UV_EEPROM_SIZE - size;
			uint8_t *d = malloc(s);
			memset(d, 0, s);
			fwrite(d, 1, sizeof(d), file);
			free(d);
		}
	}
	else {
		// the file might not exists. Try to create one
		file = fopen(this->filepath, "w+b");
		// initialize the eeprom to zero
		uint8_t d[_UV_EEPROM_SIZE] = {};
		fwrite(d, 1, sizeof(d), file);
	}
	if (file == NULL) {
		printf("Opening the EEPROM file '%s' failed\n", this->filepath);
		ret = ERR_INTERNAL;
	}
	else {
		fseek(file, eeprom_addr, SEEK_SET);
		if (fread(dest, 1, len, file) != len) {
			ret = ERR_NOT_ENOUGH_MEMORY;
		}
		fclose(file);
	}

	return ret;
}


void uv_eeprom_set_filepath(char *filepath) {
	strcpy(this->filepath, filepath);
}


/// @brief: Returns the nonvolatile memory file name
char *uv_eeprom_get_filepath(void) {
	return this->filepath;
}


#if CONFIG_EEPROM_RING_BUFFER



#define RING_BUFFER_LEN		(CONFIG_EEPROM_RING_BUFFER_END_ADDR - \
								CONFIG_EEPROM_RING_BUFFER_START_ADDR)


void uv_eeprom_ring_buffer_init(const uint16_t entry_len) {
	uint16_t i;
	uint16_t min_index = -1;
	uint16_t max_index = 1;

	this->entry_len = entry_len + sizeof(uint16_t);

	this->back_addr = this->front_addr = CONFIG_EEPROM_RING_BUFFER_START_ADDR;
	uint16_t index = 0;
	for (i = CONFIG_EEPROM_RING_BUFFER_START_ADDR;
			i <= CONFIG_EEPROM_RING_BUFFER_START_ADDR + RING_BUFFER_LEN - this->entry_len;
			i += this->entry_len) {
		uv_eeprom_read((unsigned char*) &index, sizeof(uint16_t),
				i + this->entry_len - sizeof(uint16_t));
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


uv_errors_e uv_eeprom_ring_buffer_push_back(const void *src) {
	uv_errors_e ret = ERR_NONE;

	// check for overflow
	int16_t t = this->back_addr + this->entry_len;
	if (t + this->entry_len > CONFIG_EEPROM_RING_BUFFER_END_ADDR) {
		t = CONFIG_EEPROM_RING_BUFFER_START_ADDR;
	}
	if (t == this->front_addr) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {

		uint16_t index = 0;
		// read the last index number
		uv_eeprom_read((unsigned char*) &index, sizeof(uint16_t),
				this->back_addr + this->entry_len - sizeof(uint16_t));

		// if index number was zero, first data was empty
		if (index) {
			this->back_addr += this->entry_len;
		}
		if (this->back_addr + this->entry_len > CONFIG_EEPROM_RING_BUFFER_END_ADDR) {
			this->back_addr = CONFIG_EEPROM_RING_BUFFER_START_ADDR;
		}
		uv_eeprom_write(src, this->entry_len - sizeof(uint16_t), this->back_addr);

		// check that index number cannot overflow
		if (index == 0xFFFF) {
			// there is a possibility that the index number could overflow.
			// cycle trough all entries and reduce their index numbers
			uint16_t i;
			uint16_t new_index = 0;
			for (i = CONFIG_EEPROM_RING_BUFFER_START_ADDR;
					i <= CONFIG_EEPROM_RING_BUFFER_END_ADDR - this->entry_len;
					i += this->entry_len) {
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


uv_errors_e uv_eeprom_ring_buffer_push_back_force(const void *src) {
	uv_errors_e ret = ERR_NONE;
	if ((ret = uv_eeprom_ring_buffer_push_back(src)) == ERR_BUFFER_OVERFLOW) {
		uint8_t d[this->entry_len];
		ret = uv_eeprom_ring_buffer_pop_front(d);
		if (ret == ERR_NONE) {
			ret = uv_eeprom_ring_buffer_push_back(src);
		}
	}
	return ret;
}



uv_errors_e uv_eeprom_ring_buffer_pop_back(void *dest) {
	uv_errors_e ret = ERR_NONE;
	uint8_t d = 0;
	uint16_t i = 0;
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
			if (this->back_addr < CONFIG_EEPROM_RING_BUFFER_START_ADDR) {
				this->back_addr = CONFIG_EEPROM_RING_BUFFER_END_ADDR -
						this->entry_len - (CONFIG_EEPROM_RING_BUFFER_END_ADDR % this->entry_len);
			}
		}
	}

	return ret;
}



uv_errors_e uv_eeprom_ring_buffer_pop_front(void *dest) {
	uv_errors_e ret = ERR_NONE;
	uint8_t d = 0;
	uint16_t i = 0;
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
				this->front_addr = CONFIG_EEPROM_RING_BUFFER_START_ADDR;
			}
		}
	}

	return ret;
}


uv_errors_e uv_eeprom_ring_buffer_at(void *dest, uint16_t *eeprom_addr, uint16_t index) {
	uv_errors_e ret = ERR_NONE;
	uint16_t i = 0;

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
				if (addr < CONFIG_EEPROM_RING_BUFFER_START_ADDR) {
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

uint32_t uv_eeprom_ring_buffer_get_count(void) {
	uint32_t ret = 0;
	while (uv_eeprom_ring_buffer_at(NULL, NULL, ret) == ERR_NONE) {
		ret++;
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



