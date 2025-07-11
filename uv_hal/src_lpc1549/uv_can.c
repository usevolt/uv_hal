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
#include "romapi_15xx.h"
#include "rom_can_15xx.h"
#if CONFIG_NON_VOLATILE_MEMORY
#include CONFIG_MAIN_H
#endif

/*------------- CAN Controller (CAN) ----------------------------*/
/** @addtogroup LPC15xx_CAN LPC15xx Controller Area Network(CAN)
  @{
*/
typedef struct
{
  __IO uint32_t CNTL;				/* 0x000 */
  __IO uint32_t STAT;
  __I  uint32_t EC;
  __IO uint32_t BT;
  __I  uint32_t INT;
  __IO uint32_t TEST;
  __IO uint32_t BRPE;
       uint32_t RESERVED0;
  __IO uint32_t IF1_CMDREQ;			/* 0x020 */
  __IO uint32_t IF1_CMDMSK;
  __IO uint32_t IF1_MSK1;
  __IO uint32_t IF1_MSK2;
  __IO uint32_t IF1_ARB1;
  __IO uint32_t IF1_ARB2;
  __IO uint32_t IF1_MCTRL;
  __IO uint32_t IF1_DA1;
  __IO uint32_t IF1_DA2;
  __IO uint32_t IF1_DB1;
  __IO uint32_t IF1_DB2;
       uint32_t RESERVED1[13];
  __IO uint32_t IF2_CMDREQ;			/* 0x080 */
  __IO uint32_t IF2_CMDMSK;
  __IO uint32_t IF2_MSK1;
  __IO uint32_t IF2_MSK2;
  __IO uint32_t IF2_ARB1;
  __IO uint32_t IF2_ARB2;
  __IO uint32_t IF2_MCTRL;
  __IO uint32_t IF2_DA1;
  __IO uint32_t IF2_DA2;
  __IO uint32_t IF2_DB1;
  __IO uint32_t IF2_DB2;
       uint32_t RESERVED2[21];
  __I  uint32_t TXREQ1;				/* 0x100 */
  __I  uint32_t TXREQ2;
       uint32_t RESERVED3[6];
  __I  uint32_t ND1;				/* 0x120 */
  __I  uint32_t ND2;
       uint32_t RESERVED4[6];
  __I  uint32_t IR1;				/* 0x140 */
  __I  uint32_t IR2;
       uint32_t RESERVED5[6];
  __I  uint32_t MSGV1;				/* 0x160 */
  __I  uint32_t MSGV2;
       uint32_t RESERVED6[6];
  __IO uint32_t CLKDIV;				/* 0x180 */
} LPC_CAN_TypeDef;
/*@}*/ /* end of group LPC15xx_CAN */


#define LPC_CAN               ((LPC_CAN_TypeDef *)LPC_C_CAN0_BASE)


#if CONFIG_CAN_LOG
extern bool can_log;
#endif

/// @brief: Basic CAN message data structure
/// LPC11C22 C_CAN driver struct. Do not change!
typedef struct {
	/// @brief: Messages ID and mode.
	/// Mode defines if message is 11 bit, 29 bit data frame or rtr frame.
	/// Control bits are OR'red with message ID, for example
	/// message.msg_id = CAN_MSGOBJ_STD | 0x123;
	/// Mode is CAN_MSGOBJ_STD by default.
	uint32_t msg_id;
	/// @brief: Message ID mask. Multiple ID's can be masked
	/// to be received into a single message object by masking
	/// out the least significant bits from message ID. This mask is AND'ed
	/// with the received messages ID => use 0x7FF is default mask for 11 bit ID's.
	uint32_t mask;
	/// @brief: Message data bytes
	uint8_t data[8];
	/// @brief: Defines how many bytes of data is sent
	uint8_t data_length;
	/// @brief: Defines which hardware message object is used for this message.
	/// Message objects should not be multiplexed among many messages.
	/// For receiving multiple messages with a single message object, use mask bits.
	uint8_t msgobj;
} hal_can_msg_obj_st;


