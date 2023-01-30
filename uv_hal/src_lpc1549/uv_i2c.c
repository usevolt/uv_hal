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

#include <i2cm_15xx.h>
#include <swm_15xx.h>
#include <iocon_15xx.h>
#include <uv_rtos.h>

#include "uv_terminal.h"


static LPC_I2C_T *p[I2C_COUNT] = {
		LPC_I2C
};

#if CONFIG_I2C_ASYNC


typedef struct {
	uint8_t dev_addr;
	uint8_t data_len;
	uint8_t data[CONFIG_I2C_ASYNC_MAX_BYTE_LEN];
} i2c_tx_msg_st;

typedef struct {
	uv_ring_buffer_st tx;
	i2c_tx_msg_st tx_buffer[CONFIG_I2C_ASYNC_BUFFER_LEN];
} i2c_st;

static i2c_st i2c[I2C_COUNT];
#endif



/* I2C master handle and memory for ROM API */
static I2C_HANDLE_T *i2cHandleMaster;

/* Use a buffer size larger than the expected return value of
   i2c_get_mem_size() for the static I2C handle type */
static uint32_t i2cMasterHandleMEM[0x20];


static uint32_t i2c_xfer_error;
static void I2C_XferDone(uint32_t err_code, uint32_t n)
{
	printf("xferdone\n");
	i2c_xfer_error = err_code;
}


uv_errors_e _uv_i2c_init(void) {
	// Enable I2C clock and reset I2C peripheral - the boot ROM does not do this
	Chip_I2C_Init(LPC_I2C0);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, IOCON_STDI2C_EN);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SCL);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SDA);

	uv_terminal_enable(TERMINAL_CAN);
	if (LPC_I2CD_API->i2c_get_mem_size() > sizeof(i2cMasterHandleMEM)) {
		/* Example only: this should never happen and probably isn't needed for
		   most I2C code. */
		printf("error\n");
	}

	/* Setup the I2C handle */
	i2cHandleMaster = LPC_I2CD_API->i2c_setup(LPC_I2C_BASE, i2cMasterHandleMEM);
	if (i2cHandleMaster == NULL) {
		printf("i2c_setup error\n");
	}

	/* Set I2C bitrate */
	if (LPC_I2CD_API->i2c_set_bitrate(i2cHandleMaster, Chip_Clock_GetSystemClockRate(),
									  100000) != LPC_OK) {
		printf("speed error\n");
	}

	NVIC_SetPriority(I2C0_IRQn, 1);
	NVIC_EnableIRQ(I2C0_IRQn);

//	// Setup I2CM duty cycles
//	// i2c clock should be 8 times the bus frequency
//	Chip_I2CM_SetDutyCycle(LPC_I2C0, 0x4, 0x4);
//
//	// Setup clock rate for I2C
//	// This comes directly from lpcopen examples
//	Chip_I2C_SetClockDiv(LPC_I2C0, Chip_Clock_GetMainClockRate() / (CONFIG_I2C_BAUDRATE * 8));
//	Chip_I2CM_SetBusSpeed(LPC_I2C0, CONFIG_I2C_BAUDRATE);


#if CONFIG_I2C_ASYNC
//	for (uint8_t i = 0; i < I2C_COUNT; i++) {
//		uv_ring_buffer_init(&i2c[i].tx, &i2c[i].tx_buffer,
//				CONFIG_I2C_ASYNC_BUFFER_LEN, sizeof(i2c_tx_msg_st));
//	}

//	Chip_I2C_EnableInt(LPC_I2C0,
//			I2C_INTENSET_MSTRARBLOSS |
//			I2C_INTENSET_MSTSTSTPERR);
//	NVIC_SetPriority(I2C0_IRQn, 1);
//	NVIC_ClearPendingIRQ(I2C0_IRQn);
//	NVIC_EnableIRQ(I2C0_IRQn);

#endif
//
//#if (CONFIG_I2C_MODE == I2C_MASTER)
//	// Enable Master Mode
//	Chip_I2CM_Enable(LPC_I2C0);
//#else
//#error "I2C Slave mode not yet implemented"
//#endif


	return ERR_NONE;
}


