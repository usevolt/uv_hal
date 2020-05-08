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

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_EMCY_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_EMCY_H_


#include <uv_hal_config.h>
#include "canopen/canopen_common.h"


#if CONFIG_CANOPEN


typedef enum {
	CANOPEN_EMCY_NO_ERROR				= (0x0000),
	CANOPEN_EMCY_GENERIC 				= (0x1000),
	CANOPEN_EMCY_CURRENT_GENERIC		= (0x2000),
	CANOPEN_EMCY_CURRENT_INPUT 			= (0x2100),
	CANOPEN_EMCY_CURRENT_DEVICE			= (0x2200),
	CANOPEN_EMCY_CURRENT_OUTPUT			= (0x2300),
	CANOPEN_EMCY_VOLTAGE_GENERIC		= (0x3000),
	CANOPEN_EMCY_VOLTAGE_MAINS			= (0x3100),
	CANOPEN_EMCY_VOLTAGE_DEVICE			= (0x3200),
	CANOPEN_EMCY_VOLTAGE_OUTPUT			= (0x3300),
	CANOPEN_EMCY_TEMP_GENERIC			= (0x4000),
	CANOPEN_EMCY_TEMP_AMBIENT			= (0x4100),
	CANOPEN_EMCY_TEMP_DEVICE			= (0x4200),
	CANOPEN_EMCY_HARDWARE_GENERIC		= (0x5000),
	CANOPEN_EMCY_SOFTWARE_GENERIC		= (0x6000),
	CANOPEN_EMCY_SOFTWARE_INTERNAL		= (0x6100),
	CANOPEN_EMCY_SOFTWARE_USER			= (0x6200),
	CANOPEN_EMCY_DATA_SET_GENERIC		= (0x6300),
	CANOPEN_EMCY_ADDITIONAL_MODULES		= (0x7000),
	CANOPEN_EMCY_MONITORING_GENERIC		= (0x8000),
	CANOPEN_EMCY_COMMUNICATION_GENERIC	= (0x8100),
	CANOPEN_EMCY_CAN_OVERRUN			= (0x8110),
	CANOPEN_EMCY_CAN_ERROR_PASSIVE		= (0x8120),
	CANOPEN_EMCY_LIFE_GUARD_HEARTBEAT	= (0x8130),
	CANOPEN_EMCY_RECOVERED_FROM_BUS_OFF = (0x8140),
	CANOPEN_EMCY_CAN_ID_COLLISION		= (0x8150),
	CANOPEN_EMCY_PROTOCOL_GENERIC		= (0x8200),
	CANOPEN_EMCY_PDO_LENGTH_ERROR		= (0x8210),
	CANOPEN_EMCY_PDO_LENGTH_EXCEEDED	= (0x8220),
	CANOPEN_EMCY_DAM_MPDO_NOT_PROCESSED = (0x8230),
	CANOPEN_EMCY_UNEXPECT_SYNC_LENGTH	= (0x8240),
	CANOPEN_EMCY_RPDO_TIMEOUT			= (0x8250),
	CANOPEN_EMCY_EXTERNAL_ERROR			= (0x9000),
	CANOPEN_EMCY_ADDITIONAL_FUNCTIONS	= (0xF000),
	CANOPEN_EMCY_DEVICE_SPECIFIC		= (0xFF00)


} _uv_emcy_codes_e;
typedef uint64_t uv_emcy_codes_e;






/// @brief: Defines a EMCY message structure.
/// This is used as a EMCY callback parameter, where
/// the user application can read the EMCY error codes.
typedef struct {
	/// @brief: Device specific data
	uint32_t data;
	/// @brief: CANopen EMCY error code
	uint16_t error_code;
	/// @brief: The sender node ID of this EMCY message
	uint8_t node_id;
} canopen_emcy_msg_st;



/// @brief: Emits an emergency message instantly
///
/// @param err_code: 2 MSB bytes, defining the error code category
/// @param data: Optional device specific data
void uv_canopen_emcy_send(const uv_emcy_codes_e err_code, uint32_t data);


/// @brief: Gets the oldest received EMCY message and returns true.
///
/// @return: ERR_NONE if there was an EMCY message to be returned, an error otherwise
uv_errors_e uv_canopen_emcy_get(canopen_emcy_msg_st *dest);




void _uv_canopen_emcy_init(void);
void _uv_canopen_emcy_reset(void);
void _uv_canopen_emcy_rx(const uv_can_message_st *msg);
void _uv_canopen_emcy_step(uint16_t step_ms);


#endif /* UV_HAL_INC_CANOPEN_CANOPEN_EMCY_H_ */
#endif