typedef struct {
	uint32_t used_msg_objs;
	// temporary message struct. This can be used instead of local variables
	// to save stack memory.
	uv_can_message_st temp_msg;
	// temporary hal message object to be used when receiving messages
	// defined here to be global instead of local
	hal_can_msg_obj_st temp_obj;
	bool init;
	uv_can_message_st rx_buffer_data[CONFIG_CAN0_RX_BUFFER_SIZE];
	uv_ring_buffer_st rx_buffer;
	uv_can_message_st tx_buffer_data[CONFIG_CAN0_TX_BUFFER_SIZE];
	uv_ring_buffer_st tx_buffer;
	int16_t tx_pending;
#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_st char_buffer;
	char char_buffer_data[CONFIG_TERMINAL_BUFFER_SIZE];
#endif
	bool (*rx_callback[CAN_COUNT])(void *user_ptr, uv_can_msg_st *msg);
	bool (*tx_callback[CAN_COUNT])(void *user_ptr, uv_can_msg_st *msg, can_send_flags_e flags);

} can_st;



static can_st _can = {
		.init = false,
		.tx_callback[0] = NULL,
		.rx_callback[0] = NULL
};
#define this (&_can)


void _uv_can_hal_send(uv_can_channels_e chn);





#define TX_MSG_OBJ	1


/// @brief: Time limit for pending message objects. If this exceeds, all message objects are
/// released from pending state.
#define PENDING_MSG_OBJ_TIME_LIMIT_MS		8


static uint32_t get_msgif_id(uv_can_msg_types_e ext) {
	uint32_t id;
	if (ext) {
		id = LPC_CAN->IF1_ARB1;
		id += (LPC_CAN->IF1_ARB2 & 0x1FFF) << 16;
	}
	else {
		id = (LPC_CAN->IF1_ARB2 & 0x1FFF) >> 2;
	}
	return id;
}

static void set_msgif_id(const uint32_t id, const bool ext) {
	// clear the ARB2 register, since otherwise some settings might come from the
	// last message transmitted
	LPC_CAN->IF1_ARB2 = 0;
	if (ext) {
		LPC_CAN->IF1_ARB1 = id & 0xFFFF;
		LPC_CAN->IF1_ARB2 |= ((id >> 16) & 0x1FFF) | (1 << 14);
	}
	else {
		LPC_CAN->IF1_ARB2 |= ((id & 0x7FF) << 2);
	}
}

static uint32_t get_msgif_mask(const uv_can_msg_types_e type) {
	uint32_t mask;
	if (type == CAN_EXT) {
		mask = LPC_CAN->IF1_MSK1 + ((LPC_CAN->IF1_MSK2 & 0x1FFF) << 16);
	}
	else {
		mask = (LPC_CAN->IF1_MSK2 & 0x1FFF) >> 2;
	}
	return mask;
}

static void set_msgif_mask(const uint32_t mask, const uv_can_msg_types_e type) {
	if (type == CAN_EXT) {
		LPC_CAN->IF1_MSK1 = mask & 0xFFFF;
		LPC_CAN->IF1_MSK2 &= ~(0x1FFF);
		LPC_CAN->IF1_MSK2 |= ((mask >> 16) & 0x1FFF);
	}
	else {
		LPC_CAN->IF1_MSK1 = 0xFFFF;
		LPC_CAN->IF1_MSK2 &= ~(0x1FFF);
		LPC_CAN->IF1_MSK2 |= ((mask & 0x7FF) << 2);
	}
}

/// @return: CAN_EXT if extended, CAN_STD if standard
static inline uv_can_msg_types_e get_msgif_type(void) {
	return ((LPC_CAN->IF1_ARB2 & (1 << 14)) >> 14) ? CAN_EXT : CAN_STD;
}

static void read_msg_obj(uint8_t msg_obj) {
	// load the whole message
	LPC_CAN->IF1_CMDMSK = 0x7F;
	while (LPC_CAN->IF1_CMDREQ & (1 << 15));
	LPC_CAN->IF1_CMDREQ = msg_obj;
	while (LPC_CAN->IF1_CMDREQ & (1 << 15));
}

static void write_msg_obj(uint8_t msg_obj, bool txrqst) {
	LPC_CAN->IF1_CMDMSK = 0xFB | ((txrqst ? 1 : 0) << 2);
	while(LPC_CAN->IF1_CMDREQ & (1 << 15));
	LPC_CAN->IF1_CMDREQ = msg_obj;
	while(LPC_CAN->IF1_CMDREQ & (1 << 15));
}

