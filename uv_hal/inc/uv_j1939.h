/*
 * uv_j1939.h
 *
 *  Created on: May 5, 2023
 *      Author: usevolt
 */

#ifndef HAL_UV_HAL_INC_UV_J1939_H_
#define HAL_UV_HAL_INC_UV_J1939_H_

#include "uv_can.h"
#include <stdbool.h>

/// @file Defines for SAE J1939 protocol


#define UV_J1939_PGN_DM1			(0xFECA)

#define UV_J1939_PGN_MASK			(0x3FFFF00)

typedef enum {
	J1939_TRANSPORT_CONNECTION_MANAGEMENT = 0x18ECFF00,
	J1939_TRANSPORT_DATA_TRANSFER = 0x18EBFF00,
	J1939_ENGINE_DIAGNOSTICS_MSG = 0x18FECA00,
	J1939_REQ_ENGINE_HOURS = 0x18FEE500
} j1939_ids_e;

typedef enum {
	// diagnostics message 1
	J1939_PGN_DM1 = 65226
} j1939_pgn_e;



/// @brief: Defines the structure for j1939 DTC in diagnostics PGNs
typedef struct {
	uint32_t spn;
	uint8_t fmi;
	uint8_t cm;
	uint8_t oc;
} j1939_dtc_st;

/// @brief: Parses lamp information from DM1 raw data received via package protocol
static inline uint8_t uv_j1939_dm1_get_lampinfo(uint8_t *raw_data, uint16_t data_len) {
	return (data_len >= 1) ? raw_data[0] : 0;
}

/// @brief: Parses DM1 raw data and returns DTC structure of error code with index
/// *dtc_index*.
uv_errors_e uv_j1939_dm1_get_dtc(j1939_dtc_st *dest,
								 uint8_t *raw_data,
								 uint16_t data_len,
								 uint8_t dtc_index);

typedef struct {
	uint16_t pgn;
	uint16_t byte_count;
	uint16_t byte_index;
	void *dest;
	uint16_t dest_max_count;
} uv_j1939_transport_st;


/// @brief: Initializes the receive of transport protocol
///
/// @param dest: Destination pointer where data is stored
/// @param pgn: Requested PGN to receive. If any PGN can be received, set to zero.
void uv_j1939_transport_init(uv_j1939_transport_st *this,
						  void *dest, uint16_t dest_max_count,
						  uint16_t pgn);

/// @brief: Stops receiving the transport transfer
static inline void uv_j1939_transport_clear(uv_j1939_transport_st *this) {
	this->byte_index = 0;
	this->byte_count = 0;
}

static inline uint16_t uv_j1939_transport_get_pgn(uv_j1939_transport_st *this) {
	return this->pgn;
}


/// @brief: Returns true if the transport was finished
static inline bool uv_j1939_transport_is_ready(uv_j1939_transport_st *this) {
	return ((this->byte_index == this->byte_count ||
			this->byte_index == this->dest_max_count) &&
			this->byte_count != 0);
}

/// @brief: Returns the written byte count
static inline uint16_t uv_j1939_transport_get_byte_count(uv_j1939_transport_st *this) {
	return this->byte_index;
}

/// @brief: RX function for receiving transport protocol
///
/// @return True when the whole message was received, false otherwise
bool uv_j1939_transport_rx(uv_j1939_transport_st *this, uv_can_msg_st *msg);




/// @brief: Returns the parameter group number from the message. 0 in case of error.
static inline uint16_t uv_j1939_get_pgn(uv_can_msg_st *msg) {
	return (msg->type == CAN_EXT) ? ((msg->id & 0xFFFF00) >> 8) : 0;
}


static inline uint8_t uv_j1939_get_source_address(uv_can_msg_st *msg) {
	return (msg->id & 0xFF);
}


#endif /* HAL_UV_HAL_INC_UV_J1939_H_ */
