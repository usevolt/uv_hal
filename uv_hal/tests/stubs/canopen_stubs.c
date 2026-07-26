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

#include "canopen_test_env.h"

#include <string.h>

#include "uv_canopen.h"
#include "uv_memory.h"
#include "main.h"

/// @file: The fake CAN bus and the environment the CANopen stack expects: the
/// application device struct, the object dictionary, and the handful of HAL
/// entry points that would otherwise reach hardware.
///
/// Keeping this file short is deliberate. Every entry is something the protocol
/// code depends on beyond its own logic.


/// @brief: The application device struct. CONFIG_NON_VOLATILE_START points into
/// this, and the object dictionary places the standard CANopen objects there.
dev_st dev;

/// @brief: The CANopen stack's state. Normally defined by uv_canopen.c, which
/// the tests do not link: it drags in the NMT, PDO, heartbeat and EMCY modules
/// and the CAN hardware behind them, none of which the SDO protocol needs.
_uv_canopen_st _canopen;

/// @brief: uv_memory.h expects the application to carry a project name.
const char uv_projname[] = "uv_hal_tests";


/* ---------------------------------------------------------------------------
 * test object dictionary
 * ------------------------------------------------------------------------ */

canopen_test_data_st canopen_test_data;


const canopen_object_st uv_test_obj_dict[] = {
		{
				.main_index = TEST_OBJ_U8,
				.sub_index = 0,
				.type = CANOPEN_UNSIGNED8,
				.permissions = CANOPEN_RW,
				.data_ptr = &canopen_test_data.u8
		},
		{
				.main_index = TEST_OBJ_U16,
				.sub_index = 0,
				.type = CANOPEN_UNSIGNED16,
				.permissions = CANOPEN_RW,
				.data_ptr = &canopen_test_data.u16
		},
		{
				.main_index = TEST_OBJ_U32,
				.sub_index = 0,
				.type = CANOPEN_UNSIGNED32,
				.permissions = CANOPEN_RW,
				.data_ptr = &canopen_test_data.u32
		},
		{
				.main_index = TEST_OBJ_RO,
				.sub_index = 0,
				.type = CANOPEN_UNSIGNED32,
				.permissions = CANOPEN_RO,
				.data_ptr = &canopen_test_data.ro
		},
		{
				.main_index = TEST_OBJ_WO,
				.sub_index = 0,
				.type = CANOPEN_UNSIGNED32,
				.permissions = CANOPEN_WO,
				.data_ptr = &canopen_test_data.wo
		},
		{
				.main_index = TEST_OBJ_STRING,
				.sub_index = 0,
				.type = CANOPEN_STRING,
				.permissions = CANOPEN_RW,
				.string_len = TEST_STRING_LEN,
				.data_ptr = canopen_test_data.string
		},
		{
				.main_index = TEST_OBJ_ARRAY32,
				.array_max_size = TEST_ARRAY_LEN,
				.type = CANOPEN_ARRAY32,
				.permissions = CANOPEN_RW,
				.data_ptr = canopen_test_data.array32
		},
		{
				.main_index = TEST_OBJ_ARRAY8,
				.array_max_size = TEST_ARRAY_LEN,
				.type = CANOPEN_ARRAY8,
				.permissions = CANOPEN_RW,
				.data_ptr = canopen_test_data.array8
		}
};


uint32_t uv_test_obj_dict_len(void) {
	return sizeof(uv_test_obj_dict) / sizeof(uv_test_obj_dict[0]);
}


/* ---------------------------------------------------------------------------
 * the fake bus
 * ------------------------------------------------------------------------ */

static uv_can_message_st tx_msgs[CANOPEN_TEST_TX_MAX];
static uint32_t tx_count = 0;
/// @brief: Counts frames the stack tried to send after the capture filled up, so
/// that a test can never mistake a dropped frame for one that was never sent.
static uint32_t tx_overflow = 0;

static uint32_t write_callb_count = 0;
static uint16_t write_callb_mindex = 0;
static uint8_t write_callb_sindex = 0;
static uint32_t read_callb_count = 0;
static uint16_t read_callb_mindex = 0;
static uint8_t read_callb_sindex = 0;


static void test_write_callb(uint16_t mindex, uint8_t sindex) {
	write_callb_count++;
	write_callb_mindex = mindex;
	write_callb_sindex = sindex;
}


static void test_read_callb(uint16_t mindex, uint8_t sindex) {
	read_callb_count++;
	read_callb_mindex = mindex;
	read_callb_sindex = sindex;
}