static void msg_obj_enable(uint8_t msg_obj) {
	read_msg_obj(msg_obj);
	LPC_CAN->IF1_ARB2 |= (1 << 15);
	write_msg_obj(msg_obj, false);
}

static void msg_obj_disable(uint8_t msg_obj) {
	read_msg_obj(msg_obj);
	LPC_CAN->IF1_ARB2 &= ~(1 << 15);
	write_msg_obj(msg_obj, false);
}


uv_errors_e uv_can_add_rx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg)) {
	this->rx_callback[channel] = callback_function;

	return ERR_NONE;
}


uv_errors_e uv_can_add_tx_callback(uv_can_channels_e channel,
		bool (*callback_function)(void *user_ptr, uv_can_msg_st *msg,
				can_send_flags_e flags)) {
	this->tx_callback[channel] = callback_function;

	return ERR_NONE;
}


static bool send_terminal(uv_can_msg_st *msg) {
	bool ret = false;
#if CONFIG_TERMINAL_CAN
	// terminal characters are sent to their specific buffer
	if (msg->id == UV_TERMINAL_CAN_RX_ID + uv_canopen_get_our_nodeid() &&
			msg->type == CAN_STD &&
			msg->data_8bit[0] == 0x22 &&
			msg->data_8bit[1] == (UV_TERMINAL_CAN_INDEX & 0xFF) &&
			msg->data_8bit[2] == UV_TERMINAL_CAN_INDEX >> 8 &&
			msg->data_8bit[3] == UV_TERMINAL_CAN_SUBINDEX &&
			msg->data_length > 4) {
		uint8_t i;
		for (i = 0; i < msg->data_length - 4; i++) {
			uv_ring_buffer_push(&this->char_buffer,
					(char*) &msg->data_8bit[4 + i]);
		}
		ret = true;
	}
#endif

	return ret;
}


void CAN_IRQHandler(void) {
	volatile uint32_t p = LPC_CAN->INT;

	if (p <= 0x20) {
		uint32_t int_pend = LPC_CAN->IR1 | (LPC_CAN->IR2 << 16);
		while (int_pend) {
			// find out the pending message object
			uint8_t msg_obj = 0;
			for (msg_obj = 0; msg_obj < 32; msg_obj++) {
				if (int_pend & (1 << msg_obj)) {

					__disable_irq();

					// read the pending object
					read_msg_obj(msg_obj + 1);

					if ((msg_obj + 1) != TX_MSG_OBJ) {
						// copy the message data and push it into rx ring buffer
						uv_can_msg_st msg;
						msg.type = (LPC_CAN->IF1_ARB2 & (1 << 14)) ? CAN_EXT : CAN_STD;
						msg.data_length = LPC_CAN->IF1_MCTRL & 0b1111;
						msg.id = get_msgif_id(msg.type);
						msg.data_16bit[0] = LPC_CAN->IF1_DA1;
						msg.data_16bit[1] = LPC_CAN->IF1_DA2;
						msg.data_16bit[2] = LPC_CAN->IF1_DB1;
						msg.data_16bit[3] = LPC_CAN->IF1_DB2;

						if (this->rx_callback[0] != NULL &&
								!this->rx_callback[0](__uv_get_user_ptr(), &msg)) {

						}
						else {
							if (!send_terminal(&msg)) {
								uv_ring_buffer_push(&this->rx_buffer, &msg);
							}
						}
					}
					int_pend &= ~(1 << msg_obj);

					__enable_irq();

					if ((msg_obj + 1) == TX_MSG_OBJ) {
						_uv_can_hal_send(CAN0);
					}
				}
			}

		}
	}
	else {
		// detect bus off and fix it
		if (LPC_CAN->STAT & (1 << 7)) {
			LPC_CAN->CNTL &= ~(1);
		}
	}

	// clear status interrupts by reading the status register
	// clear succesfull rx & tx
	LPC_CAN->STAT &= ~(0b11 << 3);
}


