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

#include "uv_can.h"

#if CONFIG_CAN

#if CONFIG_CANOPEN
#include "uv_canopen.h"
#endif

#include "uv_gpio.h"
#include "uv_utilities.h"
#include "uv_rtos.h"
#if CONFIG_TERMINAL_CAN
#include "uv_terminal.h"
#include "uv_memory.h"
#include "uv_uart.h"
#endif
#include <stdio.h>
#include <string.h>
#include "chip.h"
#include "can_17xx_40xx.h"
#include "chip_lpc407x_8x.h"
#if CONFIG_NON_VOLATILE_MEMORY
#include CONFIG_MAIN_H
#endif



typedef struct {
	struct {
		bool init;
		uv_can_message_st rx_buffer_data[CONFIG_CAN0_RX_BUFFER_SIZE];
		uv_ring_buffer_st rx_buffer;
		uv_can_message_st tx_buffer_data[CONFIG_CAN0_TX_BUFFER_SIZE];
		uv_ring_buffer_st tx_buffer;
		bool (*rx_callback)(void *user_ptr, uv_can_msg_st *msg);
		bool (*tx_callback)(void *user_ptr, uv_can_msg_st *msg);
		LPC_CAN_T *lpc_can;
	} can[CAN_COUNT];

	#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
	#endif
} can_st;



static can_st _this __attribute__((section (".data_RAM2"))) = {
		.can = {
			{
				.init = false,
				.rx_callback = NULL,
				.tx_callback = NULL,
				.lpc_can = LPC_CAN1
			},
			{
				.init = false,
				.rx_callback = NULL,
				.tx_callback = NULL,
				.lpc_can = LPC_CAN2
			}
		},
};
#if defined(this)
#undef this
#endif
#define this (&_this)


void _uv_can_hal_send(uv_can_channels_e chn);





uv_errors_e uv_can_add_rx_callback(uv_can_channels_e chn,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg)) {
	this->can[chn].rx_callback = callback_function;

	return ERR_NONE;
}




uv_errors_e uv_can_add_tx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg)) {
	this->can[channel].tx_callback = callback_function;

	return ERR_NONE;
}


uv_errors_e _uv_can_init() {
	SystemCoreClockUpdate();

	Chip_SYSCTL_PeriphReset(SYSCTL_RESET_CAN1);
	Chip_SYSCTL_PeriphReset(SYSCTL_RESET_CAN2);
	Chip_SYSCTL_PeriphReset(SYSCTL_RESET_CANACC);


#if CONFIG_CAN0
	uint32_t baudrate;
	uv_ring_buffer_init(&this->can[0].rx_buffer, this->can[0].rx_buffer_data,
			CONFIG_CAN0_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
	uv_ring_buffer_init(&this->can[0].tx_buffer, this->can[0].tx_buffer_data,
			CONFIG_CAN0_TX_BUFFER_SIZE, sizeof(uv_can_message_st));

#if CONFIG_NON_VOLATILE_MEMORY
	baudrate = CONFIG_NON_VOLATILE_START.can_baudrate;
#else
	baudrate = CONFIG_CAN0_BAUDRATE;
#endif
	if (!baudrate || (baudrate > 2000000)) {
		baudrate = CONFIG_CAN0_BAUDRATE;
		CONFIG_NON_VOLATILE_START.can_baudrate = baudrate;
	}
	Chip_CAN_Init(this->can[0].lpc_can, LPC_CANAF, LPC_CANAF_RAM);
	Chip_CAN_SetBitRate(this->can[0].lpc_can, baudrate);
	Chip_CAN_EnableInt(this->can[0].lpc_can,
			CAN_IER_TIE1 | CAN_IER_TIE2 | CAN_IER_TIE3 | CAN_IER_RIE);


#if CONFIG_CAN0_RX_PIN == P0_0
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 0, FUNC1);
#elif CONFIG_CAN0_RX_PIN == P0_21
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 21, FUNC4);
#else
#error "CONFIG_CAN0_RX_PIN valid values P0_0 or P0_21"
#endif
#if CONFIG_CAN0_TX_PIN == P0_1
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 1, FUNC1);
#elif CONFIG_CAN0_TX_PIN == P0_22
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 22, FUNC4);
#else
#error "CONFIG_CAN0_TX_PIN valid values P0_1 or P0_22"
#endif

	this->can[0].init = true;
