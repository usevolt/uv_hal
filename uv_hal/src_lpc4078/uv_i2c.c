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



#include <uv_i2c.h>

#if CONFIG_I2C

#include <uv_rtos.h>
#include "uv_terminal.h"
#include "chip.h"


#if CONFIG_I2C_ASYNC


typedef struct {
	uint8_t data[CONFIG_I2C_ASYNC_MAX_BYTE_LEN];
	uint16_t data_len;
} i2c_tx_msg_st;


typedef struct {
	uv_ring_buffer_st tx;
	i2c_tx_msg_st tx_buffer[CONFIG_I2C_ASYNC_BUFFER_LEN];
} i2c_st;

static i2c_st i2c[I2C_COUNT];



static void transmit_next(i2c_e channel);
#endif
static void i2c_transfer_int_callb(uint32_t err_code, uint32_t n);


// Maximum number of no-progress poll iterations the blocking master transfer
// will spin before it gives up, resets the peripheral and aborts. This guards
// against a missing or stuck I2C slave (e.g. an unpopulated RTC) hanging the
// calling task – and thus the whole system – forever. The value only needs to
// be far larger than the idle-poll count of a legitimate byte transfer
// (a few thousand at normal baudrates), which it is by a wide margin.
#if !defined(CONFIG_I2C_POLL_TIMEOUT)
#define CONFIG_I2C_POLL_TIMEOUT			1000000
#endif


// Returns the register block for the given I2C channel.
static LPC_I2C_T *i2c_get_regs(i2c_e channel) {
	LPC_I2C_T *ret;
	if (channel == I2C1) {
		ret = LPC_I2C1;
	}
	else if (channel == I2C2) {
		ret = LPC_I2C2;
	}
	else {
		ret = LPC_I2C0;
	}
	return ret;
}


// Disables and re-enables the I2C peripheral. Toggling I2EN resets the internal
// state machine and clears a stuck STOP condition, releasing the bus so the
// waiting loops (both the poll handler and Chip_I2C_MasterTransfer's bus-free
// wait) can proceed after an aborted transfer.
static void i2c_bus_reset(i2c_e channel) {
	LPC_I2C_T *pI2C = i2c_get_regs(channel);
	pI2C->CONCLR = I2C_CON_I2EN | I2C_CON_STA | I2C_CON_STO |
			I2C_CON_SI | I2C_CON_AA;
	pI2C->CONSET = I2C_CON_I2EN;
}


// Set when the chip driver signals a finished (or NAK'd / errored) transfer.
static volatile bool i2c_xfer_done[I2C_NUM_INTERFACE];


// Custom polling master event handler. Mirrors Chip_I2C_EventHandlerPolling but
// bails out after CONFIG_I2C_POLL_TIMEOUT iterations with no bus state change,
// so an absent/stuck slave cannot block the transfer forever. On timeout it
// resets the peripheral to free the bus and aborts the transfer.
static void i2c_master_event_handler(I2C_ID_T id, I2C_EVENT_T event) {
	if (event == I2C_EVENT_DONE) {
		i2c_xfer_done[id] = true;
	}
	else if (event == I2C_EVENT_WAIT) {
		i2c_xfer_done[id] = false;
		uint32_t idle = 0;
		while (!i2c_xfer_done[id]) {
			if (Chip_I2C_IsStateChanged(id)) {
				// bus made progress: service the state machine
				Chip_I2C_MasterStateHandler(id);
				idle = 0;
			}
			else {
				idle++;
				if (idle >= CONFIG_I2C_POLL_TIMEOUT) {
					// slave missing / bus stuck: give up and recover
					i2c_bus_reset(id);
					i2c_xfer_done[id] = true;
				}
				else {
					// keep polling
				}
			}
		}
	}
	else {
		// I2C_EVENT_LOCK / I2C_EVENT_UNLOCK are not used in polling mode
	}
}





uv_errors_e _uv_i2c_init(void) {
	uv_errors_e ret = ERR_NONE;

#if CONFIG_I2C0
#if CONFIG_I2C0_SDA_IO == P0_27
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 27, FUNC1 | MD_HS_ENA);
#elif CONFIG_I2C0_SDA_IO == P1_30
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 30, FUNC4 | MD_HS_ENA | MD_ANA_DIS | MD_OD_ENA);
#elif CONFIG_I2C0_SDA_IO == P5_2
#if CONFIG_I2C0_BAUDRATE <= 40000
	Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 2, FUNC5 | MD_HS_ENA);