static I2CM_XFER_T xfer = {};

uv_errors_e uv_i2cm_readwrite(i2c_e channel, uint8_t dev_addr, uint8_t *tx_buffer, uint16_t tx_len,
		uint8_t *rx_buffer, uint16_t rx_len) {
	uv_errors_e ret = ERR_NONE;

//	while (!Chip_I2CM_IsMasterPending(p[channel])) {
//		uv_rtos_task_yield();
//	}
//#if CONFIG_I2C_ASYNC
//	while (true) {
//		uv_disable_int();
//		if (uv_ring_buffer_empty(&i2c[channel].tx)) {
//			uv_enable_int();
//			break;
//		}
//		uv_enable_int();
//		uv_rtos_task_yield();
//	}
//#endif
//
//	xfer.slaveAddr = dev_addr;
//	xfer.rxBuff = rx_buffer;
//	xfer.rxSz = rx_len;
//	xfer.txBuff = tx_buffer;
//	xfer.txSz = tx_len;
//	xfer.status = 0;
//
//	uint32_t r = 0;
//	/* start transfer */
//	Chip_I2CM_Xfer(p[channel], &xfer);
//
//	while (r == 0) {
//		/* wait for status change interrupt */
//		while (!Chip_I2CM_IsMasterPending(p[channel])) {
//			uv_rtos_task_yield();
//		}
////#if !CONFIG_I2C_ASYNC
//		/* call state change handler */
//		r = Chip_I2CM_XferHandler(p[channel], &xfer);
////#endif
//	}
//
//	if (xfer.status == I2CM_STATUS_ARBLOST) {
//		ret = ERR_ABORTED;
//	}
//

	return ret;
}



#if CONFIG_I2C_ASYNC




static i2c_tx_msg_st msg;
static I2C_RESULT_T res;
I2C_PARAM_T param;
static void transmit_next(i2c_e channel) {
	uv_disable_int();
		if (uv_ring_buffer_pop(&i2c[channel].tx, &msg) == ERR_NONE) {
			uv_terminal_enable(TERMINAL_CAN);
			printf("!\n");

			param.stop_flag = 1;
			param.func_pt = &I2C_XferDone;
			param.num_bytes_rec = 0;
			param.num_bytes_send = msg.data_len;
			param.buffer_ptr_rec = NULL;
			param.buffer_ptr_send = msg.data;
			printf("Transmit\n");
			LPC_I2CD_API->i2c_master_transmit_intr(i2cHandleMaster, &param, &res);

//			xfer.slaveAddr = msg.dev_addr;
//			xfer.rxBuff = NULL;
//			xfer.rxSz = 0;
//			xfer.txBuff = msg.data;
//			xfer.txSz = msg.data_len;
//			xfer.status = 0;
//			/* start transfer */
//			Chip_I2CM_Xfer(p[channel], &xfer);
		}
//	if (Chip_I2CM_IsMasterPending(p[channel])) {
//		// I2C is idle, ready to send next message
//	}
	uv_enable_int();
}



void I2C0_IRQHandler(void) {
	LPC_I2CD_API->i2c_isr_handler(i2cHandleMaster);
	//	uv_disable_int();
//	volatile uint32_t status = Chip_I2CM_GetStatus(I2C0);
//	Chip_I2CM_XferHandler(I2C0, &xfer);
//	transmit_next(I2C0);
//	Chip_I2CM_ClearStatus(I2C0, I2C_STAT_MSTPENDING);
//	uv_enable_int();
}


uv_errors_e uv_i2cm_write_async(i2c_e channel, uint8_t dev_addr,
		uint8_t *tx_buffer, uint16_t tx_len) {
	uv_can_errors_e ret = ERR_NONE;
	i2c_tx_msg_st msg;
	msg.dev_addr = dev_addr;
	msg.data_len = tx_len;
	memcpy(msg.data, tx_buffer, tx_len);

	printf("transmitting\n");
	uv_disable_int();
	ret = uv_ring_buffer_push(&i2c[channel].tx, &msg);
	uv_enable_int();

	transmit_next(channel);
}


#endif



#endif