#endif

#if CONFIG_CAN1

	// todo: init CAN1 also
	uv_ring_buffer_init(&this->can[1].rx_buffer, this->can[1].rx_buffer_data,
			CONFIG_CAN1_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
	uv_ring_buffer_init(&this->can[1].tx_buffer, this->can[1].tx_buffer_data,
			CONFIG_CAN1_TX_BUFFER_SIZE, sizeof(uv_can_message_st));

	Chip_CAN_Init(this->can[1].lpc_can, LPC_CANAF, LPC_CANAF_RAM);
	Chip_CAN_SetBitRate(this->can[1].lpc_can, CONFIG_CAN1_BAUDRATE);
	Chip_CAN_EnableInt(this->can[1].lpc_can,
			CAN_IER_TIE1 | CAN_IER_TIE2 | CAN_IER_TIE3 | CAN_IER_RIE);


#if CONFIG_CAN1_RX_PIN == P0_4
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 4, FUNC2);
#elif CONFIG_CAN1_RX_PIN == P2_7
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 7, FUNC1 | MD_FAST_SLEW_RATE);
#else
#error "CONFIG_CAN1_RX_PIN valid values P0_4 or P2_7"
#endif
#if CONFIG_CAN1_TX_PIN == P0_5
	Chip_IOCON_PinMuxSet(LPC_IOCON, 0, 5, FUNC2);
#elif CONFIG_CAN1_TX_PIN == P2_8
	Chip_IOCON_PinMuxSet(LPC_IOCON, 2, 8, FUNC1 | MD_FAST_SLEW_RATE);
#else
#error "CONFIG_CAN1_TX_PIN valid values P0_5 or P2_8"
#endif

	this->can[1].init = true;
#endif

	// initialize acceptance filter
	CANAF_LUT_T afsections = {
		NULL, 0,
		NULL, 0,
		NULL, 0,
		NULL, 0,
		NULL, 0
	};
	Chip_CAN_SetAFLUT(LPC_CANAF, LPC_CANAF_RAM, &afsections);
	Chip_CAN_SetAFMode(LPC_CANAF, CAN_AF_NORMAL_MODE);

	/* Enable the CAN Interrupt */
	// Interrupt priority should not be 0 since FreeRTOS SYSCALL level is set to 1
	NVIC_ClearPendingIRQ(CAN_IRQn);
	NVIC_SetPriority(CAN_IRQn, 1);
	NVIC_EnableIRQ(CAN_IRQn);

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_init(&this->char_buffer, this->char_buffer_data,
			CONFIG_TERMINAL_BUFFER_SIZE, sizeof(char));
#if !CONFIG_CANOPEN
	uv_can_config_rx_message(CONFIG_TERMINAL_CAN_CHN,
			UV_TERMINAL_CAN_RX_ID + uv_get_id(),
			CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
#endif

	return ERR_NONE;

}




static void insert_msg(uv_can_chn_e chn,
		uint32_t id, uint32_t mask, uv_can_msg_types_e type, uint8_t ctz) {
	// clear masked bits to zero
	id = id & mask;
	// configure range
	if (type == CAN_STD) {
		CAN_STD_ID_RANGE_ENTRY_T range = {};
		range.LowerID.CtrlNo = chn;
		range.LowerID.ID_11 = id;
		range.UpperID.CtrlNo = chn;
		range.UpperID.ID_11 = (ctz) ? (id | (0x7FF & (~mask))) : id;
//		printf("RX configuring STD range 0x%x ... 0x%x for CAN%u\n",
//				range.LowerID.ID_11,
//				range.UpperID.ID_11,
//				range.LowerID.CtrlNo);
		Chip_CAN_InsertGroupSTDEntry(LPC_CANAF, LPC_CANAF_RAM, &range);
	}
	else {
		// type == CAN_EXT
		CAN_EXT_ID_RANGE_ENTRY_T range = {};
		range.LowerID.CtrlNo = chn;
		range.LowerID.ID_29 = id;
		range.UpperID.CtrlNo = chn;
		range.UpperID.ID_29 = (ctz) ? (id | (0x1FFFFFFF & (~mask))) : id;
//		printf("RX configuring EXT range 0x%x ... 0x%x for CAN%u\n",
//				range.LowerID.ID_29,
//				range.UpperID.ID_29,
//				range.LowerID.CtrlNo);
		Chip_CAN_InsertGroupEXTEntry(LPC_CANAF, LPC_CANAF_RAM, &range);
	}
}