#else
	Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 2, FUNC5 | MD_HS_ENA | MD_HD_ENA);
#endif
#else
#error "CONFIG_I2C0_SDA_IO doesnt define suitable IO pin"
#endif
#if CONFIG_I2C0_SCL_IO == P1_31
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 31, FUNC4 | MD_HS_ENA | MD_ANA_DIS | MD_OD_ENA);
#elif CONFIG_I2C0_SCL_IO == P0_28
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 28, FUNC1 | MD_HS_ENA);
#elif CONFIG_I2C0_SCL_IO == P5_3
#if CONFIG_I2C0_BAUDRATE <= 40000
	Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 3, FUNC5 | MD_HS_ENA);
#else
	Chip_IOCON_PinMuxSet(LPC_IOCON, 5, 3, FUNC5 | MD_HS_ENA | MD_HD_ENA);
#endif
#else
#error "CONFIG_I2C0_SCL_IO doesnt define suitable IO pin"
#endif
	Chip_I2C_Init(I2C0);
	Chip_I2C_SetClockRate(I2C0, CONFIG_I2C0_BAUDRATE);
	Chip_I2C_SetMasterEventHandler(I2C0, i2c_master_event_handler);

#endif



#if CONFIG_I2C1
#if CONFIG_I2C1_SDA_IO == P0_0
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 0, FUNC3 | MD_HS_ENA | MD_OD_ENA);
#elif CONFIG_I2C1_SDA_IO == P1_19
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 19, FUNC3 | MD_HS_ENA | MD_OD_ENA);
#elif CONFIG_I2C1_SDA_IO == P2_14
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 14, FUNC2 | MD_HS_ENA | MD_OD_ENA);
#else
#error "CONFIG_I2C1_SDA_IO doesnt define suitable IO pin"
#endif
#if CONFIG_I2C1_SCL_IO == P0_1
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, FUNC3 | MD_HS_ENA | MD_OD_ENA);
#elif CONFIG_I2C1_SCL_IO == P0_20
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 20, FUNC3 | MD_HS_ENA | MD_OD_ENA);
#elif CONFIG_I2C1_SCL_IO == P2_15
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 15, FUNC2 | MD_HS_ENA | MD_OD_ENA);
#else
#error "CONFIG_I2C1_SCL_IO doesnt define suitable IO pin"
#endif
	Chip_I2C_Init(I2C1);
	Chip_I2C_SetClockRate(I2C1, CONFIG_I2C1_BAUDRATE);
	Chip_I2C_SetMasterEventHandler(I2C1, i2c_master_event_handler);
#endif


#if CONFIG_I2C2
#if CONFIG_I2C2_SDA_IO == P0_10
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 10, FUNC2 | MD_HS_ENA | MD_OD_ENA);
#elif CONFIG_I2C2_SDA_IO == P1_15
	Chip_IOCON_PinMuxSet(LPC_IOCON, 1, 15, FUNC3 | MD_HS_ENA | MD_OD_ENA);
#elif CONFIG_I2C2_SDA_IO == P2_30
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 30, FUNC2 | MD_HS_ENA | MD_OD_ENA);
#else
#error "CONFIG_I2C2_SDA_IO doesnt define suitable IO pin"
#endif
#if CONFIG_I2C2_SCL_IO == P0_11
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 11, FUNC2 | MD_HS_ENA | MD_OD_ENA);
#elif CONFIG_I2C2_SCL_IO == P2_31
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 31, FUNC2 | MD_HS_ENA | MD_OD_ENA);
#elif CONFIG_I2C2_SCL_IO == P4_21
	Chip_IOCON_PinMuxSet(LPC_IOCON, 4, 21, FUNC2 | MD_HS_ENA | MD_OD_ENA);
#else
#error "CONFIG_I2C2_SCL_IO doesnt define suitable IO pin"
#endif
	Chip_I2C_Init(I2C1);
	Chip_I2C_SetClockRate(I2C1, CONFIG_I2C2_BAUDRATE);
	Chip_I2C_SetMasterEventHandler(I2C2, i2c_master_event_handler);
