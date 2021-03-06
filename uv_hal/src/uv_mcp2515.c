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



#include "uv_mcp2515.h"
#include <string.h>
#include <uv_rtos.h>


#if CONFIG_MCP2515


#define CMD_RESET		0b11000000
#define CMD_READ		0b00000011
#define CMD_READ_RX		0b10010000
#define CMD_WRITE		0b00000010
#define CMD_LOAD_TX		0b01000000
#define CMD_RTS			0b10000000
#define CMD_READ_STATUS	0b10100000
#define CMD_RX_STATUS	0b10110000
#define CMD_BIT_MODIFY	0b00000101

// for register map, refer to http://ww1.microchip.com/downloads/en/DeviceDoc/20001801H.pdf, page 63

static void task(void *ptr);
static inline void handle_int(uv_mcp2515_st *this);

int32_t uv_mcp2515_init(uv_mcp2515_st *this, spi_e spi, spi_slaves_e ssel,
		uv_gpios_e int_gpio, uint32_t can_baudrate) {
	int32_t ret = 1;
	this->spi = spi;
	this->ssel = ssel;
	this->int_gpio = int_gpio;
	this->can_baudrate = can_baudrate;
	uv_queue_init(&this->rx_queue, CONFIG_MCP2515_RX_BUFFER_LEN, sizeof(uv_can_msg_st));
	uv_queue_init(&this->tx_queue, CONFIG_MCP2515_TX_BUFFER_LEN, sizeof(uv_can_msg_st));
	uv_mutex_init(&this->int_mutex);
	// lock interrupt mutex. Int mutex is unlocked only when there's
	// interrupt pending in the mcp2515.
	uv_mutex_lock(&this->int_mutex);

	// send reset command
	uint16_t write[10] = {}, read[10] = {};
	write[0] = CMD_RESET;
	uv_spi_write_sync(this->spi, this->ssel, write, 8, 1);

	uv_rtos_task_delay(20);

	// read CAN control register and compare it to the default value
	write[0] = CMD_READ;
	write[1] = 0x0F;
	memset(read, 0, sizeof(read));
	uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 3);
	if (read[2] == 0x87) {
		// mcp2515 found
		ret = 0;
		// configure can baudrate
		uint8_t sjw = 0;
		uint8_t prop_seg = 2;
		uint8_t tseg1 = 2;
		uint8_t tseg2 = 3;
		uint8_t brp = (uint32_t) CONFIG_MCP2515_FOSC /
				(2 * (this->can_baudrate * (prop_seg + tseg1 + tseg2 + 1)));
		write[0] = CMD_WRITE;
		write[1] = 0x2A;
		write[2] = (sjw << 6) | (brp & 0x3F);
		write[3] = (1 << 7) | (((tseg1 - 1) & 0x7) << 3) | ((prop_seg - 1) & 0x7);
		write[4] = (tseg2 - 1) & 0x7;
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 5);

		// disable txrts pins
		write[0] = CMD_WRITE;
		write[1] = 0x0D;
		write[2] = 0;
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 3);

		// configure rxobjects to receive all messages

		// rxbuffer0 config
		write[0] = CMD_WRITE;
		write[1] = 0x60;
		write[2] = 0;
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 3);

		// disable rxbnf pins
		write[0] = CMD_WRITE;
		write[1] = 0x0C;
		write[2] = 0;
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 3);

		// rxbuffer1 config
		write[0] = CMD_WRITE;
		write[1] = 0x70;
		write[2] = 0;
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 3);

		// set mask to zero (receive all messages)
		write[0] = CMD_WRITE;
		write[1] = 0x20;
		memset(&write[2], 0, 8);
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 10);

		// clear all interrupts
		write[0] = CMD_WRITE;
		write[1] = 0x2C;
		write[2] = 0;
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 3);

		// enable receive and transmit message interrupts
		write[0] = CMD_WRITE;
		write[1] = 0x2B;
		write[2] = 0x3 | (1 << 2);
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 3);


		// set up normal operation
		write[0] = CMD_WRITE;
		write[1] = 0x0F;
		write[3] = 0;
		uv_spi_write_sync(this->spi, this->ssel, write, 8, 3);


		// last things last, set gpio interrupt since MCP2515 is running
		uv_gpio_init_input(this->int_gpio, PULL_UP_ENABLED);
		uv_gpio_init_int(this->int_gpio, INT_FALLING_EDGE);

		int32_t task_ret = uv_rtos_task_create(&task, "mcp2515_", UV_RTOS_MIN_STACK_SIZE,
				this, 0xFFFFFFFF, NULL);
		ret = (task_ret == -1) ? false : ret;
	}

	return ret;
}

