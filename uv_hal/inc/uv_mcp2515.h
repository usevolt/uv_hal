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


#ifndef HAL_UV_HAL_INC_UV_MCP2515_H_
#define HAL_UV_HAL_INC_UV_MCP2515_H_

#include <uv_hal_config.h>
#include "uv_spi.h"
#include "uv_can.h"
#include "uv_rtos.h"

#if CONFIG_MCP2515

/// @file: MCP2515 is an SPI-CAN interface module


#if !CONFIG_SPI
#error "At least one SPI module has to be enabled for MCP2515."
#endif
#if !CONFIG_MCP2515_RX_BUFFER_LEN
#error "CONFIG_MCP2515_RX_BUFFER_LEN should define the receive buffer length\
 in CAN messages."
#endif
#if !CONFIG_MCP2515_TX_BUFFER_LEN
#error "CONFIG_MCP2515_TX_BUFFER_LEN should define the transmit buffer length\
 in CAN messages."
#endif
#if !CONFIG_MCP2515_FOSC
#error "CONFIG_MCP2515_FOSC should define the crystal oscillator frequency in Hz which is soldered to the pcb."
#endif



typedef struct {

	// SPI channel for this node
	spi_e spi;
	// SPI slave select for this node
	spi_slaves_e ssel;
	// interrupt gpio
	uv_gpios_e int_gpio;
	// receive buffer
	uv_can_msg_st rx_buffer[CONFIG_MCP2515_RX_BUFFER_LEN];
	uv_ring_buffer_st rx;
	// transmit buffer
	uv_can_msg_st tx_buffer[CONFIG_MCP2515_TX_BUFFER_LEN];
	uv_ring_buffer_st tx;

	// Stores the CAN baudrate set for the MCP2515
	uint32_t can_baudrate;

	// mutex for accessing the SPI bus
	uv_mutex_st mutex;

} uv_mcp2515_st;



/// @brief: Initializes and resets the mcp2515 ic
///
/// @return: 0 if initialized succesfully, otherwise returns 1.
int32_t uv_mcp2515_init(uv_mcp2515_st *this, spi_e spi, spi_slaves_e ssel,
		uv_gpios_e int_gpio, uint32_t can_baudrate);



/// @brief: Interrupt handler. This should be called when the gpio interrupt happens
void uv_mcp2515_int(uv_mcp2515_st *this, bool from_isr);


uv_can_errors_e uv_mcp2515_get_error_state(uv_mcp2515_st *this);


/// @brief: Sends a CAN message
///
/// @return ERR_NONE when sent succesfully, error otherwise describing the problem.
uv_errors_e uv_mcp2515_send(uv_mcp2515_st *this, uv_can_msg_st *msg);



/// @brief: Pops a received message from the receive buffer. Also works as a step function.
///
/// @return ERR_NONE if messages were to receive, error if buffer was empty.
uv_errors_e uv_mcp2515_receive(uv_mcp2515_st *this, uv_can_msg_st *dest);




#endif


#endif /* HAL_UV_HAL_INC_UV_MCP2515_H_ */
