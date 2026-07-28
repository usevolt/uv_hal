/*
 * uv_remote_stream.c
 *
 *  Byte-stream framer and payload packer for the REMOTE protocol. See
 *  uv_remote_stream.h.
 */

#include <string.h>
#include "uv_remote_stream.h"


// Lengths of all REMOTE message types, in the same order as
// remote_msg_types_e. Variable-length types (CAN, UI, UI_ASSET) list their
// maximum here; the framer patches in the exact length once it has read the
// length byte.
const uint8_t remote_msg_type_len[REMOTE_MSG_TYPE_COUNT + 1] = {
		REMOTE_MSG_TYPE_CONNECT_LEN,
		REMOTE_MSG_TYPE_CONNECT_RESP_LEN,
		REMOTE_MSG_TYPE_CAN_MAX_LEN,
		REMOTE_MSG_TYPE_RXCONF_LEN,
		REMOTE_MSG_TYPE_RXCLEAR_LEN,
		REMOTE_MSG_TYPE_RXDONE_LEN,
		REMOTE_MSG_TYPE_UI_LEN,
		REMOTE_MSG_TYPE_UI_INPUT_LEN,
		REMOTE_MSG_TYPE_UI_ASSET_REQ_LEN,
		REMOTE_MSG_TYPE_UI_ASSET_LEN,
		REMOTE_MSG_TYPE_IOT_CTRL_LEN,
		REMOTE_MSG_TYPE_IOT_STATUS_LEN,
		REMOTE_MSG_TYPE_UI_INFO_LEN,
		0
};


void remote_stream_reset(remote_stream_st *this) {
	this->receiving_type = REMOTE_MSG_TYPE_COUNT;
	this->byte_count = 0;
	this->msg_len = 0;
	memset(this->rx_data, 0, sizeof(this->rx_data));
}


/// @brief: Consumes one byte. Calls *callb* once a whole message is assembled.
static void stream_feed_byte(remote_stream_st *s, uint8_t c,
		remote_frame_callb_t callb, void *user) {
	// hunting for the start byte and the type byte
	if (s->receiving_type == REMOTE_MSG_TYPE_COUNT) {
		s->rx_data[s->byte_count] = c;
		if (s->byte_count == 0) {
			if (c == REMOTE_MSG_START_BYTE) {
				s->byte_count++;
			}
			else {
				// not a message start, stay in sync hunting
			}
		}
		else {
			if (c < REMOTE_MSG_TYPE_COUNT) {
				s->receiving_type = (remote_msg_types_e) c;
				s->msg_len = remote_msg_type_len[s->receiving_type];
			}
			else {
				// unknown message type, resynchronize
				s->byte_count = 0;
			}
		}
	}
	else {
		// already inside a message: this byte is part of it
	}

	if (s->receiving_type != REMOTE_MSG_TYPE_COUNT) {
		if (s->byte_count < s->msg_len) {
			s->rx_data[s->byte_count] = c;
			s->byte_count++;
		}
		else {
		}

		// Byte index 2 of a CAN message is the data length, and of a UI /
		// UI_ASSET chunk the chunk length. Both are variable length, so patch
		// the expected message length in as soon as that byte arrives.
		if ((s->receiving_type == REMOTE_MSG_TYPE_CAN) &&
				(s->byte_count == 3)) {
			s->msg_len = REMOTE_MSG_TYPE_CAN_LEN(c);
			if (s->msg_len > REMOTE_MSG_TYPE_CAN_LEN(8)) {
				// impossible length, abandon the message
				remote_stream_reset(s);
			}
			else {
			}
		}
		else if (((s->receiving_type == REMOTE_MSG_TYPE_UI) ||
				(s->receiving_type == REMOTE_MSG_TYPE_UI_ASSET)) &&
				(s->byte_count == 3)) {
			s->msg_len = REMOTE_UI_MSG_LEN(c);
			if (s->msg_len > REMOTE_UI_MSG_LEN(REMOTE_UI_CHUNK_MAX_LEN)) {
				// chunk larger than allowed, abandon the message
				remote_stream_reset(s);
			}
			else {
			}
		}
		else if (s->byte_count == s->msg_len) {
			remote_msg_types_e type = s->receiving_type;
			uint8_t len = s->msg_len;
			// reset before the callback so a handler that re-enters the framer
			// (or resets the link) always sees a clean state
			s->receiving_type = REMOTE_MSG_TYPE_COUNT;
			s->byte_count = 0;
			if (callb != NULL) {
				callb(user, type, s->rx_data, len);
			}
			else {
			}
		}
		else {
			// message still incomplete
		}
	}
	else {
	}
}


void remote_stream_feed(remote_stream_st *this, const uint8_t *data,
		uint16_t len, remote_frame_callb_t callb, void *user) {
	for (uint16_t i = 0; i < len; i++) {
		stream_feed_byte(this, data[i], callb, user);
	}
}


void remote_pack_clear(remote_pack_st *this) {
	this->len = 0;
}


bool remote_pack_append(remote_pack_st *this, const uint8_t *frame,
		uint8_t len) {
	bool ret;
	if (((uint16_t) this->len + (uint16_t) len) > REMOTE_PACK_MAX_LEN) {
		// caller must flush first; a message is never split across payloads
		ret = false;
	}
	else {
		memcpy(&this->buf[this->len], frame, len);
		this->len = (uint16_t) (this->len + len);
		ret = true;
	}
	return ret;
}
