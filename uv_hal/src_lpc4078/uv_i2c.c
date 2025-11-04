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
	Chip_I2C_SetMasterEventHandler(I2C0, Chip_I2C_EventHandlerPolling);

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
	Chip_I2C_SetMasterEventHandler(I2C1, Chip_I2C_EventHandlerPolling);
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
	Chip_I2C_SetMasterEventHandler(I2C2, Chip_I2C_EventHandlerPolling);
#endif

	return ret;
}


uv_errors_e uv_i2cm_read(i2c_e channel, uint8_t *tx_buffer, uint16_t tx_len,
		uint8_t *rx_buffer, uint16_t rx_len) {
	uv_errors_e ret = ERR_NONE;

	while (Chip_I2C_IsMasterActive(channel)) {
		uv_rtos_task_yield();
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
