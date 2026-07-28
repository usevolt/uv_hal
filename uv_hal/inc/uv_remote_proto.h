/*
 * uv_remote_proto.h
 *
 *  Wire format of the Usevolt REMOTE protocol.
 */

#ifndef UV_HAL_INC_UV_REMOTE_PROTO_H_
#define UV_HAL_INC_UV_REMOTE_PROTO_H_

/// @file: The REMOTE message framing, shared by every end of the link.
///
/// A device speaks this over whatever transport it has — a raw XB3 byte pipe or
/// the payloads of MQTT messages — and a sink (the uvcan desktop tool) speaks it
/// back. Both ends need the identical definitions, and they live in uv_hal
/// because that is the only module every project shares: the uvcan control
/// modules are per-device, so a protocol header there could only be duplicated.
///
/// Nothing here describes an implementation. The transport lives in the uvcan
/// remote module (remote.h / remote_iot.c / remote_rf.c) and the UI command
/// stream carried by REMOTE_MSG_TYPE_UI is described in uv_ui_remote.h.

#include <stdint.h>
#include "uv_utilities.h"
#include "uv_can.h"


/// @brief: Every message starts with this byte, which is also what the framer
/// hunts for when it has to resynchronize.
#define REMOTE_MSG_START_BYTE		(0x85)


/// @brief: Usevolt REMOTE message protocol message types.
/// Message starts with 0x85 byte, second byte defines the type.
typedef enum {
	// This message requests the receiver to connect to sender.
	// Message contains 16 bytes of data, the sender's serial and receiver's nodeid.
	// After this, the receiver tests if the sender is on the same CAN-bus by reading
	// SDO data. If sender is not found on CAN-bus, the receiver doesn't respond
	// and does not start sending message data
	REMOTE_MSG_TYPE_CONNECT_REQ = 0,
	// Response to CONNECT_REQ message, wihtout any additional data
	REMOTE_MSG_TYPE_CONNECT_RESP,
	// Message contains individual can message.
	// First byte is REMOTE_MSG_START_BYTE,
	// Second is REMOTE_MSG_TYPE_CAN,
	// Third is data_len,
	// bytes 4 - 8 is COB-ID with message type as defined in CANopen
	// then data, according to data_len
	REMOTE_MSG_TYPE_CAN,
	// Add a rx message to mask filter
	// First byte is REMOTE_MSG_START_BYTE,
	// then remote_can_rxconf_st defining the message that should be received
	REMOTE_MSG_TYPE_RXCONF,
	// clears all current RX confs
	REMOTE_MSG_TYPE_RXCLEAR,
	// rxconfiguration done, remote device should move to CONNECTED state
	REMOTE_MSG_TYPE_RXDONE,
	// UI mirroring: a chunk of the compact UI command stream (source -> sink).
	// [0x85][UI][chunk_len][ui_flags][cmd bytes: chunk_len]. The sink appends
	// chunk payloads into a reassembly buffer and decodes on FRAME_END.
	REMOTE_MSG_TYPE_UI,
	// Reverse input event (sink -> source), fixed length:
	// [0x85][UI_INPUT][action][x:2][y:2][scroll][key]
	REMOTE_MSG_TYPE_UI_INPUT,
	// Sink -> source: request an asset (bitmap/font) it lacks by id:
	// [0x85][UI_ASSET_REQ][asset_kind][id:2]
	REMOTE_MSG_TYPE_UI_ASSET_REQ,
	// Chunked asset stream (source -> sink), same chunk shape as UI
	REMOTE_MSG_TYPE_UI_ASSET,
	// IoT control (server -> device): the absolute mask of REMOTE_IOT_FEATURE_*
	// the server wants enabled — not a set/clear command, so a re-send always
	// converges and no ack sequencing is needed.
	// [0x85][IOT_CTRL][feature_mask]
	REMOTE_MSG_TYPE_IOT_CTRL,
	// IoT status (device -> server), sent whenever the applied feature mask or
	// the submodule state changes, so the server sees what actually took
	// effect (conf may veto what it asked for).
	// [0x85][IOT_STATUS][applied_mask][iot_state]
	REMOTE_MSG_TYPE_IOT_STATUS,
	// Geometry of the mirrored display (device -> sink), sent whenever UI
	// mirroring is switched on and before the first frame of that session, so
	// the sink can size its window before it has anything to draw. The command
	// stream itself carries no absolute dimensions.
	// [0x85][UI_INFO][width:2][height:2]
	REMOTE_MSG_TYPE_UI_INFO,
	REMOTE_MSG_TYPE_COUNT
} remote_msg_types_e;


/// @brief: Remotely switchable features of the iot submodule. Both default to
/// off at boot and after every broker reconnect; only the server turns them on,
/// with REMOTE_MSG_TYPE_IOT_CTRL.
#define REMOTE_IOT_FEATURE_UI			(1 << 0)
#define REMOTE_IOT_FEATURE_CAN			(1 << 1)
#define REMOTE_IOT_FEATURE_ALL			(REMOTE_IOT_FEATURE_UI | \
										 REMOTE_IOT_FEATURE_CAN)