static void task(void *ptr) {
	uv_mcp2515_st *this = ptr;
	uint16_t step_ms = 1;
	while (true) {
		// mutex_lock works as a task delay function which triggers
		// if an interrupt is received from the mcp2515
		uv_mutex_lock_ms(&this->int_mutex, step_ms);

		// check the mcp2515 state and possibly send or receive messages
		handle_int(this);
	}
}


void uv_mcp2515_int(uv_mcp2515_st *this) {
	// unlock the mutex to indicate that interrupt is active
	uv_mutex_unlock_isr(&this->int_mutex);
}


static inline void handle_int(uv_mcp2515_st *this) {
	uv_can_msg_st msg;
	uint16_t write[14] = { }, read[14] = { };

	do {
		write[0] = CMD_READ_STATUS;
		write[1] = 0;
		uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 2);

		if (read[1] & (0b11)) {
			// 0 if rxbuffer zero had the message, otherwise 1
			uint8_t buffer = ((read[2] & (1 << 1)) >> 1);

			// fetch the message
			write[0] = CMD_READ_RX | (buffer << 2);
			memset(&write[1], 0, 13 * sizeof(write[1]));
			uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 14);

			msg.type = (read[2] & (1 << 4)) ? CAN_EXT : CAN_STD;
			if (msg.type == CAN_EXT) {
				// extended identifier
				msg.id = (((uint32_t) read[1]) << 21) +
						(((uint32_t) (read[2] >> 5)) << 18) +
						(((uint32_t) (read[2] & 0b11)) << 16) +
						(((uint32_t) (read[3])) << 8) +
						read[4];
			}
			else {
				// standard identifier
				msg.id = (((uint32_t) read[1]) << 3) + (read[2] >> 5);
			}
			// DLC
			msg.data_length = read[5] & 0b1111;
			// copy data bytes
			memset(msg.data_8bit, 0, sizeof(msg.data_8bit));
			for (uint8_t i = 0; i < msg.data_length; i++) {
				msg.data_8bit[i] = read[6 + i];
			}

			// try to push the received message to the queue
			uv_queue_push(&this->rx_queue, &msg, 5);
		}

		// clear txif if set
		if (read[1] & (1 << 3)) {
			// clear txif flag bit
			write[0] = CMD_BIT_MODIFY;
			write[1] = 0x2C;
			write[2] = (1 << 2);
			write[3] = 0;
			uv_spi_write_sync(this->spi, this->ssel, write, 8, 4);
		}

		// check if txbuffer0 is not transmitting a message at the moment
		if (!(read[1] & (1 << 2))) {

			// check if there's any new messages to be sent
			if (uv_queue_pop(&this->tx_queue, &msg, 0) == ERR_NONE) {
				// send the message

				write[0] = CMD_LOAD_TX;
				if (msg.type == CAN_STD) {
					// standard frame
					write[1] = (msg.id >> 3) & 0xFF;
					write[2] = ((msg.id & 0x7) << 5);
					write[3] = 0;
					write[4] = 0;
				}
				else {
					// extended frame
					write[1] = (msg.id >> 21) & 0xFF;
					write[2] = (((msg.id >> 18) & 0x7) << 5) |
							(1 << 3) |
							((msg.id >> 16) & 0x3);
					write[3] = ((msg.id >> 8) & 0xFF);
					write[4] = (msg.id & 0xFF);
				}
				if (msg.data_length > 8) {
					msg.data_length = 8;
				}
				write[5] = msg.data_length;
				for (uint8_t i = 0; i < msg.data_length; i++) {
					write[6 + i] = msg.data_8bit[i];
				}

				uv_spi_write_sync(this->spi, this->ssel, write, 8, 6 + msg.data_length);

				// request to send the first transmit buffer
				write[0] = CMD_RTS | 1;
				uv_spi_write_sync(this->spi, this->ssel, write, 8, 1);
			}
		}
	} while (!uv_gpio_get(this->int_gpio));
}






uv_can_errors_e uv_mcp2515_get_error_state(uv_mcp2515_st *this) {
	uint16_t write[3] = {}, read[sizeof(write)] = {};
	write[0] = CMD_READ;
	write[1] = 0x2D;
	uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 3);

	uv_can_errors_e ret;

	if (read[2] & (1 << 5)) {
		ret = CAN_ERROR_BUS_OFF;
	}
	else if (read[2] & (0b11 << 3)) {
		ret = CAN_ERROR_PASSIVE;
	}
	else {
		ret = CAN_ERROR_ACTIVE;
	}
	write[0] = CMD_READ;
	write[1] = 0x0F;
	uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 3);

	return ret;
}



#endif