uv_errors_e _uv_can_init() {
	this->init = true;
	this->used_msg_objs = (1 << (TX_MSG_OBJ - 1));

	uv_ring_buffer_init(&this->rx_buffer, this->rx_buffer_data,
			CONFIG_CAN0_RX_BUFFER_SIZE, sizeof(uv_can_message_st));
	uv_ring_buffer_init(&this->tx_buffer, this->tx_buffer_data,
			CONFIG_CAN0_TX_BUFFER_SIZE, sizeof(uv_can_message_st));

	SystemCoreClockUpdate();

	/* Enable clocking for CAN and reset the controller */
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_CAN);
	Chip_SYSCTL_PeriphReset(RESET_CAN);

	// enable error interrupts & automatic retransmission
	LPC_CAN->CNTL = 0b11 | (1 << 3) | (1 << 6);

	// baudrate prescaler
	// BTR register: TSEG1 = 4, TSEG2 = 3, SJW = 0
	uint32_t baudrate;
#if CONFIG_NON_VOLATILE_MEMORY
	baudrate = CONFIG_NON_VOLATILE_START.can_baudrate;
#else
	baudrate = CONFIG_CAN0_BAUDRATE;
#endif
	if (!baudrate || (baudrate > 2000000)) {
		baudrate = CONFIG_CAN0_BAUDRATE;
		CONFIG_NON_VOLATILE_START.can_baudrate = baudrate;
	}
	LPC_CAN->BT = ((SystemCoreClock / (baudrate * 8) - 1) & 0x3F)
				  | (3 << 8) | (2 << 12);
	LPC_CAN->CLKDIV = 0;

	// init all message objects
	for (int i = 0; i < 32; i++) {
		msg_obj_disable(i + 1);
	}

	/* Enable the CAN Interrupt */
	// Interrupt priority should not be 0 since FreeRTOS SYSCALL level is set to 1
	NVIC_SetPriority(CAN_IRQn, 1);
	NVIC_EnableIRQ(CAN_IRQn);

	Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_CAN0_TX_PIN),
			UV_GPIO_PIN(CONFIG_CAN0_TX_PIN), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_IOCON_PinMuxSet(LPC_IOCON, UV_GPIO_PORT(CONFIG_CAN0_RX_PIN),
			UV_GPIO_PIN(CONFIG_CAN0_RX_PIN), (IOCON_MODE_INACT | IOCON_DIGMODE_EN));
	Chip_SWM_MovablePortPinAssign(SWM_CAN_TD1_O, UV_GPIO_PORT(CONFIG_CAN0_TX_PIN),
			UV_GPIO_PIN(CONFIG_CAN0_TX_PIN));
	Chip_SWM_MovablePortPinAssign(SWM_CAN_RD1_I,  UV_GPIO_PORT(CONFIG_CAN0_RX_PIN),
			UV_GPIO_PIN(CONFIG_CAN0_RX_PIN));


#if CAN_LOOP_BACK_MODE
	// enable loop-back mode for testing
	LPC_CAN->CNTL |= 1 << 7;
	LPC_CAN->TEST |= 1 << 4;
#endif

#if CONFIG_TERMINAL_CAN
	uv_ring_buffer_init(&this->char_buffer, this->char_buffer_data,
			CONFIG_TERMINAL_BUFFER_SIZE, sizeof(char));
#if !CONFIG_CANOPEN
	uv_can_config_rx_message(CAN0, UV_TERMINAL_CAN_RX_ID + uv_get_id(),
			CAN_ID_MASK_DEFAULT, CAN_STD);
#endif
#endif


	// disable access to bit timing register and start CAN controller operation
	LPC_CAN->CNTL &= ~((1 << 0) | (1 << 6));

	return ERR_NONE;

}



uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id,
		unsigned int mask,
		uv_can_msg_types_e type) {
	uv_errors_e ret = ERR_NONE;

	__disable_irq();

	// check if any message objects are configured to receive this type of data already
	bool match = false;
	for (uint8_t i = TX_MSG_OBJ; i < 32; i++) {
		if (GET_MASKED(this->used_msg_objs, (1 << i))) {

			read_msg_obj(i + 1);
			uv_can_msg_types_e msgif_type = get_msgif_type();
			uint32_t msgif_id = get_msgif_id(msgif_type),
					msgif_mask = get_msgif_mask(msgif_type);
			if (msgif_type == type) {
				// check if new id is a subset of the existing msgif mask
				uint32_t masked_id = id & mask;
				uint32_t masked_ifid = msgif_id & msgif_mask;
				uint32_t bits_differ = masked_id ^ masked_ifid;
				if (masked_id == masked_ifid) {
					// if new msg matches with msgif message and mask, we dont
					// need to configure new message
					if ((msgif_id & msgif_mask) == (id & msgif_mask)) {
						match = true;
						break;
					}
				}
				// difference between masked id's has to be 1 bit at most
				// and also new mask has to be include old mask in it
				else if (uv_countofbit(bits_differ, 1) == 1 &&
						(msgif_mask & ~mask) == 0) {
					// only 1 bit differs in ID. Modify mask to include both messages
					msgif_mask &= ~bits_differ;
					msg_obj_disable(i + 1);
					read_msg_obj(i + 1);
					// write receive message data to msg obj
					set_msgif_mask(msgif_mask, type);
					set_msgif_id(msgif_id, (msgif_type == CAN_EXT));
					write_msg_obj(i + 1, false);
					msg_obj_enable(i + 1);
					match = true;
					break;
				}
				else {

				}
			}
		}
	}

	if (!match) {
		// this message is not yet configured.
		// If any message objects are still not in use,
		// config them with settings requested
		// the last message object is reserved for sending messages
		for (uint8_t i = TX_MSG_OBJ; i < 32; i++) {
			// search for a unused message object to be used for receiving the messages
			if (!GET_MASKED(this->used_msg_objs, (1 << i))) {

				msg_obj_disable(i + 1);

				read_msg_obj(i + 1);
				// write receive message data to msg obj
				set_msgif_mask(mask, type);
				// use msg type and dir for filtering
				LPC_CAN->IF1_MSK2 |= (0b11 << 14);
				set_msgif_id(id, (type == CAN_EXT));
				LPC_CAN->IF1_MCTRL = (1 << 7) |		// single message (end of buffer)
						(1 << 10) |					// receive interrupt enabled
						(1 << 12);					// use mask for filtering

				write_msg_obj(i + 1, false);
				msg_obj_enable(i + 1);

				this->used_msg_objs |= (1 << i);
				match = true;
				break;
			}
		}
	}

	if (!match) {
		ret = ERR_CAN_RX_MESSAGE_COUNT_FULL;
	}

	if (ret) {
		uv_terminal_enable(TERMINAL_CAN);
		printf("CAN config rxmsgobj err: %u\n", ret);
	}

	__enable_irq();

	return ret;
}


void uv_can_clear_rx_messages(uv_can_chn_e chn) {
	if (chn);
	this->used_msg_objs = (1 << (TX_MSG_OBJ - 1));

	// clear all rx message objects
	for (int i = TX_MSG_OBJ; i < 32; i++) {
		msg_obj_disable(i + 1);
	}
}



void _uv_can_hal_send(uv_can_channels_e chn) {
	if (chn);

	// if tx object is pending, try to wait for the HAl task to release the pending msg obj
	if ((LPC_CAN->TXREQ1 & 1) || !this->init) {
		return;
	}

	uv_can_message_st msg;

	NVIC_DisableIRQ(CAN_IRQn);

	uv_errors_e e = uv_ring_buffer_pop(&this->tx_buffer, &msg);

	if (e == ERR_NONE) {


		msg_obj_disable(TX_MSG_OBJ);

		set_msgif_id(msg.id, (msg.type == CAN_EXT));

		LPC_CAN->IF1_ARB2 |= (1 << 13) |					// transmit msg
				(((msg.type == CAN_EXT) ? 1 : 0) << 14) |	// frame type
				(1 << 15);									// message valid

		LPC_CAN->IF1_MCTRL = (msg.data_length << 0) |		// data length
				(1 << 7) |									// single message (end of buffer)
				(1 << 8) |									// transmit request
				(1 << 11) |									// transmit int enabled
				(1 << 15);									// new data written by CPU
		LPC_CAN->IF1_DA1 = msg.data_16bit[0];
		LPC_CAN->IF1_DA2 = msg.data_16bit[1];
		LPC_CAN->IF1_DB1 = msg.data_16bit[2];
		LPC_CAN->IF1_DB2 = msg.data_16bit[3];

		// load the message into msg obj
		write_msg_obj(TX_MSG_OBJ, true);

		msg_obj_enable(TX_MSG_OBJ);

	}
	NVIC_EnableIRQ(CAN_IRQn);
}