// --- UI mirroring wire constants --------------------------------------------

// Max UI command bytes carried per chunk. Kept so that a whole chunk
// (REMOTE_UI_MSG_LEN below) still fits a uint8_t length field.
#define REMOTE_UI_CHUNK_MAX_LEN			200
// On-wire length of a UI / UI_ASSET chunk carrying *chunk* command bytes:
// [start][type][chunk_len][ui_flags] + chunk bytes.
#define REMOTE_UI_MSG_LEN(chunk)		(4 + (chunk))
#define REMOTE_UI_MSG_MAX_LEN			REMOTE_UI_MSG_LEN(REMOTE_UI_CHUNK_MAX_LEN)
// ui_flags bits
#define REMOTE_UI_FLAG_FRAME_START		(1 << 0)
#define REMOTE_UI_FLAG_FRAME_END		(1 << 1)
#define REMOTE_UI_FLAG_RGB565			(1 << 2)	// reserved for a future 2-byte color mode


/// @brief: Structure for masking can messages that should be sent via remote.
/// Copied verbatim in and out of a RXCONF message, so it is part of the wire
/// format rather than an implementation detail.
typedef struct {
	uint32_t id;
	uint32_t mask;
	uv_can_msg_types_e type;
} remote_can_rxconf_st;


#define REMOTE_MSG_TYPE_CONNECT_LEN				(sizeof(uint64_t) + sizeof(uint8_t) + 2)
#define REMOTE_MSG_TYPE_CONNECT_SERIAL_INDEX	2
#define REMOTE_MSG_TYPE_CONNECT_NODEID_INDEX	10
#define REMOTE_MSG_TYPE_CONNECT_RESP_LEN		(2)
#define REMOTE_MSG_TYPE_CAN_LEN(data_len)		(2 + 1 + 4 + (data_len))
#define REMOTE_MSG_TYPE_CAN_MAX_LEN				(2 + 1 + 4 + 8)
#define REMOTE_MSG_TYPE_RXCONF_LEN				(sizeof(remote_can_rxconf_st) + 2)
#define REMOTE_MSG_TYPE_RXCLEAR_LEN				2
#define REMOTE_MSG_TYPE_RXDONE_LEN				2
// UI / UI_ASSET are variable length; the len table holds their max and the rx
// state machine patches the exact length from the chunk_len byte (index 2).
#define REMOTE_MSG_TYPE_UI_LEN					REMOTE_UI_MSG_MAX_LEN
#define REMOTE_MSG_TYPE_UI_INPUT_LEN			9
#define REMOTE_MSG_TYPE_UI_ASSET_REQ_LEN		5
#define REMOTE_MSG_TYPE_UI_ASSET_LEN			REMOTE_UI_MSG_MAX_LEN
#define REMOTE_MSG_TYPE_IOT_CTRL_LEN			3
#define REMOTE_MSG_TYPE_IOT_STATUS_LEN			4
#define REMOTE_MSG_TYPE_UI_INFO_LEN				6
#define REMOTE_MSG_TYPE_MAX_LEN					(MAX(\
		REMOTE_MSG_TYPE_UI_LEN, \
		MAX(REMOTE_MSG_TYPE_CONNECT_LEN, \
		MAX(REMOTE_MSG_TYPE_CAN_MAX_LEN,\
				MAX(REMOTE_MSG_TYPE_RXCONF_LEN,\
						REMOTE_MSG_TYPE_RXCLEAR_LEN)))))


static inline const char *remote_msg_type_to_str(remote_msg_types_e type) {
	const char *ret = "COUNT";
	switch (type) {
	case REMOTE_MSG_TYPE_CONNECT_REQ:
		ret = "CONNECT_REQ";
		break;
	case REMOTE_MSG_TYPE_CONNECT_RESP:
		ret = "CONNECT_RESP";
		break;
	case REMOTE_MSG_TYPE_CAN:
		ret = "CAN";
		break;
	case REMOTE_MSG_TYPE_RXCONF:
		ret = "RXCONF";
		break;
	case REMOTE_MSG_TYPE_RXCLEAR:
		ret = "RXCLEAR";
		break;
	case REMOTE_MSG_TYPE_RXDONE:
		ret = "RXDONE";
		break;
	case REMOTE_MSG_TYPE_UI:
		ret = "UI";
		break;
	case REMOTE_MSG_TYPE_UI_INPUT:
		ret = "UI_INPUT";
		break;
	case REMOTE_MSG_TYPE_UI_ASSET_REQ:
		ret = "UI_ASSET_REQ";
		break;
	case REMOTE_MSG_TYPE_UI_ASSET:
		ret = "UI_ASSET";
		break;
	case REMOTE_MSG_TYPE_IOT_CTRL:
		ret = "IOT_CTRL";
		break;
	case REMOTE_MSG_TYPE_IOT_STATUS:
		ret = "IOT_STATUS";
		break;
	case REMOTE_MSG_TYPE_UI_INFO:
		ret = "UI_INFO";
		break;
	default:
		break;
	}
	return ret;
}


#endif /* UV_HAL_INC_UV_REMOTE_PROTO_H_ */
