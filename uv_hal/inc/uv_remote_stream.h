/*
 * uv_remote_stream.h
 *
 *  Byte-stream framer and payload packer for the REMOTE protocol.
 */

#ifndef UV_HAL_INC_UV_REMOTE_STREAM_H_
#define UV_HAL_INC_UV_REMOTE_STREAM_H_

/// @file: Turns a stream of bytes back into whole REMOTE messages, and batches
/// whole messages into one transport payload.
///
/// Transport agnostic on purpose: a device feeds the framer from an XB3 byte
/// pipe or from MQTT payloads, and the desktop sink feeds it from MQTT too.
/// Both ends need identical framing — the variable length patching in
/// particular is easy to get subtly wrong — so it lives here rather than being
/// written twice.

#include <stdint.h>
#include <stdbool.h>
#include "uv_remote_proto.h"


/// @brief: Called for each complete message the framer decodes. *data* points
/// at the whole message including the start byte and the type.
typedef void (*remote_frame_callb_t)(void *user, remote_msg_types_e type,
		const uint8_t *data, uint8_t len);


/// @brief: Framer state. One instance per link.
typedef struct {
	remote_msg_types_e receiving_type;
	uint8_t byte_count;
	uint8_t msg_len;
	union {
		// structure to receive CAN data
		struct __attribute__((packed)) {
			uint8_t start_byte;
			uint8_t msg_type_can;
			uint8_t data_len;
			uint32_t id;
			uint8_t data[8];
		} can;
		uint8_t rx_data[REMOTE_MSG_TYPE_MAX_LEN];
	};
} remote_stream_st;


/// @brief: Writes *msg* into *dest* as a whole REMOTE_MSG_TYPE_CAN message.
/// *dest* must hold at least REMOTE_MSG_TYPE_CAN_MAX_LEN bytes; the message is
/// REMOTE_MSG_TYPE_CAN_LEN(msg->data_length) long.
void remote_can_msg_encode(const uv_can_msg_st *msg, uint8_t *dest);


/// @brief: Reads a CAN message back out of one. *data* points at the whole
/// message, start byte and all — i.e. at what the framer hands its callback.
void remote_can_msg_decode(const uint8_t *data, uv_can_msg_st *dest);


/// @brief: Largest transport payload a batch of messages is packed into. On the
/// iot link this is one MQTT message; batching matters there because a publish
/// costs a full AT round trip regardless of size. Override per project.
#ifndef CONFIG_REMOTE_PACK_MAX_LEN
#define CONFIG_REMOTE_PACK_MAX_LEN		256
#endif
#define REMOTE_PACK_MAX_LEN				CONFIG_REMOTE_PACK_MAX_LEN


/// @brief: Accumulates whole messages into one transport payload. A message is
/// never split across payloads, so the framer at the far end always sees a
/// clean byte stream even if a payload is lost.
typedef struct {
	uint16_t len;
	uint8_t buf[REMOTE_PACK_MAX_LEN];
} remote_pack_st;


/// @brief: Puts the framer back into "hunting for a start byte" state,
/// discarding any partially received message.
void remote_stream_reset(remote_stream_st *this);

/// @brief: Feeds *len* received bytes through the framer, invoking *callb* once
/// per complete message with *user* passed back to it. Safe to call with
/// arbitrary chunk boundaries — messages may span calls.
void remote_stream_feed(remote_stream_st *this, const uint8_t *data,
		uint16_t len, remote_frame_callb_t callb, void *user);

/// @brief: Empties the pack buffer.
void remote_pack_clear(remote_pack_st *this);

/// @brief: Appends one whole message to the pack buffer.
/// @return: false if it does not fit — flush the buffer and retry.
bool remote_pack_append(remote_pack_st *this, const uint8_t *frame,
		uint8_t len);


#endif /* UV_HAL_INC_UV_REMOTE_STREAM_H_ */
