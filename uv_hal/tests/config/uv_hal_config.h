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

#ifndef UV_HAL_TESTS_UV_HAL_CONFIG_H_
#define UV_HAL_TESTS_UV_HAL_CONFIG_H_

/// @file: uv_hal configuration used *only* by the host unit test build.
///
/// Normally uv_hal_config.h is supplied by the application that consumes uv_hal.
/// The test build supplies its own so that the test suite is self contained and
/// can be run from any device repository (or from a bare uv_hal checkout)
/// without depending on any particular application's configuration.
///
/// Everything that would pull in real hardware is disabled here. Only the
/// hardware independent modules are configured, which is exactly the set of
/// modules the unit tests cover - see README.md for why.


/* Build for the host, not for an MCU. */
#define CONFIG_TARGET_LINUX							1

/* uv_memory.h requires an interface revision; the tests do not revision. */
#define CONFIG_INTERFACE_REVISION					0

/* uv_utilities.c includes uv_rtos.h, which requires a FreeRTOS configuration to
 * be present. The tests never start the scheduler; the only RTOS symbol that is
 * actually referenced at link time is stubbed in stubs/rtos_stubs.c. */
#define CONFIG_RTOS									1
#define CONFIG_RTOS_HEAP_SIZE						(configMINIMAL_STACK_SIZE * 10)
#define CONFIG_UV_BOOTLOADER						0

/* No hardware peripherals in a unit test build. */
#define CONFIG_TERMINAL								0
#define CONFIG_NON_VOLATILE_MEMORY					0

/* Modules under test. */
#define CONFIG_PID									1
#define CONFIG_JSON									1

/* The CANopen SDO server and client are under test. CAN is configured only far
 * enough for the headers to be well formed and for the protocol code to compile;
 * no hardware is touched. uv_can_send() is stubbed in stubs/canopen_stubs.c,
 * which is what lets a test see the frames the stack puts on the "bus". */
#define CONFIG_CAN									1
#define CONFIG_CAN0									1
#define CONFIG_CAN1									0
#define CONFIG_CAN0_BAUDRATE						250000
#define CONFIG_CAN0_TX_BUFFER_SIZE					32
#define CONFIG_CAN0_RX_BUFFER_SIZE					64
#define CONFIG_CAN0_RX_STDMSG_SIZE					20
#define CONFIG_CAN0_RX_STDRANGE_SIZE				10
#define CONFIG_CAN0_RX_EXTMSG_SIZE					10
#define CONFIG_CAN0_RX_EXTRANGE_SIZE				10
#define CONFIG_CAN_LOG								0
#define CONFIG_CAN_ERROR_LOG						0

#define CONFIG_CANOPEN								1
#define CONFIG_CANOPEN_CHANNEL						uv_can_get_dev()
#define CONFIG_CANOPEN_DEFAULT_NODE_ID				0x0A
#define CONFIG_CANOPEN_EMCY_INHIBIT_TIME_MS			500
#define CONFIG_CANOPEN_EMCY_RX_BUFFER_SIZE			3
#define CONFIG_CANOPEN_VENDOR_ID					CANOPEN_USEVOLT_VENDOR_ID
#define CONFIG_CANOPEN_PRODUCT_CODE					0
#define CONFIG_CANOPEN_REVISION_NUMBER				0
#define CONFIG_CANOPEN_LOG							0
#define CONFIG_CANOPEN_NMT_SLAVE					0
#define CONFIG_CANOPEN_NMT_MASTER					1
#define CONFIG_CANOPEN_AUTO_PREOPERATIONAL			1
#define CONFIG_CANOPEN_RXPDO_COUNT					1
#define CONFIG_CANOPEN_TXPDO_COUNT					1
#define CONFIG_CANOPEN_RXPDO_TIMEOUT_MS				500
#define CONFIG_CANOPEN_SDO_SERVER					1
#define CONFIG_CANOPEN_SDO_SYNC						1
#define CONFIG_CANOPEN_SDO_SEGMENTED				1
#define CONFIG_CANOPEN_SDO_BLOCK_TRANSFER			0
#define CONFIG_CANOPEN_SDO_TIMEOUT_MS				1000
#define CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS			uv_test_obj_dict
#define CONFIG_CANOPEN_OBJ_DICT_APP_PARAMS_COUNT	uv_test_obj_dict_len
#define CONFIG_CANOPEN_OBJ_DICT_IN_RISING_ORDER		1
#define CONFIG_CANOPEN_HEARTBEAT_PRODUCER			1
#define CONFIG_CANOPEN_HEARTBEAT_CONSUMER			0
#define CONFIG_CANOPEN_HEARTBEAT_PRODUCER_COUNT		1
#define CONFIG_CANOPEN_HEARTBEAT_PRODUCER_NODEID1	0
#define CONFIG_CANOPEN_HEARTBEAT_PRODUCER_TIME1		600
#define CONFIG_CANOPEN_PRODUCER_HEARTBEAT_TIME_MS	1000
#define CONFIG_CANOPEN_UPDATE_PDO_MAPPINGS_ON_NODEID_WRITE 1

/* The CANopen sources include CONFIG_MAIN_H for the application's device
 * struct. config/main.h is a placeholder that supplies only what they need. */
#define CONFIG_MAIN_H								"main.h"

/* uv_utilities.h declares `extern CONFIG_APP_ST;` for the application's global
 * device struct, without including CONFIG_MAIN_H - so this names the struct tag
 * as an incomplete type, exactly as the real applications do. The definition is
 * in config/main.h and the instance in stubs/canopen_stubs.c. */
#define CONFIG_APP_ST								struct _dev_st dev

/* The CANopen object dictionary places the standard objects (node id, baudrate,
 * heartbeat times, PDO parameters) inside the application's non-volatile
 * region. */
#define CONFIG_NON_VOLATILE_START					dev.data_start
#define CONFIG_NON_VOLATILE_END						dev.data_end


#endif /* UV_HAL_TESTS_UV_HAL_CONFIG_H_ */