uv_errors_e uv_can_config_rx_message(uv_can_channels_e chn,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {
	uv_errors_e ret = ERR_NONE;

	__disable_irq();
	Chip_CAN_SetAFMode(LPC_CANAF, CAN_AF_OFF_MODE);

	//config rx msg
	// calculate trailing zeroes from mask
	uint8_t ctz = uv_ctz(mask);
	// clear masked bits to zero
	id = id & mask;

	// configure single message in case mask has no zero bits
	insert_msg(chn, id, mask, type, ctz);

	// mask has to be divided and configured separately
	uint8_t last_onebit = ctz;
	for (uint8_t i = ctz; i < ((type == CAN_STD) ? 11 : 29); i++) {
		// loop through all bits. If other than trailing bits are masked,
		// all matching messages have to be configured separately
		if (!(mask & (1 << i))) {
			for (uint32_t val = 1; val < (1 << (i - last_onebit)); val++) {
				uint32_t idd = id + (val << i);
				uint32_t mask = ~((1 << ctz) - 1);
				insert_msg(chn,
						idd,
						mask,
						type, ctz);
			}
		}
		else {
			last_onebit = i;
		}
	}

	Chip_CAN_SetAFMode(LPC_CANAF, CAN_AF_NORMAL_MODE);
	__enable_irq();

	return ret;
}


void uv_can_clear_rx_messages(uv_can_chn_e chn) {
	Chip_CAN_clearAFLUT(LPC_CANAF, LPC_CANAF_RAM);
}



void CAN_IRQHandler(void) {
#if CONFIG_CAN0
	{
		// check what interrupt source is active
		uint32_t canint = Chip_CAN_GetIntStatus(this->can[0].lpc_can);
		if (canint & (CAN_ICR_TI1 | CAN_ICR_TI2 | CAN_ICR_TI3)) {
			// transmit interrupt active, send next msg
			_uv_can_hal_send(CAN0);
		}
		else if (canint & CAN_ICR_RI) {
			CAN_MSG_T m;
			if (Chip_CAN_Receive(this->can[0].lpc_can, &m) == SUCCESS) {
				uv_can_msg_st msg = { };
				msg.data_length = m.DLC;
				msg.id = m.ID & 0x1FFFFFFF;
				msg.type = (m.ID & CAN_EXTEND_ID_USAGE) ? CAN_EXT : CAN_STD;
				memcpy(msg.data_8bit, m.Data, m.DLC);

				if (this->can[0].rx_callback != NULL &&
						!this->can[0].rx_callback(__uv_get_user_ptr(), &msg)) {
				}
				else {
					uv_ring_buffer_push(&this->can[0].rx_buffer, &msg);
				}
#if CONFIG_TERMINAL_CAN && (CONFIG_TERMINAL_CAN_CHN == CAN0)
				// terminal characters are sent to their specific buffer
				if (msg.id == UV_TERMINAL_CAN_RX_ID +
						uv_canopen_get_our_nodeid() &&
						msg.type == CAN_STD &&
						msg.data_8bit[0] == 0x22 &&
						msg.data_8bit[1] == (UV_TERMINAL_CAN_INDEX & 0xFF) &&
						msg.data_8bit[2] == UV_TERMINAL_CAN_INDEX >> 8 &&
						msg.data_8bit[3] == UV_TERMINAL_CAN_SUBINDEX &&
						msg.data_length > 4) {
					uint8_t i;
					for (i = 0; i < msg.data_length - 4; i++) {
						uv_ring_buffer_push(&this->char_buffer,
								(char*) &msg.data_8bit[4 + i]);
					}
				}
#endif
			}
		}
		else if (canint != 0) {
//			printf("*** CAN AF configuration error 0x%x\n", canint);
			Chip_CAN_ConfigFullCANInt(LPC_CANAF, DISABLE);
		}
		else {

		}

	}
#endif
#if CONFIG_CAN1
	{
		// check what interrupt source is active
		uint32_t canint = Chip_CAN_GetIntStatus(this->can[1].lpc_can);
		if (canint & (CAN_ICR_TI1 | CAN_ICR_TI2 | CAN_ICR_TI3)) {
			// transmit interrupt active, send next msg
			_uv_can_hal_send(CAN1);
		}
		else if (canint & CAN_ICR_RI) {
			CAN_MSG_T m;
			if (Chip_CAN_Receive(this->can[1].lpc_can, &m) == SUCCESS) {
				uv_can_msg_st msg = { };
				msg.data_length = m.DLC;
				msg.id = m.ID & 0x1FFFFFFF;
				msg.type = (m.ID & CAN_EXTEND_ID_USAGE) ? CAN_EXT : CAN_STD;
				memcpy(msg.data_8bit, m.Data, m.DLC);
				if (this->can[1].rx_callback != NULL &&
						!this->can[1].rx_callback(__uv_get_user_ptr(), &msg)) {

				}
				else {
					uv_ring_buffer_push(&this->can[1].rx_buffer, &msg);
				}
#if CONFIG_TERMINAL_CAN && (CONFIG_TERMINAL_CAN_CHN == CAN1)
				// terminal characters are sent to their specific buffer
				if (msg.id == UV_TERMINAL_CAN_RX_ID +
						uv_canopen_get_our_nodeid() &&
						msg.type == CAN_STD &&
						msg.data_8bit[0] == 0x22 &&
						msg.data_8bit[1] == (UV_TERMINAL_CAN_INDEX & 0xFF) &&
						msg.data_8bit[2] == UV_TERMINAL_CAN_INDEX >> 8 &&
						msg.data_8bit[3] == UV_TERMINAL_CAN_SUBINDEX &&
						msg.data_length > 4) {
					uint8_t i;
					for (i = 0; i < msg.data_length - 4; i++) {
						uv_ring_buffer_push(&this->char_buffer,
								(char*) &msg.data_8bit[4 + i]);
					}
				}
#endif
			}
		}
		else if (canint != 0) {
//			printf("0x%x\n", canint);
			Chip_CAN_ConfigFullCANInt(LPC_CANAF, DISABLE);
		}
		else {

		}
	}
#endif
}



void _uv_can_hal_send(uv_can_channels_e chn) {
	if (chn);

	uv_can_message_st msg;

	NVIC_DisableIRQ(CAN_IRQn);

	uv_errors_e e = uv_ring_buffer_pop(&this->can[chn].tx_buffer, &msg);

	if (e == ERR_NONE) {
		// wait until tx msg obj is free
		uint8_t txbuf;
		while ((txbuf = Chip_CAN_GetFreeTxBuf(this->can[chn].lpc_can)) ==
				CAN_BUFFER_LAST) {
			// if CAN bus is in error state, stop and return
			e = uv_can_get_error_state(chn);
			if (e != CAN_ERROR_ACTIVE) {
				break;
			}
		}

		if (e == ERR_NONE) {
			// send msg
			CAN_MSG_T m;
			m.DLC = msg.data_length;
			memcpy(m.Data, msg.data_8bit, 8);
			m.ID = msg.id | ((msg.type == CAN_EXT) ? CAN_EXTEND_ID_USAGE : 0);
			m.Type = 0;
			// call tx callback
			Chip_CAN_Send(this->can[chn].lpc_can, txbuf, &m);
		}
	}
	NVIC_EnableIRQ(CAN_IRQn);
}



uv_can_errors_e uv_can_get_error_state(uv_can_channels_e chn) {
	if (chn) {};
	uv_can_errors_e e = CAN_ERROR_ACTIVE;

	uint32_t stat = Chip_CAN_GetGlobalStatus(this->can[chn].lpc_can);
	if (stat & CAN_GSR_BS) {
		e = CAN_ERROR_BUS_OFF;
	}
	else if (stat & CAN_GSR_ES) {
		e = CAN_ERROR_PASSIVE;
	}
	else {

	}
	return e;
}


static uv_errors_e uv_can_send_sync(uv_can_channels_e chn, uv_can_message_st *msg) {
	uv_errors_e ret = ERR_NONE;

	if (this->can[chn].init) {

		// disable transmit interrupts so that we have free txbuffer
		NVIC_DisableIRQ(CAN_IRQn);

		// wait until tx msg obj is free
		uint8_t txbuf;
		while ((txbuf = Chip_CAN_GetFreeTxBuf(this->can[chn].lpc_can)) ==
				CAN_BUFFER_LAST) {
			// if CAN bus is in error state, stop and return
			uv_can_errors_e e = uv_can_get_error_state(chn);
			if (e != CAN_ERROR_ACTIVE) {
				ret = ERR_CAN_BUS_OFF;
				break;
			}
		}

		if (ret == ERR_NONE) {

			// send msg
			CAN_MSG_T m;
			m.DLC = msg->data_length;
			memcpy(m.Data, msg->data_8bit, 8);
			m.ID = msg->id | ((msg->type == CAN_EXT) ? CAN_EXTEND_ID_USAGE : 0);
			m.Type = 0;
			Chip_CAN_Send(this->can[chn].lpc_can, txbuf, &m);

			// wait until message is transferred or CAN status changes
			bool br = false;
			while (true) {
				uint32_t stat = Chip_CAN_GetGlobalStatus(this->can[chn].lpc_can);
				if ((stat & CAN_GSR_BS) ||
						(stat & CAN_GSR_ES)) {
					br = true;
					ret = ERR_CAN_BUS_OFF;
				}
				if (Chip_CAN_GetStatus(this->can[chn].lpc_can) & CAN_SR_TBS(txbuf)) {
					br = true;
				}
				if (br) {
					break;
				}
			}
		}
		NVIC_EnableIRQ(CAN_IRQn);
	}
	else {
		ret = ERR_NOT_INITIALIZED;
	}

	return ret;
}


void uv_can_set_baudrate(uv_can_chn_e chn, uint32_t baudrate) {
	Chip_CAN_SetBitRate(this->can[chn].lpc_can, baudrate);
}










/*
 * GLOBAL FUNCTIONS ACROSS TARGET MCU's
 */


#if CONFIG_TERMINAL_CAN
uv_errors_e uv_can_get_char(char *dest) {
	return uv_ring_buffer_pop(&this->char_buffer, dest);
}
#endif



uv_errors_e uv_can_send_flags(uv_can_channels_e chn, uv_can_msg_st *msg,
		can_send_flags_e flags) {
	uv_errors_e ret = ERR_NONE;
	uv_disable_int();
	if (flags & CAN_SEND_FLAGS_LOCAL) {
		// pushing to receive buffer never triggers rx callback
		if (this->can[chn].rx_callback &&
				!this->can[chn].rx_callback(__uv_get_user_ptr(), msg)) {
		}
		else {
			ret = uv_ring_buffer_push(&this->can[chn].rx_buffer, msg);
		}
	}
	if (flags & CAN_SEND_FLAGS_SYNC) {
		ret = uv_can_send_sync(chn, msg);
	}
	if (flags & CAN_SEND_FLAGS_NORMAL) {
		ret = uv_ring_buffer_push(&this->can[chn].tx_buffer, msg);
	}
	if (!(flags & CAN_SEND_FLAGS_NO_TX_CALLB) &&
			(this->can[chn].tx_callback != NULL)) {
		this->can[chn].tx_callback(__uv_get_user_ptr(), msg);
	}

	uv_enable_int();
	return ret;
}



uv_errors_e uv_can_pop_message(uv_can_channels_e chn, uv_can_message_st *message) {
	uv_errors_e ret = ERR_NONE;
	uv_disable_int();
	ret = uv_ring_buffer_pop(&this->can[chn].rx_buffer, message);
	uv_enable_int();
	return ret;
}


uv_errors_e uv_can_reset(uv_can_channels_e chn) {
	Chip_SYSCTL_PeriphReset(SYSCTL_RESET_CAN1 + chn);

	return ERR_NONE;
}



/// @brief: Inner hal step function which is called in rtos hal task
void _uv_can_hal_step(unsigned int step_ms) {


	// send the next message from the buffer
#if CONFIG_CAN0
	_uv_can_hal_send(CAN0);
	// boot up CAN if we end up in BUSOFF mode
	if (this->can[0].lpc_can->GSR & (1 << 7)) {
		this->can[0].lpc_can->MOD &= ~1;
	}
#endif
#if CONFIG_CAN1
	_uv_can_hal_send(CAN1);
	// boot up CAN if we end up in BUSOFF mode
	if (this->can[1].lpc_can->GSR & (1 << 7)) {
		this->can[1].lpc_can->MOD &= ~1;
	}
#endif
}


#endif
