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

#ifndef UV_HAL_INC_CANOPEN_CANOPEN_PDO_H_
#define UV_HAL_INC_CANOPEN_CANOPEN_PDO_H_


#include <uv_hal_config.h>
#include "canopen/canopen_common.h"
#include "uv_can.h"


#if CONFIG_CANOPEN

enum {
	/// @brief: PDO is transmitted asynchronously
	CANOPEN_PDO_TRANSMISSION_ASYNC = 0xFF,
};
typedef uint32_t canopen_pdo_transmission_types_e;



/// @brief: Values for PDO communication parameters
/// These are used when defining PDO COB-ID's. Currently this
/// CANopen stack doesn't support RTR. Also all PDO's are
/// asynchronous.
typedef enum {
	CANOPEN_PDO_ENABLED = 0,
	/// @brief: PDO transmission is disabled. This should be OR'red with the PDO
	/// communication parameter's COB-ID field.
	CANOPEN_PDO_DISABLED = 0x80000000,
	CANOPEN_PDO_EXT = (1 << 29)
} canopen_pdo_cob_id_mapping_e;

// when set, PDO COB-iD linkage to our NODE-ID is broken.
// Linkage is also broken by manually modifying NODE-ID via SDO
#define CANOPEN_PDO_RESERVED_FLAGS_BREAKNODEIDLINKAGE	(1 << 0)



/// @brief: a nice way for defining a TXPDO
/// communication parameters in object dictionary
typedef struct {
	/// @brief: COB-ID for this PDO
	uint32_t cob_id;
	/// @brief: Transmission type. Currently only asynchronous
	/// transmissions are supported, so this must be set to PDO_TRANSMISSION_ASYNC
	/// by the application.
	canopen_pdo_transmission_types_e transmission_type;
	// minimum time the pdo can be sent
	int32_t inhibit_time;
	// reserved data for internal use, used to detect the default NODE-ID linkage
	int32_t reserved;
	// the time delay for sending the PDO messages
	uint32_t event_timer;
} canopen_pdo_com_parameter_st;
#define CANOPEN_PDO_COM_ARRAY_SIZE		5
#define CANOPEN_PDO_COM_ARRAY_TYPE			CANOPEN_ARRAY32




/// @brief: A nice way for defining a CANopen PDO mapping parameter
/// PDO mapping parameter object should be an array of these
typedef struct {
	/// @brief: Mapped object's main_index
	uint16_t main_index;
	/// @brief: Mapped object's sub_index
	uint8_t sub_index;
	/// @brief: Mapped bit length
	/// @note: This length is in bits!
	uint8_t length;
} canopen_pdo_mapping_st;



typedef struct {
	canopen_pdo_mapping_st mappings[CONFIG_CANOPEN_PDO_MAPPING_COUNT];
} canopen_pdo_mapping_parameter_st;
#define CANOPEN_PDO_MAPPING_PARAMETER_TYPE	CANOPEN_ARRAY32



/// @brief: Enables the PDO message by clearing the 31'th bit from the cob id
static inline void canopen_txpdo_enable(canopen_pdo_com_parameter_st *pdo) {
	(pdo->cob_id &= ~(1 << 31));
}
static inline void canopen_rxpdo_enable(canopen_pdo_com_parameter_st *pdo) {
	(pdo->cob_id &= ~(1 << 31));
}



void _uv_canopen_pdo_init();

void _uv_canopen_pdo_reset();

void _uv_canopen_pdo_step(uint16_t step_ms);

void _uv_canopen_pdo_rx(const uv_can_message_st *msg);

/// @brief: If the given object is mapped to a transmit PDO, the PDO is updated
/// and transmitted immediately
void uv_canopen_pdo_mapping_update(uint16_t main_index, uint8_t subindex);


/// @brief: Returns a pointer to the PDO mapping parameter of the RXPDO of and index of *rxpdo*
canopen_pdo_mapping_parameter_st *uv_canopen_rxpdo_get_mapping(uint16_t rxpdo);

/// @brief: Returns a pointer to the PDO mapping parameter of the TXPDO of and index of *txpdo*
canopen_pdo_mapping_parameter_st *uv_canopen_txpdo_get_mapping(uint16_t txpdo);


/// @brief: Returns the RXPDO communication parameter assigned to RXPDO with an index of *rxpdo*
canopen_pdo_com_parameter_st *uv_canopen_rxpdo_get_com(uint16_t rxpdo);

canopen_pdo_com_parameter_st *uv_canopen_txpdo_get_com(uint16_t txpdo);

/// @brief Configures the internal pdo mapping pointers according to obj dict mapping parameter
///
/// @param mapping_param_index Obj dict main index of the pdo mapping parameter
/// @param ptr_bfr Pointer to mapping pointer array of CONFIG_CANOPEN_PDO_MAPPING_COUNT len
/// @param permissions: CANOPEN_RO for TXPDO, CANOPEN_WO for RXPDO
void _uv_canopen_pdo_mapping_ptr_conf(canopen_pdo_mapping_parameter_st *mapping_param,
		uint8_t **ptr_bfr, canopen_permissions_e permissions);


#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_PDO_H_ */
