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
	// Sink -> source: request an asset (bitmap/font) it lacks:
	// [0x85][UI_ASSET_REQ][asset_kind][id:4]
	// A font is asked for by the font_id the STRING op already carries; a
	// bitmap by the hash of its file name.
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
	// The device closes the session (device -> server), carrying nothing but
	// the type. Sent when the machine operator ends remote access from the
	// device's own screen, which also switches every feature off locally — so
	// this is not what stops the stream, it is what lets the server say why the
	// stream stopped instead of showing a view that has silently gone dead.
	// [0x85][CLOSE]
	REMOTE_MSG_TYPE_CLOSE,
	// Per-class CAN forwarding statistics (device -> sink), sent about once a
	// second while CAN forwarding is on. What the far end cannot work out for
	// itself: how much the device chose not to send, and which class it came
	// out of. Payload is remote_can_stats_st verbatim.
	// [0x85][CAN_STATS][remote_can_stats_st]
	REMOTE_MSG_TYPE_CAN_STATS,
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
///
/// A mask of zero means "every id of this type": it is the only way to ask for
/// a whole bus, and the receiving device switches its acceptance filter to
/// accept-all for it rather than trying to express it as an id range.
typedef struct {
	uint32_t id;
	uint32_t mask;
	uv_can_msg_types_e type;
} remote_can_rxconf_st;


// --- CAN forwarding ---------------------------------------------------------

/// @brief: Bit set in the id field of a CAN message to mark it as extended.
/// The same bit CANopen uses for it (CANOPEN_PDO_EXT), spelled out here so the
/// framing needs no CANopen headers — a 29 bit id never reaches it.
#define REMOTE_CAN_ID_EXT_FLAG			(1u << 29)


/// @brief: What a forwarded CAN frame is worth, which decides how hard the
/// device tries to get it across a link that cannot carry everything.
///
/// The classes are not a preference order alone: each one is published on its
/// own link slot, so a class that saturates can only ever stall itself. A
/// firmware download flooding the SDO class does not delay a heartbeat.
typedef enum {
	/// NMT, EMCY and heartbeat. Rare, urgent, and what tells the far end the
	/// machine is still there at all. Sent whole and in order, first.
	REMOTE_CAN_CLASS_CTRL = 0,
	/// SDO in both directions. Must stay in order and must not be coalesced —
	/// a transfer is a conversation, not a value — but it is the one class that
	/// can genuinely flood the link, so it drops before the classes above it.
	REMOTE_CAN_CLASS_SDO,
	/// Process data, SYNC, and every extended (J1939) frame. Periodic
	/// broadcasts whose newest value is the only one worth having, so these are
	/// staged latest-value per id rather than queued.
	REMOTE_CAN_CLASS_BULK,
	REMOTE_CAN_CLASS_COUNT
} remote_can_class_e;


/// @brief: Classifies a CAN frame by its COB-id, the way CANopen assigns
/// function codes. Both ends classify identically, so the sink can label what
/// it receives without being told.
///
/// Every extended frame is bulk whatever its id: extended ids are J1939 or
/// application traffic, which is periodic broadcast by nature and carries none
/// of the CANopen function codes.
static inline remote_can_class_e remote_can_class_of(uint32_t id,
		uv_can_msg_types_e type) {
	remote_can_class_e ret;
	if (type != CAN_STD) {
		ret = REMOTE_CAN_CLASS_BULK;
	}
	else {
		uint32_t fc = id & 0x780u;
		if ((id == 0x000u) ||
				// 0x080 on its own is SYNC, which is periodic; only 0x081-0x0FF
				// is an emergency from a node
				((fc == 0x080u) && ((id & 0x7Fu) != 0u)) ||
				(fc == 0x700u)) {
			ret = REMOTE_CAN_CLASS_CTRL;
		}
		else if ((fc == 0x580u) || (fc == 0x600u)) {
			ret = REMOTE_CAN_CLASS_SDO;
		}
		else {
			ret = REMOTE_CAN_CLASS_BULK;
		}
	}
	return ret;
}


static inline const char *remote_can_class_to_str(remote_can_class_e cls) {
	const char *ret = "?";
	switch (cls) {
	case REMOTE_CAN_CLASS_CTRL:
		ret = "ctrl";
		break;
	case REMOTE_CAN_CLASS_SDO:
		ret = "sdo";
		break;
	case REMOTE_CAN_CLASS_BULK:
		ret = "bulk";
		break;
	default:
		break;
	}
	return ret;
}


/// @brief: The whole bus is being accepted, i.e. a mask-zero rxconf is in
/// effect and the acceptance filter has been opened up for it.
#define REMOTE_CAN_STATS_FLAG_RX_ALL	(1u << 0)

/// @brief: What the device has done with the CAN traffic it was asked to
/// forward, per class. Sent verbatim as REMOTE_MSG_TYPE_CAN_STATS.
///
/// *dropped* is the number the device threw away because the link could not
/// keep up — the one thing the sink has no way of noticing on its own, since a
/// dropped frame looks exactly like a frame that was never sent.
typedef struct __attribute__((packed)) {
	uint32_t forwarded[REMOTE_CAN_CLASS_COUNT];
	uint32_t dropped[REMOTE_CAN_CLASS_COUNT];
	uint16_t queued[REMOTE_CAN_CLASS_COUNT];
	/// frames received from the sink and put on the device's own bus
	uint32_t injected;
	uint8_t rxconf_count;
	uint8_t flags;
} remote_can_stats_st;


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
#define REMOTE_MSG_TYPE_UI_ASSET_REQ_LEN		7
#define REMOTE_MSG_TYPE_UI_ASSET_LEN			REMOTE_UI_MSG_MAX_LEN
#define REMOTE_MSG_TYPE_IOT_CTRL_LEN			3
#define REMOTE_MSG_TYPE_IOT_STATUS_LEN			4
#define REMOTE_MSG_TYPE_UI_INFO_LEN				6
#define REMOTE_MSG_TYPE_CLOSE_LEN				2
#define REMOTE_MSG_TYPE_CAN_STATS_LEN			(2 + sizeof(remote_can_stats_st))
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
	case REMOTE_MSG_TYPE_CLOSE:
		ret = "CLOSE";
		break;
	case REMOTE_MSG_TYPE_CAN_STATS:
		ret = "CAN_STATS";
		break;
	default:
		break;
	}
	return ret;
}


#endif /* UV_HAL_INC_UV_REMOTE_PROTO_H_ */