void canopen_test_env_reset(void) {
	memset(&dev, 0, sizeof(dev));
	memset(&canopen_test_data, 0, sizeof(canopen_test_data));
	memset(&_canopen, 0, sizeof(_canopen));
	memset(tx_msgs, 0, sizeof(tx_msgs));
	tx_count = 0;
	tx_overflow = 0;
	write_callb_count = 0;
	write_callb_mindex = 0;
	write_callb_sindex = 0;
	read_callb_count = 0;
	read_callb_mindex = 0;
	read_callb_sindex = 0;

	_canopen.current_node_id = CANOPEN_TEST_NODEID;
	_canopen.state = CANOPEN_OPERATIONAL;
	dev.data_start.id = CANOPEN_TEST_NODEID;

	// the identity object at CONFIG_CANOPEN_IDENTITY_INDEX reads straight out of
	// here. uv_canopen_init() fills it in on a real device; the tests do not
	// link uv_canopen.c, so the same values are set here.
	_canopen.device_type = 'U';
	_canopen.identity.vendor_id = CONFIG_CANOPEN_VENDOR_ID;
	_canopen.identity.product_code = CONFIG_CANOPEN_PRODUCT_CODE;
	_canopen.identity.revision_number = CONFIG_CANOPEN_REVISION_NUMBER;

	_uv_canopen_sdo_init();
	_uv_canopen_sdo_reset();
	_uv_canopen_sdo_server_add_write_callb(&test_write_callb);
	_uv_canopen_sdo_server_add_read_callb(&test_read_callb);

	// the init sequence itself is not what any test is looking at
	tx_count = 0;
	tx_overflow = 0;
}


uint32_t canopen_test_tx_count(void) {
	return tx_count;
}


const uv_can_message_st *canopen_test_tx_at(uint32_t index) {
	const uv_can_message_st *ret = NULL;

	if (index < tx_count) {
		ret = &tx_msgs[index];
	}
	return ret;
}


const uv_can_message_st *canopen_test_tx_last(void) {
	const uv_can_message_st *ret = NULL;

	if (tx_count != 0) {
		ret = &tx_msgs[tx_count - 1];
	}
	return ret;
}


void canopen_test_tx_clear(void) {
	tx_count = 0;
	tx_overflow = 0;
}


uint32_t canopen_test_write_callb_count(uint16_t *mindex, uint8_t *sindex) {
	if (mindex != NULL) {
		*mindex = write_callb_mindex;
	}
	if (sindex != NULL) {
		*sindex = write_callb_sindex;
	}
	return write_callb_count;
}


uint32_t canopen_test_read_callb_count(uint16_t *mindex, uint8_t *sindex) {
	if (mindex != NULL) {
		*mindex = read_callb_mindex;
	}
	if (sindex != NULL) {
		*sindex = read_callb_sindex;
	}
	return read_callb_count;
}


/* ---------------------------------------------------------------------------
 * uv_hal entry points that would reach hardware
 * ------------------------------------------------------------------------ */

uv_errors_e uv_can_send_flags(uv_can_channels_e chn, uv_can_msg_st *msg,
		can_send_flags_e flags) {
	uv_errors_e ret = ERR_NONE;

	// _uv_canopen_sdo_send() sends a reply addressed to this node twice: once
	// on the bus and once locally, so that a local SDO client sees it too. The
	// local copy is not a bus frame, so it is not captured - a test asserting
	// on "the frames the device put on the wire" should not see each of them
	// twice.
	if ((flags & CAN_SEND_FLAGS_LOCAL) == 0) {
		if (tx_count < CANOPEN_TEST_TX_MAX) {
			tx_msgs[tx_count] = *msg;
			tx_count++;
		}
		else {
			tx_overflow++;
			ret = ERR_BUFFER_OVERFLOW;
		}
	}
	return ret;
}


uv_errors_e uv_can_config_rx_message(uv_can_channels_e channel,
		unsigned int id, unsigned int mask, uv_can_msg_types_e type) {
	return ERR_NONE;
}


uv_can_channels_e uv_can_get_dev(void) {
	return 0;
}


canopen_node_states_e uv_canopen_get_state(void) {
	return _canopen.state;
}


static canopen_pdo_com_parameter_st test_rxpdo_com;
static canopen_pdo_com_parameter_st test_txpdo_com;


canopen_pdo_com_parameter_st *uv_canopen_rxpdo_get_com(int16_t rxpdo) {
	return &test_rxpdo_com;
}


canopen_pdo_com_parameter_st *uv_canopen_txpdo_get_com(int16_t txpdo) {
	return &test_txpdo_com;
}


void uv_rtos_task_delay(unsigned int ms) {
}


void vPortYield(void);

void vPortYield(void) {
}
