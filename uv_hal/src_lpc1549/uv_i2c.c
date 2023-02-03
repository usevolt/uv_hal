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

/* Use a buffer size larger than the expected return value of
   i2c_get_mem_size() for the static I2C handle type */
static uint32_t i2c_master_handle_mem[0x20];
static I2C_HANDLE_T *i2c_handle_master;
static i2c_tx_msg_st msg;
static I2C_RESULT_T res;
static I2C_PARAM_T param;



static void transmit_next(i2c_e channel);
static void i2c_transfer_int_callb(uint32_t err_code, uint32_t n);



uv_errors_e _uv_i2c_init(void) {
	// Enable I2C clock and reset I2C peripheral - the boot ROM does not do this
	Chip_I2C_Init(LPC_I2C0);

	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, IOCON_STDI2C_EN);
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 23, IOCON_STDI2C_EN);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SCL);
	Chip_SWM_EnableFixedPin(SWM_FIXED_I2C0_SDA);

	uv_terminal_enable(TERMINAL_CAN);
	if (LPC_I2CD_API->i2c_get_mem_size() >
	sizeof(i2c_master_handle_mem)) {
		/* Example only: this should never happen and probably isn't needed for
		   most I2C code. */
		printf("error\n");
	}

	/* Setup the I2C handle */
	i2c_handle_master = LPC_I2CD_API->i2c_setup(
			LPC_I2C_BASE, i2c_master_handle_mem);
	if (i2c_handle_master == NULL) {
		printf("i2c_setup error\n");
	}

	/* Set I2C bitrate */
	if (LPC_I2CD_API->i2c_set_bitrate(
			i2c_handle_master, Chip_Clock_GetSystemClockRate(),
									  CONFIG_I2C_BAUDRATE) != LPC_OK) {
		printf("speed error\n");
	}

	NVIC_SetPriority(I2C0_IRQn, 1);
	NVIC_EnableIRQ(I2C0_IRQn);

#if CONFIG_I2C_ASYNC
	for (uint8_t i = 0; i < I2C_COUNT; i++) {
		uv_ring_buffer_init(&i2c[i].tx, &i2c[i].tx_buffer,
				CONFIG_I2C_ASYNC_BUFFER_LEN, sizeof(i2c_tx_msg_st));
	}
#endif


	return ERR_NONE;
}



uv_errors_e uv_i2cm_readwrite(i2c_e channel, uint8_t dev_addr, uint8_t *tx_buffer, uint16_t tx_len,
		uint8_t *rx_buffer, uint16_t rx_len) {
	uv_errors_e ret = ERR_NONE;

	while (LPC_I2CD_API->i2c_get_status(i2c_handle_master) != IDLE) {
		uv_rtos_task_yield();
	}
#if CONFIG_I2C_ASYNC
	while (true) {
		uv_disable_int();
		if (uv_ring_buffer_empty(&i2c[channel].tx)) {
			uv_enable_int();
			break;
		}
		uv_enable_int();
		uv_rtos_task_yield();
	}
#endif

	param.stop_flag = 1;
	param.func_pt = &i2c_transfer_int_callb;
	param.num_bytes_rec = rx_len;
	param.buffer_ptr_rec = rx_buffer;
	param.num_bytes_send = tx_len;
	param.buffer_ptr_send = tx_buffer;

	if (LPC_I2CD_API->i2c_master_tx_rx_poll(
			i2c_handle_master, &param, &res) != LPC_OK) {
		ret = ERR_ABORTED;
	}


	return ret;
}



#if CONFIG_I2C_ASYNC




static void transmit_next(i2c_e channel) {
	if (LPC_I2CD_API->i2c_get_status(i2c_handle_master) == IDLE) {
		uv_disable_int();
		if (uv_ring_buffer_pop(&i2c[channel].tx, &msg) == ERR_NONE) {
			param.stop_flag = 1;
			param.func_pt = &i2c_transfer_int_callb;
			param.num_bytes_rec = 0;
			param.num_bytes_send = msg.data_len;
			param.buffer_ptr_rec = NULL;
			param.buffer_ptr_send = msg.data;

			LPC_I2CD_API->i2c_master_transmit_intr(
					i2c_handle_master, &param, &res);
		}
		uv_enable_int();
	}
}



void I2C0_IRQHandler(void) {
	LPC_I2CD_API->i2c_isr_handler(i2c_handle_master);
}



static void i2c_transfer_int_callb(uint32_t err_code, uint32_t n) {
	transmit_next(I2C0);
}



uv_errors_e uv_i2cm_write_async(i2c_e channel, uint8_t dev_addr,
		uint8_t *tx_buffer, uint16_t tx_len) {
	uv_can_errors_e ret = ERR_NONE;
	i2c_tx_msg_st msg;
	msg.dev_addr = dev_addr;
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
