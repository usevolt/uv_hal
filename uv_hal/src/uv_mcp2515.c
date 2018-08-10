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

static void send_next(uv_mcp2515_st *this, uint16_t *status);


bool uv_mcp2515_init(uv_mcp2515_st *this, spi_e spi, spi_slaves_e ssel,
		uv_gpios_e int_gpio, uint32_t can_baudrate) {
	bool ret = false;
	this->spi = spi;
	this->ssel = ssel;
	this->int_gpio = int_gpio;
	this->can_baudrate = can_baudrate;
	uv_ring_buffer_init(&this->rx, this->rx_buffer,
			CONFIG_MCP2515_RX_BUFFER_LEN, sizeof(this->rx_buffer[0]));
	uv_ring_buffer_init(&this->tx, this->tx_buffer,
			CONFIG_MCP2515_TX_BUFFER_LEN, sizeof(this->tx_buffer[0]));

	uv_mutex_init(&this->mutex);
	uv_mutex_lock(&this->mutex);

	uv_gpio_init_input(this->int_gpio, PULL_UP_ENABLED);
	uv_gpio_init_int(this->int_gpio, INT_FALLING_EDGE);

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
		ret = true;
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

		// mcp2515 should now be up and running

	}

	uv_mutex_unlock(&this->mutex);

	return ret;
}



void uv_mcp2515_int(uv_mcp2515_st *this, bool from_isr) {
	// read through all interrupts and clear them

	bool locked;
	if (from_isr) {
		locked = uv_mutex_lock_isr(&this->mutex);
	}
	else {
		uv_mutex_lock(&this->mutex);
		locked = true;
	}
	if (locked) {

		while (uv_gpio_get(this->int_gpio) == 0) {


			uint16_t write[14] = { }, read[14] = { };
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

				uv_can_msg_st msg;
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
				// copy data btes
				memset(msg.data_8bit, 0, sizeof(msg.data_8bit));
				for (uint8_t i = 0; i < msg.data_length; i++) {
					msg.data_8bit[i] = read[6 + i];
				}

				uv_ring_buffer_push(&this->rx, &msg);
			}
			// check if txbuffer0 is not transmitting a message at the moment
			if (!(read[1] & (1 << 2))) {
				// clear txif flag bit
				write[0] = CMD_BIT_MODIFY;
				write[1] = 0x2C;
				write[2] = (1 << 2);
				write[3] = 0;
				uv_spi_write_sync(this->spi, this->ssel, write, 8, 4);

				// message transmitted, ready to send a new one
				send_next(this, &read[1]);
			}
		}
		(from_isr) ? uv_mutex_unlock_isr(&this->mutex) : uv_mutex_unlock(&this->mutex);
	}
}


static void send_next(uv_mcp2515_st *this, uint16_t *status) {

	uv_can_msg_st msg;
	uv_errors_e e = uv_ring_buffer_peek(&this->tx, &msg);
	if (e == ERR_NONE) {

		uint16_t write[13] = {}, read[2] = {};
		if (status == NULL) {
			// see if mcp2515 is ready to load new message
			write[0] = CMD_READ_STATUS;
			uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 2);
		}
		else {
			read[1] = *status;
		}

		// message object is ready to accept new data
		if (!(read[1] & (1 << 2))) {

			// remove the message from ring buffer
			uv_ring_buffer_pop(&this->tx, &msg);

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

			write[0] = CMD_READ_STATUS;
			uv_spi_readwrite_sync(this->spi, this->ssel, write, read, 8, 2);
		}
	}
}


uv_errors_e uv_mcp2515_send(uv_mcp2515_st *this, uv_can_msg_st *msg) {
	uv_errors_e ret = ERR_NONE;

	uv_mutex_lock(&this->mutex);
	ret = uv_ring_buffer_push(&this->tx, msg);
	uv_mutex_unlock(&this->mutex);

	return ret;
}



uv_errors_e uv_mcp2515_receive(uv_mcp2515_st *this, uv_can_msg_st *dest) {
	uv_errors_e ret;


	// since interrupts are edge sensitive, it _might_ be possible for an edge to miss
	// an interrupt. To make sure that doesnt happen, check here int state.
	uv_mcp2515_int(this, false);

	uv_mutex_lock(&this->mutex);

	// try to send the next message
	send_next(this, NULL);

	ret = uv_ring_buffer_pop(&this->rx, dest);
	uv_mutex_unlock(&this->mutex);

	return ret;
}





#endif