#endif

	return ret;
}


uv_errors_e uv_i2cm_read(i2c_e channel, uint8_t *tx_buffer, uint16_t tx_len,
		uint8_t *rx_buffer, uint16_t rx_len) {
	uv_errors_e ret = ERR_NONE;

	// wait for any ongoing transfer to finish, but don't hang forever if the
	// bus was left stuck by a missing/unresponsive slave: reset and continue
	uint32_t busywait = 0;
	while (Chip_I2C_IsMasterActive(channel)) {
		busywait++;
		if (busywait >= CONFIG_I2C_POLL_TIMEOUT) {
			i2c_bus_reset(channel);
			break;
		}
		else {
			uv_rtos_task_yield();
		}
	}
	I2C_XFER_T xfer = {};
	if (tx_len) {
		// slave address is read from txbuffer first byte
		// first bit includes R/W bit that is automatically appended
		xfer.slaveAddr = ((tx_buffer[0]) >> 1);
		// first byte should be reserved for the slave address
		if (rx_len) {
			rx_len--;
			rx_buffer = &rx_buffer[1];
		}
		xfer.rxSz = rx_len;
		xfer.rxBuff = rx_buffer;
		xfer.txSz = tx_len - 1;
		xfer.txBuff = &tx_buffer[1];
	}
	else {
		// slave address is read from rx_buffer first byte
		// first bit includes R/W bit that is automatically appended
		xfer.slaveAddr = ((rx_buffer[0]) >> 1);
		xfer.rxSz = rx_len - 1;
		xfer.rxBuff = &rx_buffer[1];
		xfer.txSz = 0;
		xfer.txBuff = NULL;
	}
	while (Chip_I2C_MasterTransfer(channel, &xfer) == I2C_STATUS_ARBLOST) {}


	return ret;
}



uv_errors_e uv_i2cm_write(i2c_e channel, uint8_t *tx_buffer, uint16_t tx_len) {
	uv_errors_e ret = ERR_NONE;

	if (tx_len > 1) {
		I2C_XFER_T xfer = {};
		xfer.slaveAddr = ((tx_buffer[0]) >> 1);
		xfer.txBuff = tx_buffer + 1;
		xfer.txSz = tx_len - 1;
		while (Chip_I2C_MasterTransfer(channel, &xfer) == I2C_STATUS_ARBLOST) {}
	}
	else {
		ret = ERR_ABORTED;
	}

	return ret;
}




static void i2c_transfer_int_callb(uint32_t err_code, uint32_t n) {
#if CONFIG_I2C_ASYNC
	transmit_next(I2C0);
#endif
}



#if CONFIG_I2C_ASYNC


static i2c_tx_msg_st txmsg;
static void transmit_next(i2c_e channel) {
#warning "ASYNC mode not yet implemented on LPC4078"
	if (LPC_I2CD_API->i2c_get_status(i2c_handle_master) == IDLE) {
		uv_disable_int();
		if (uv_ring_buffer_pop(&i2c[channel].tx, &txmsg) == ERR_NONE) {
			param.stop_flag = 1;
			param.func_pt = &i2c_transfer_int_callb;
			param.num_bytes_rec = 0;
			param.num_bytes_send = txmsg.data_len;
			param.buffer_ptr_rec = NULL;
			param.buffer_ptr_send = txmsg.data;

			LPC_I2CD_API->i2c_master_transmit_intr(
					i2c_handle_master, &param, &res);
		}
		uv_enable_int();
	}
}



uv_errors_e uv_i2cm_write_async(i2c_e channel,
		uint8_t *tx_buffer, uint16_t tx_len) {
#warning "ASYNC mode not yet implemented on LPC4078"
	uv_can_errors_e ret = ERR_NONE;
	i2c_tx_msg_st msg;

	msg.data_len = tx_len;
	memcpy(msg.data, tx_buffer, tx_len);

	uv_disable_int();
	ret = uv_ring_buffer_push(&i2c[channel].tx, &msg);
	uv_enable_int();

	transmit_next(channel);

	return ret;
}


#endif



#endif