uv_can_errors_e uv_can_get_error_state(uv_can_channels_e channel) {
	if (channel) {};
	uv_can_errors_e e = CAN_ERROR_ACTIVE;

	if (LPC_CAN->STAT & (1 << 7)) {
		e = CAN_ERROR_BUS_OFF;
	}
	else if (LPC_CAN->STAT & (1 << 5)) {
		e = CAN_ERROR_PASSIVE;
	}
	else if (LPC_CAN->STAT & (1 << 6)) {
		e = CAN_ERROR_WARNING;
	}
	else {

	}
	return e;
}


static uv_errors_e uv_can_send_sync(uv_can_channels_e channel, uv_can_message_st *msg) {
	uv_errors_e ret = ERR_NONE;

	if (this->init) {

		// wait until tx msg obj is free
		while (LPC_CAN->TXREQ1 & (1 << (TX_MSG_OBJ - 1))) {
			// if CAN bus is in error state, stop and return
			uv_can_errors_e e = uv_can_get_error_state(channel);
			if (e != CAN_ERROR_ACTIVE) {
				ret = ERR_CAN_BUS_OFF;
				break;
			}
		}

		if (ret == ERR_NONE) {

		uv_disable_int();

			msg_obj_disable(TX_MSG_OBJ);

			set_msgif_id(msg->id, (msg->type == CAN_EXT));

			LPC_CAN->IF1_ARB2 |= (1 << 13) |					// transmit msg
					(((msg->type == CAN_EXT) ? 1 : 0) << 14) |	// frame type
					(1 << 15);									// message valid

			LPC_CAN->IF1_MCTRL = (msg->data_length << 0) |		// data length
					(1 << 7) |									// single message (end of buffer)
					(1 << 8) |									// transmit request
					(1 << 11) |									// transmit int enabled
					(1 << 15);									// new data written by CPU
			LPC_CAN->IF1_DA1 = msg->data_16bit[0];
			LPC_CAN->IF1_DA2 = msg->data_16bit[1];
			LPC_CAN->IF1_DB1 = msg->data_16bit[2];
			LPC_CAN->IF1_DB2 = msg->data_16bit[3];

			// load the message into msg obj
			write_msg_obj(TX_MSG_OBJ, true);

			msg_obj_enable(TX_MSG_OBJ);

			uv_enable_int();

			// wait until message is transferred or CAN status changes
			bool br = false;
			while (true) {
				if (!(LPC_CAN->TXREQ1 & 1)) {
					br = true;
				}
				else if (LPC_CAN->STAT & (0b111 << 5)) {
					br = true;
					ret = ERR_CAN_BUS_OFF;
				}
				else {

				}
				if (br) {
					break;
				}
			}
		}
	}
	else {
		ret = ERR_NOT_INITIALIZED;
	}

	return ret;
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
		if (!send_terminal(msg)) {
			ret = uv_ring_buffer_push(&this->rx_buffer, msg);
		}
	}
	if (flags & CAN_SEND_FLAGS_SYNC) {
		ret = uv_can_send_sync(chn, msg);
	}
	if (flags & CAN_SEND_FLAGS_NORMAL) {
		ret = uv_ring_buffer_push(&this->tx_buffer, msg);
	}
	if (!(flags & CAN_SEND_FLAGS_NO_TX_CALLB) &&
			(this->tx_callback[chn] != NULL)) {
		this->tx_callback[chn](__uv_get_user_ptr(), msg, flags);
	}

	uv_enable_int();
	return ret;
}




uv_errors_e uv_can_pop_message(uv_can_channels_e channel, uv_can_message_st *message) {
	uv_errors_e ret = ERR_NONE;
	uv_disable_int();
	ret = uv_ring_buffer_pop(&this->rx_buffer, message);
	uv_enable_int();
	return ret;
}



uv_errors_e uv_can_reset(uv_can_channels_e channel) {
	Chip_SYSCTL_PeriphReset(RESET_CAN);

	return ERR_NONE;
}



/// @brief: Inner hal step function which is called in rtos hal task
void _uv_can_hal_step(unsigned int step_ms) {

#if C_CAN
	if (this->tx_pending > 0) {
		this->tx_pending -= step_ms;
	}
	else if (this->tx_pending == 0) {
		this->tx_pending = -1;
	}
#endif

	// send the next message from the buffer
#if CONFIG_CAN0
	_uv_can_hal_send(CAN0);
#endif
#if CONFIG_CAN1
	_uv_can_hal_send(CAN1);
#endif
}


#endif
