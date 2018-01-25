/*
 * canopen_pdo.h
 *
 *  Created on: Feb 22, 2017
 *      Author: usevolt
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
	CANOPEN_PDO_DISABLED = 0x80000000
} canopen_pdo_cob_id_mapping_e;



/// @brief: a nice way for defining a RXPDO
/// communication parameters in object dictionary
typedef struct {
	/// @brief: COB-ID for this PDO
	uint32_t cob_id;
	/// @brief: Transmission type. Currently only asynchronous
	/// transmissions are supported, so this should be set to 0xFF
	/// by the application.
	canopen_pdo_transmission_types_e transmission_type;
} canopen_rxpdo_com_parameter_st;
#define CANOPEN_RXPDO_COM_ARRAY_SIZE	2


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
	// reserved data for internal use
	int32_t _reserved;
	// the time delay for sending the PDO messages
	uint32_t event_timer;
} canopen_txpdo_com_parameter_st;
#define CANOPEN_TXPDO_COM_ARRAY_SIZE		5



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



/// @brief: Enables the PDO message by clearing the 31'th bit from the cob id
static inline void canopen_txpdo_enable(canopen_txpdo_com_parameter_st *pdo) {
	(pdo->cob_id &= ~(1 << 31));
}
static inline void canopen_rxpdo_enable(canopen_rxpdo_com_parameter_st *pdo) {
	(pdo->cob_id &= ~(1 << 31));
}



void _uv_canopen_pdo_init();

void _uv_canopen_pdo_reset();

void _uv_canopen_pdo_step(uint16_t step_ms);

void _uv_canopen_pdo_rx(const uv_can_message_st *msg);

/// @brief: If the given object is mapped to a transmit PDO, the PDO is updated
/// and transmitted immediately
void uv_canopen_pdo_mapping_update(uint16_t main_index, uint8_t subindex);



#endif

#endif /* UV_HAL_INC_CANOPEN_CANOPEN_PDO_H_ */
