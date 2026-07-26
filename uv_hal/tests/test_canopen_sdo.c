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

#include "uv_test.h"
#include "canopen_test_env.h"

#include <string.h>

#include "uv_canopen.h"
#include "canopen/canopen_sdo.h"
/* for the non-volatile node id the SDO server writes through the object dict */
#include "main.h"

/// @file: Tests for the CANopen SDO server.
///
/// SDO is how every parameter on every Usevolt device is read and written: by
/// the uvcan command line tool, by the uv0d display over the bus, and by the
/// bootloader during a firmware update. It is a wire protocol, so the tests are
/// written against the wire: each one hands the stack the exact bytes a real
/// master would send and asserts on the exact bytes that come back.
///
/// That is deliberate. Asserting on the frames rather than on internal state
/// means these tests still hold if the implementation is restructured, and it
/// means a change that would break interoperability with an existing master -
/// the thing that actually costs a service visit - fails here first.


/* SDO command byte layout, CiA 301:
 *
 *   bits 7..5  command specifier
 *   bit  4     toggle (segmented transfers)
 *   bits 3..2  number of unused data bytes, expedited transfers ("n")
 *   bit  1     expedited flag "e"
 *   bit  0     size indicated flag "s"
 */
#define SDO_CMD_EXPEDITED			(1 << 1)
#define SDO_CMD_SIZE_INDICATED		(1 << 0)
#define SDO_CMD_TOGGLE				(1 << 4)

#define STEP_MS						20


/// @brief: Builds an SDO request frame addressed to the device under test.
///
/// @param cmd: the command byte
/// @param mindex: object dictionary main index
/// @param sindex: object dictionary sub index
/// @param data: the four data bytes, as a little endian word
static uv_can_message_st sdo_request(uint8_t cmd, uint16_t mindex,
		uint8_t sindex, uint32_t data) {
	uv_can_message_st msg;

	memset(&msg, 0, sizeof(msg));
	msg.type = CAN_STD;
	msg.id = CANOPEN_SDO_REQUEST_ID + CANOPEN_TEST_NODEID;
	msg.data_length = 8;
	msg.data_8bit[0] = cmd;
	msg.data_8bit[1] = mindex & 0xFF;
	msg.data_8bit[2] = mindex >> 8;
	msg.data_8bit[3] = sindex;
	msg.data_32bit[1] = data;

	return msg;
}


/// @brief: Sends an expedited write (initiate domain download) request
///
/// @param len: the number of significant data bytes, 1 ... 4
static void sdo_write_expedited(uint16_t mindex, uint8_t sindex,
		uint32_t data, uint8_t len) {
	uv_can_message_st msg = sdo_request(
			INITIATE_DOMAIN_DOWNLOAD | SDO_CMD_EXPEDITED | SDO_CMD_SIZE_INDICATED |
			((4 - len) << 2),
			mindex, sindex, data);
	_uv_canopen_sdo_rx(&msg);
}


/// @brief: Sends a read (initiate domain upload) request
static void sdo_read(uint16_t mindex, uint8_t sindex) {
	uv_can_message_st msg = sdo_request(INITIATE_DOMAIN_UPLOAD, mindex, sindex, 0);
	_uv_canopen_sdo_rx(&msg);
}


/// @brief: Sends an upload segment request with the given toggle bit
static void sdo_upload_segment(bool toggle) {
	uv_can_message_st msg = sdo_request(
			UPLOAD_DOMAIN_SEGMENT | (toggle ? SDO_CMD_TOGGLE : 0), 0, 0, 0);
	_uv_canopen_sdo_rx(&msg);
}


/// @brief: Sends a download segment carrying *data_count* payload bytes
static void sdo_download_segment(bool toggle, const void *data,
		uint8_t data_count, bool last) {
	uv_can_message_st msg;

	memset(&msg, 0, sizeof(msg));
	msg.type = CAN_STD;
	msg.id = CANOPEN_SDO_REQUEST_ID + CANOPEN_TEST_NODEID;
	msg.data_length = 8;
	msg.data_8bit[0] = DOWNLOAD_DOMAIN_SEGMENT |
			(toggle ? SDO_CMD_TOGGLE : 0) |
			(((7 - data_count) & 0b111) << 1) |
			(last ? 1 : 0);
	memcpy(&msg.data_8bit[1], data, data_count);

	_uv_canopen_sdo_rx(&msg);
}


/// @brief: Returns the command specifier of a reply frame
static uint8_t reply_cmd(const uv_can_message_st *msg) {
	return msg->data_8bit[0] & 0b11100000;
}


/// @brief: Returns the main index a reply frame echoes back
static uint16_t reply_mindex(const uv_can_message_st *msg) {
	return msg->data_8bit[1] + (msg->data_8bit[2] * 256);
}


/// @brief: Asserts that the last frame sent is an abort carrying *code*, for the
/// object the request named.
#define ASSERT_ABORTED_WITH(mindex_, sindex_, code_) \
	do { \
		const uv_can_message_st *m_ = canopen_test_tx_last(); \
		TEST_ASSERT_NOT_NULL(m_); \
		TEST_ASSERT_EQ(m_->id, CANOPEN_SDO_RESPONSE_ID + CANOPEN_TEST_NODEID); \
		TEST_ASSERT_EQ(m_->data_8bit[0], ABORT_DOMAIN_TRANSFER); \
		TEST_ASSERT_EQ(reply_mindex(m_), (mindex_)); \
		TEST_ASSERT_EQ(m_->data_8bit[3], (sindex_)); \
		TEST_ASSERT_EQ(m_->data_32bit[1], (uint32_t) (code_)); \
	} while (0)


/* ---------------------------------------------------------------------------
 * expedited upload (read)
 * ------------------------------------------------------------------------ */

TEST(sdo_read, replies_to_a_read_of_an_8_bit_object) {
	canopen_test_env_reset();
	canopen_test_data.u8 = 0xAB;

	sdo_read(TEST_OBJ_U8, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(canopen_test_tx_count(), 1);
	TEST_ASSERT_EQ(reply->id, CANOPEN_SDO_RESPONSE_ID + CANOPEN_TEST_NODEID);
	TEST_ASSERT_EQ(reply->data_length, 8);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
	/* expedited, size indicated, three of the four data bytes unused */
	TEST_ASSERT_TRUE((reply->data_8bit[0] & SDO_CMD_EXPEDITED) != 0);
	TEST_ASSERT_TRUE((reply->data_8bit[0] & SDO_CMD_SIZE_INDICATED) != 0);
	TEST_ASSERT_EQ((reply->data_8bit[0] >> 2) & 0b11, 3);
	/* the request's object is echoed back so the master can match the reply */
	TEST_ASSERT_EQ(reply_mindex(reply), TEST_OBJ_U8);
	TEST_ASSERT_EQ(reply->data_8bit[3], 0);
	TEST_ASSERT_EQ(reply->data_8bit[4], 0xAB);
}


TEST(sdo_read, replies_to_a_read_of_a_16_bit_object) {
	canopen_test_env_reset();
	canopen_test_data.u16 = 0x1234;

	sdo_read(TEST_OBJ_U16, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ((reply->data_8bit[0] >> 2) & 0b11, 2);
	TEST_ASSERT_EQ(reply->data_8bit[4], 0x34);
	TEST_ASSERT_EQ(reply->data_8bit[5], 0x12);
}


TEST(sdo_read, replies_to_a_read_of_a_32_bit_object) {
	canopen_test_env_reset();
	canopen_test_data.u32 = 0xDEADBEEF;

	sdo_read(TEST_OBJ_U32, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	/* all four data bytes are significant */
	TEST_ASSERT_EQ((reply->data_8bit[0] >> 2) & 0b11, 0);
	TEST_ASSERT_EQ(reply->data_32bit[1], 0xDEADBEEF);
}


TEST(sdo_read, a_read_only_object_can_be_read) {
	canopen_test_env_reset();
	canopen_test_data.ro = 0x11223344;

	sdo_read(TEST_OBJ_RO, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
	TEST_ASSERT_EQ(reply->data_32bit[1], 0x11223344);
}


TEST(sdo_read, reading_a_write_only_object_is_aborted) {
	canopen_test_env_reset();

	sdo_read(TEST_OBJ_WO, 0);

	ASSERT_ABORTED_WITH(TEST_OBJ_WO, 0,
			CANOPEN_SDO_ERROR_ATTEMPT_TO_READ_A_WRITE_ONLY_OBJECT);
}


TEST(sdo_read, reading_a_missing_object_is_aborted) {
	canopen_test_env_reset();

	sdo_read(TEST_OBJ_MISSING, 0);

	ASSERT_ABORTED_WITH(TEST_OBJ_MISSING, 0,
			CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
}


TEST(sdo_read, reading_array_subindex_zero_returns_the_element_count) {
	canopen_test_env_reset();

	/* an array's sub index 0 carries its length, so a master can discover how
	 * many elements to walk */
	sdo_read(TEST_OBJ_ARRAY32, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
	TEST_ASSERT_EQ(reply->data_32bit[1], TEST_ARRAY_LEN);
}


TEST(sdo_read, fires_the_read_callback) {
	uint16_t mindex = 0;
	uint8_t sindex = 0;
	canopen_test_env_reset();

	TEST_ASSERT_EQ(canopen_test_read_callb_count(NULL, NULL), 0);

	sdo_read(TEST_OBJ_U32, 0);

	TEST_ASSERT_EQ(canopen_test_read_callb_count(&mindex, &sindex), 1);
	TEST_ASSERT_EQ(mindex, TEST_OBJ_U32);
	TEST_ASSERT_EQ(sindex, 0);
}


/* ---------------------------------------------------------------------------
 * expedited download (write)
 * ------------------------------------------------------------------------ */

TEST(sdo_write, writes_an_8_bit_object) {
	canopen_test_env_reset();

	sdo_write_expedited(TEST_OBJ_U8, 0, 0x5A, 1);

	TEST_ASSERT_EQ(canopen_test_data.u8, 0x5A);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_DOWNLOAD_REPLY);
	TEST_ASSERT_EQ(reply_mindex(reply), TEST_OBJ_U8);
	TEST_ASSERT_EQ(reply->data_8bit[3], 0);
}


TEST(sdo_write, writes_a_16_bit_object) {
	canopen_test_env_reset();

	sdo_write_expedited(TEST_OBJ_U16, 0, 0xBEEF, 2);

	TEST_ASSERT_EQ(canopen_test_data.u16, 0xBEEF);
}


TEST(sdo_write, writes_a_32_bit_object) {
	canopen_test_env_reset();

	sdo_write_expedited(TEST_OBJ_U32, 0, 0x12345678, 4);

	TEST_ASSERT_EQ(canopen_test_data.u32, 0x12345678);
}


TEST(sdo_write, does_not_write_past_the_object_it_addresses) {
	canopen_test_env_reset();
	canopen_test_data.u16 = 0;
	canopen_test_data.u32 = 0xFFFFFFFF;

	/* a master that sends four data bytes for a one byte object must not have
	 * the surplus land on whatever sits next to it */
	sdo_write_expedited(TEST_OBJ_U8, 0, 0xFFFFFFFF, 4);

	TEST_ASSERT_EQ(canopen_test_data.u8, 0xFF);
	TEST_ASSERT_EQ(canopen_test_data.u16, 0);
	TEST_ASSERT_EQ(canopen_test_data.u32, 0xFFFFFFFF);
}


TEST(sdo_write, a_write_only_object_can_be_written) {
	canopen_test_env_reset();

	sdo_write_expedited(TEST_OBJ_WO, 0, 0xCAFE, 4);

	TEST_ASSERT_EQ(canopen_test_data.wo, 0xCAFE);
}


TEST(sdo_write, writing_a_read_only_object_is_aborted) {
	canopen_test_env_reset();
	canopen_test_data.ro = 0x1111;

	sdo_write_expedited(TEST_OBJ_RO, 0, 0x2222, 4);

	ASSERT_ABORTED_WITH(TEST_OBJ_RO, 0,
			CANOPEN_SDO_ERROR_ATTEMPT_TO_WRITE_A_READ_ONLY_OBJECT);
	/* and the object is left alone */
	TEST_ASSERT_EQ(canopen_test_data.ro, 0x1111);
}


TEST(sdo_write, writing_a_missing_object_is_aborted) {
	canopen_test_env_reset();

	sdo_write_expedited(TEST_OBJ_MISSING, 0, 1, 4);

	ASSERT_ABORTED_WITH(TEST_OBJ_MISSING, 0,
			CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);
}


TEST(sdo_write, writes_an_array_element) {
	canopen_test_env_reset();

	/* array sub indices are 1 based: sub 1 is element 0 */
	sdo_write_expedited(TEST_OBJ_ARRAY32, 2, 0xAABBCCDD, 4);

	TEST_ASSERT_EQ(canopen_test_data.array32[1], 0xAABBCCDD);
	TEST_ASSERT_EQ(canopen_test_data.array32[0], 0);
	TEST_ASSERT_EQ(canopen_test_data.array32[2], 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_DOWNLOAD_REPLY);
}


TEST(sdo_write, reads_back_an_array_element_that_was_written) {
	canopen_test_env_reset();

	sdo_write_expedited(TEST_OBJ_ARRAY32, 3, 0x0F0F0F0F, 4);
	canopen_test_tx_clear();

	/* Reading an array element is a *segmented* transfer: the server treats any
	 * sub index other than 0 as the start of a stream running from that element
	 * to the end of the array, so the initiate reply carries the remaining byte
	 * count rather than the value. Element 3 of 4 leaves two 32 bit elements. */
	sdo_read(TEST_OBJ_ARRAY32, 3);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
	TEST_ASSERT_EQ(reply->data_32bit[1], 2 * sizeof(uint32_t));

	/* the first segment then carries the element itself */
	canopen_test_tx_clear();
	sdo_upload_segment(false);

	reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), UPLOAD_DOMAIN_SEGMENT_REPLY);

	uint32_t value;
	memcpy(&value, &reply->data_8bit[1], sizeof(value));
	TEST_ASSERT_EQ(value, 0x0F0F0F0F);
}


TEST(sdo_write, writing_array_subindex_zero_is_aborted) {
	canopen_test_env_reset();

	/* sub index 0 reports the array length and is not a storage slot */
	sdo_write_expedited(TEST_OBJ_ARRAY32, 0, 99, 4);

	ASSERT_ABORTED_WITH(TEST_OBJ_ARRAY32, 0,
			CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
}


TEST(sdo_write, writing_past_the_end_of_an_array_is_aborted) {
	canopen_test_env_reset();

	/* the array has TEST_ARRAY_LEN elements, so sub index TEST_ARRAY_LEN + 1
	 * is one past the last. Accepting it would write outside the object.
	 * The dictionary lookup rejects the sub index before the write is even
	 * attempted, so this reports "does not exist" rather than "unsupported
	 * access" - a more precise answer for the master. */
	sdo_write_expedited(TEST_OBJ_ARRAY32, TEST_ARRAY_LEN + 1, 0xFFFFFFFF, 4);

	ASSERT_ABORTED_WITH(TEST_OBJ_ARRAY32, TEST_ARRAY_LEN + 1,
			CANOPEN_SDO_ERROR_OBJECT_DOES_NOT_EXIST);

	/* nothing was written anywhere in the array */
	for (uint32_t i = 0; i < TEST_ARRAY_LEN; i++) {
		TEST_ASSERT_EQ(canopen_test_data.array32[i], 0);
	}
}


TEST(sdo_write, fires_the_write_callback) {
	uint16_t mindex = 0;
	uint8_t sindex = 0;
	canopen_test_env_reset();

	TEST_ASSERT_EQ(canopen_test_write_callb_count(NULL, NULL), 0);

	sdo_write_expedited(TEST_OBJ_U32, 0, 1, 4);

	TEST_ASSERT_EQ(canopen_test_write_callb_count(&mindex, &sindex), 1);
	TEST_ASSERT_EQ(mindex, TEST_OBJ_U32);
	TEST_ASSERT_EQ(sindex, 0);
}


TEST(sdo_write, an_aborted_write_does_not_fire_the_write_callback) {
	canopen_test_env_reset();

	sdo_write_expedited(TEST_OBJ_RO, 0, 1, 4);

	TEST_ASSERT_EQ(canopen_test_write_callb_count(NULL, NULL), 0);
}


/* ---------------------------------------------------------------------------
 * node id range checking
 *
 * The node id is written like any other object, but a value outside 1 ... 0x7F
 * cannot be addressed on the bus. It is stored as written, so an invalid one
 * would only surface after the device is reset - by which point the device is
 * unreachable.
 * ------------------------------------------------------------------------ */

TEST(sdo_nodeid, a_valid_node_id_is_accepted) {
	canopen_test_env_reset();

	sdo_write_expedited(CONFIG_CANOPEN_NODEID_INDEX, 0, 0x20, 1);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_DOWNLOAD_REPLY);
	TEST_ASSERT_EQ(dev.data_start.id, 0x20);
}


TEST(sdo_nodeid, node_id_zero_is_rejected) {
	canopen_test_env_reset();

	sdo_write_expedited(CONFIG_CANOPEN_NODEID_INDEX, 0, 0, 1);

	ASSERT_ABORTED_WITH(CONFIG_CANOPEN_NODEID_INDEX, 0,
			CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_LOW);
	TEST_ASSERT_EQ(dev.data_start.id, CANOPEN_TEST_NODEID);
}


TEST(sdo_nodeid, a_node_id_above_the_addressable_range_is_rejected) {
	canopen_test_env_reset();

	sdo_write_expedited(CONFIG_CANOPEN_NODEID_INDEX, 0, 0x80, 1);

	ASSERT_ABORTED_WITH(CONFIG_CANOPEN_NODEID_INDEX, 0,
			CANOPEN_SDO_ERROR_VALUE_OF_PARAMETER_TOO_HIGH);
	TEST_ASSERT_EQ(dev.data_start.id, CANOPEN_TEST_NODEID);
}


TEST(sdo_nodeid, the_highest_valid_node_id_is_accepted) {
	canopen_test_env_reset();

	sdo_write_expedited(CONFIG_CANOPEN_NODEID_INDEX, 0, 0x7F, 1);

	TEST_ASSERT_EQ(dev.data_start.id, 0x7F);
}


/* ---------------------------------------------------------------------------
 * segmented upload (reading a string)
 * ------------------------------------------------------------------------ */

TEST(sdo_segmented_read, initiating_a_string_read_reports_the_total_length) {
	canopen_test_env_reset();
	strcpy(canopen_test_data.string, "hello");

	sdo_read(TEST_OBJ_STRING, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
	/* not expedited, but the size is indicated */
	TEST_ASSERT_TRUE((reply->data_8bit[0] & SDO_CMD_EXPEDITED) == 0);
	TEST_ASSERT_TRUE((reply->data_8bit[0] & SDO_CMD_SIZE_INDICATED) != 0);
	TEST_ASSERT_EQ(reply->data_32bit[1], TEST_STRING_LEN);
}


TEST(sdo_segmented_read, transfers_a_string_in_seven_byte_segments) {
	canopen_test_env_reset();
	for (uint32_t i = 0; i < TEST_STRING_LEN; i++) {
		canopen_test_data.string[i] = (char) ('A' + (i % 26));
	}

	sdo_read(TEST_OBJ_STRING, 0);
	canopen_test_tx_clear();

	uint8_t received[TEST_STRING_LEN];
	uint32_t received_len = 0;
	bool toggle = false;
	bool done = false;

	for (uint32_t seg = 0; (seg < 32) && !done; seg++) {
		sdo_upload_segment(toggle);

		const uv_can_message_st *reply = canopen_test_tx_last();
		TEST_ASSERT_NOT_NULL(reply);
		TEST_ASSERT_EQ(reply_cmd(reply), UPLOAD_DOMAIN_SEGMENT_REPLY);
		/* the server must echo the toggle bit it was sent */
		TEST_ASSERT_EQ((reply->data_8bit[0] & SDO_CMD_TOGGLE) != 0, toggle);

		uint8_t unused = (reply->data_8bit[0] & 0b1110) >> 1;
		uint8_t count = 7 - unused;
		TEST_ASSERT_TRUE(received_len + count <= TEST_STRING_LEN);
		memcpy(&received[received_len], &reply->data_8bit[1], count);
		received_len += count;

		done = ((reply->data_8bit[0] & SDO_CMD_SIZE_INDICATED) != 0);
		toggle = !toggle;
	}

	TEST_ASSERT_TRUE(done);
	TEST_ASSERT_EQ(received_len, TEST_STRING_LEN);
	TEST_ASSERT_EQ(memcmp(received, canopen_test_data.string, TEST_STRING_LEN), 0);
}


/* The final segment of an upload announces how many of its seven data bytes are
 * *not* used, in bits 3..1 of the command byte ("n" in CiA 301). The server used
 * to OR that count in unshifted, landing it in bits 2..0 where it overlapped the
 * "no more segments" flag in bit 0, so every final segment that did not happen
 * to carry exactly seven bytes announced the wrong length.
 *
 * canopen_sdo_client.c reads and writes n at bits 3..1, and the segmented
 * *download* path in the server decodes it from there too - the encoder was the
 * odd one out. Our own client never noticed because it stops at the total size
 * the initiate reply gave it, but a master that trusts n reads trailing
 * garbage. */
TEST(sdo_segmented_read, the_final_segment_reports_its_length_in_bits_3_to_1) {
	canopen_test_env_reset();

	/* every possible remainder, so no single value can pass by luck */
	for (uint32_t used = 1; used <= 7; used++) {
		uint32_t total = 7 + used;
		canopen_test_env_reset();
		memset(canopen_test_data.string, 'z', sizeof(canopen_test_data.string));

		/* start the read partway in so that exactly *total* bytes remain */
		sdo_read(TEST_OBJ_STRING, TEST_STRING_LEN - total);
		canopen_test_tx_clear();

		/* first segment takes seven bytes and is not the last */
		sdo_upload_segment(false);
		const uv_can_message_st *reply = canopen_test_tx_last();
		TEST_ASSERT_NOT_NULL(reply);
		TEST_ASSERT_TRUE((reply->data_8bit[0] & SDO_CMD_SIZE_INDICATED) == 0);

		/* second segment is the last and carries the remainder */
		canopen_test_tx_clear();
		sdo_upload_segment(true);
		reply = canopen_test_tx_last();
		TEST_ASSERT_NOT_NULL(reply);
		TEST_ASSERT_TRUE((reply->data_8bit[0] & SDO_CMD_SIZE_INDICATED) != 0);

		uint8_t n = (reply->data_8bit[0] & 0b1110) >> 1;
		TEST_ASSERT_EQ(7 - n, used);
	}
}


TEST(sdo_segmented_read, an_unaltered_toggle_bit_is_aborted) {
	canopen_test_env_reset();
	strcpy(canopen_test_data.string, "abcdefghijklmnop");

	sdo_read(TEST_OBJ_STRING, 0);
	sdo_upload_segment(false);
	canopen_test_tx_clear();

	/* the toggle bit must alternate; repeating it means the master lost a
	 * segment, and continuing would silently corrupt the transfer */
	sdo_upload_segment(false);

	ASSERT_ABORTED_WITH(TEST_OBJ_STRING, 0,
			CANOPEN_SDO_ERROR_SDO_TOGGLE_BIT_NOT_ALTERED);
}


TEST(sdo_segmented_read, an_abort_from_the_master_returns_the_server_to_ready) {
	canopen_test_env_reset();
	strcpy(canopen_test_data.string, "abcdefghijklmnop");
	canopen_test_data.u32 = 0x99887766;

	sdo_read(TEST_OBJ_STRING, 0);
	sdo_upload_segment(false);

	/* the master gives up mid transfer */
	uv_can_message_st abort = sdo_request(ABORT_DOMAIN_TRANSFER,
			TEST_OBJ_STRING, 0, CANOPEN_SDO_ERROR_GENERAL);
	_uv_canopen_sdo_rx(&abort);
	canopen_test_tx_clear();

	/* an unrelated request must now be served normally rather than being
	 * treated as the next segment of the abandoned transfer */
	sdo_read(TEST_OBJ_U32, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
	TEST_ASSERT_EQ(reply_mindex(reply), TEST_OBJ_U32);
	TEST_ASSERT_EQ(reply->data_32bit[1], 0x99887766);
}


TEST(sdo_segmented_read, a_stalled_transfer_times_out) {
	canopen_test_env_reset();
	strcpy(canopen_test_data.string, "abcdefghijklmnop");

	sdo_read(TEST_OBJ_STRING, 0);
	canopen_test_tx_clear();

	/* the master goes away mid transfer. The server must not stay busy
	 * forever - it would refuse every later request from anyone. */
	for (uint32_t t = 0; t < CONFIG_CANOPEN_SDO_TIMEOUT_MS * 2; t += STEP_MS) {
		_uv_canopen_sdo_step(STEP_MS);
	}

	ASSERT_ABORTED_WITH(TEST_OBJ_STRING, 0,
			CANOPEN_SDO_ERROR_SDO_PROTOCOL_TIMED_OUT);

	/* and it is serving requests again */
	canopen_test_tx_clear();
	sdo_read(TEST_OBJ_U8, 0);
	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
}


TEST(sdo_segmented_read, does_not_time_out_while_the_master_keeps_talking) {
	canopen_test_env_reset();
	for (uint32_t i = 0; i < TEST_STRING_LEN; i++) {
		canopen_test_data.string[i] = 'x';
	}

	sdo_read(TEST_OBJ_STRING, 0);

	/* each segment restarts the timeout, so a slow but live master must be
	 * allowed to finish */
	bool toggle = false;
	for (uint32_t seg = 0; seg < 3; seg++) {
		for (uint32_t t = 0; t < CONFIG_CANOPEN_SDO_TIMEOUT_MS / 2; t += STEP_MS) {
			_uv_canopen_sdo_step(STEP_MS);
		}
		canopen_test_tx_clear();
		sdo_upload_segment(toggle);
		toggle = !toggle;

		const uv_can_message_st *reply = canopen_test_tx_last();
		TEST_ASSERT_NOT_NULL(reply);
		TEST_ASSERT_EQ(reply_cmd(reply), UPLOAD_DOMAIN_SEGMENT_REPLY);
	}
}


/* ---------------------------------------------------------------------------
 * segmented download (writing a string)
 * ------------------------------------------------------------------------ */

TEST(sdo_segmented_write, transfers_a_string_in_seven_byte_segments) {
	static const char payload[] = "the quick brown fox jumps over!";
	canopen_test_env_reset();

	uv_can_message_st init = sdo_request(INITIATE_DOMAIN_DOWNLOAD,
			TEST_OBJ_STRING, 0, 0);
	_uv_canopen_sdo_rx(&init);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_DOWNLOAD_REPLY);

	uint32_t sent = 0;
	bool toggle = false;
	uint32_t total = sizeof(payload);

	while (sent < total) {
		uint8_t count = (total - sent > 7) ? 7 : (uint8_t) (total - sent);
		bool last = ((sent + count) >= total);

		canopen_test_tx_clear();
		sdo_download_segment(toggle, &payload[sent], count, last);

		reply = canopen_test_tx_last();
		TEST_ASSERT_NOT_NULL(reply);
		TEST_ASSERT_EQ(reply_cmd(reply), DOWNLOAD_DOMAIN_SEGMENT_REPLY);
		TEST_ASSERT_EQ((reply->data_8bit[0] & SDO_CMD_TOGGLE) != 0, toggle);

		sent += count;
		toggle = !toggle;
	}

	TEST_ASSERT_STR_EQ(canopen_test_data.string, payload);
}


TEST(sdo_segmented_write, an_unaltered_toggle_bit_is_aborted) {
	canopen_test_env_reset();

	uv_can_message_st init = sdo_request(INITIATE_DOMAIN_DOWNLOAD,
			TEST_OBJ_STRING, 0, 0);
	_uv_canopen_sdo_rx(&init);
	sdo_download_segment(false, "abcdefg", 7, false);
	canopen_test_tx_clear();

	sdo_download_segment(false, "hijklmn", 7, false);

	ASSERT_ABORTED_WITH(TEST_OBJ_STRING, 0,
			CANOPEN_SDO_ERROR_SDO_TOGGLE_BIT_NOT_ALTERED);
}


TEST(sdo_segmented_write, a_segmented_write_to_a_scalar_object_is_aborted) {
	canopen_test_env_reset();

	/* segmented transfers only make sense for strings and arrays; a scalar
	 * always fits in one expedited frame */
	uv_can_message_st init = sdo_request(INITIATE_DOMAIN_DOWNLOAD,
			TEST_OBJ_U32, 0, 0);
	_uv_canopen_sdo_rx(&init);

	ASSERT_ABORTED_WITH(TEST_OBJ_U32, 0,
			CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
}


TEST(sdo_segmented_write, does_not_write_past_the_end_of_the_string) {
	canopen_test_env_reset();

	uv_can_message_st init = sdo_request(INITIATE_DOMAIN_DOWNLOAD,
			TEST_OBJ_STRING, 0, 0);
	_uv_canopen_sdo_rx(&init);

	/* keep feeding full segments well past the object's length. The server has
	 * to stop accepting them rather than run off the end of the string - there
	 * is no MMU on the target to catch it. */
	bool toggle = false;
	bool aborted = false;
	for (uint32_t seg = 0; (seg < (TEST_STRING_LEN / 7) + 4) && !aborted; seg++) {
		canopen_test_tx_clear();
		sdo_download_segment(toggle, "0123456", 7, false);
		toggle = !toggle;

		const uv_can_message_st *reply = canopen_test_tx_last();
		TEST_ASSERT_NOT_NULL(reply);
		aborted = (reply->data_8bit[0] == ABORT_DOMAIN_TRANSFER);
	}

	TEST_ASSERT_TRUE(aborted);

	/* whatever landed, it stayed inside the object */
	TEST_ASSERT_EQ(canopen_test_data.array32[0], 0);
	TEST_ASSERT_EQ(canopen_test_data.array8[0], 0);
}


TEST(sdo_segmented_write, a_string_written_segmented_reads_back_segmented) {
	static const char payload[] = "round trip through the bus";
	canopen_test_env_reset();

	uv_can_message_st init = sdo_request(INITIATE_DOMAIN_DOWNLOAD,
			TEST_OBJ_STRING, 0, 0);
	_uv_canopen_sdo_rx(&init);

	uint32_t sent = 0;
	bool toggle = false;
	uint32_t total = sizeof(payload);
	while (sent < total) {
		uint8_t count = (total - sent > 7) ? 7 : (uint8_t) (total - sent);
		sdo_download_segment(toggle, &payload[sent], count,
				(sent + count) >= total);
		sent += count;
		toggle = !toggle;
	}

	/* now read the same object back out */
	canopen_test_tx_clear();
	sdo_read(TEST_OBJ_STRING, 0);

	uint8_t received[TEST_STRING_LEN];
	uint32_t received_len = 0;
	bool done = false;
	toggle = false;
	for (uint32_t seg = 0; (seg < 32) && !done; seg++) {
		sdo_upload_segment(toggle);
		const uv_can_message_st *reply = canopen_test_tx_last();
		TEST_ASSERT_NOT_NULL(reply);

		uint8_t count = 7 - ((reply->data_8bit[0] & 0b1110) >> 1);
		memcpy(&received[received_len], &reply->data_8bit[1], count);
		received_len += count;
		done = ((reply->data_8bit[0] & SDO_CMD_SIZE_INDICATED) != 0);
		toggle = !toggle;
	}

	TEST_ASSERT_TRUE(done);
	TEST_ASSERT_STR_EQ((const char*) received, payload);
}


/* ---------------------------------------------------------------------------
 * addressing and framing
 * ------------------------------------------------------------------------ */

TEST(sdo_addressing, a_request_for_another_node_is_ignored) {
	canopen_test_env_reset();

	uv_can_message_st msg = sdo_request(INITIATE_DOMAIN_UPLOAD,
			TEST_OBJ_U32, 0, 0);
	msg.id = CANOPEN_SDO_REQUEST_ID + CANOPEN_TEST_NODEID + 1;
	_uv_canopen_sdo_rx(&msg);

	/* answering another node's request would collide with that node's reply */
	TEST_ASSERT_EQ(canopen_test_tx_count(), 0);
}


TEST(sdo_addressing, a_frame_that_is_not_an_sdo_is_ignored) {
	canopen_test_env_reset();

	uv_can_message_st msg = sdo_request(INITIATE_DOMAIN_UPLOAD,
			TEST_OBJ_U32, 0, 0);
	/* a TXPDO, not an SDO */
	msg.id = 0x180 + CANOPEN_TEST_NODEID;
	_uv_canopen_sdo_rx(&msg);

	TEST_ASSERT_EQ(canopen_test_tx_count(), 0);
}


TEST(sdo_addressing, a_short_frame_is_ignored) {
	canopen_test_env_reset();

	/* every SDO frame is exactly 8 bytes; a shorter one cannot carry an
	 * index and must not be parsed as though it did */
	uv_can_message_st msg = sdo_request(INITIATE_DOMAIN_UPLOAD,
			TEST_OBJ_U32, 0, 0);
	msg.data_length = 4;
	_uv_canopen_sdo_rx(&msg);

	TEST_ASSERT_EQ(canopen_test_tx_count(), 0);
}


TEST(sdo_addressing, an_extended_frame_is_ignored) {
	canopen_test_env_reset();

	uv_can_message_st msg = sdo_request(INITIATE_DOMAIN_UPLOAD,
			TEST_OBJ_U32, 0, 0);
	msg.type = CAN_EXT;
	_uv_canopen_sdo_rx(&msg);

	TEST_ASSERT_EQ(canopen_test_tx_count(), 0);
}


TEST(sdo_addressing, replies_carry_the_response_cob_id_of_this_node) {
	canopen_test_env_reset();

	sdo_read(TEST_OBJ_U32, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	/* a master matches replies by COB-ID, so this is the one thing that must
	 * never drift */
	TEST_ASSERT_EQ(reply->id, CANOPEN_SDO_RESPONSE_ID + CANOPEN_TEST_NODEID);
	TEST_ASSERT_EQ(reply->type, CAN_STD);
	TEST_ASSERT_EQ(reply->data_length, 8);
}


TEST(sdo_addressing, an_unknown_command_specifier_is_aborted) {
	canopen_test_env_reset();

	/* 0xE0 is not a command specifier this stack implements */
	uv_can_message_st msg = sdo_request(0xE0, TEST_OBJ_U32, 0, 0);
	_uv_canopen_sdo_rx(&msg);

	ASSERT_ABORTED_WITH(TEST_OBJ_U32, 0,
			CANOPEN_SDO_ERROR_UNSUPPORTED_ACCESS_TO_OBJECT);
}


TEST(sdo_addressing, a_stopped_node_answers_nothing) {
	canopen_test_env_reset();
	_canopen.state = CANOPEN_STOPPED;

	sdo_read(TEST_OBJ_U32, 0);
	sdo_write_expedited(TEST_OBJ_U32, 0, 1, 4);

	/* CiA 301: a stopped node communicates only NMT and heartbeat */
	TEST_ASSERT_EQ(canopen_test_tx_count(), 0);
	TEST_ASSERT_EQ(canopen_test_data.u32, 0);
}


TEST(sdo_addressing, the_server_follows_a_changed_node_id) {
	canopen_test_env_reset();
	_canopen.current_node_id = 0x33;

	uv_can_message_st msg = sdo_request(INITIATE_DOMAIN_UPLOAD,
			TEST_OBJ_U32, 0, 0);
	msg.id = CANOPEN_SDO_REQUEST_ID + 0x33;
	_uv_canopen_sdo_rx(&msg);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply->id, CANOPEN_SDO_RESPONSE_ID + 0x33);
}


/* ---------------------------------------------------------------------------
 * standard object dictionary entries
 * ------------------------------------------------------------------------ */

TEST(sdo_standard_objects, the_identity_object_reports_its_element_count) {
	canopen_test_env_reset();

	sdo_read(CONFIG_CANOPEN_IDENTITY_INDEX, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
	/* vendor id, product code and revision number - and nothing beyond them.
	 * A too-large count made the server report trailing entries that do not
	 * exist and read past the object to fill them. */
	TEST_ASSERT_EQ(reply->data_32bit[1], CANOPEN_IDENTITY_OBJECT_ARRAY_SIZE);
}


TEST(sdo_standard_objects, the_identity_object_reports_the_vendor_id) {
	canopen_test_env_reset();

	sdo_read(CONFIG_CANOPEN_IDENTITY_INDEX, 1);
	canopen_test_tx_clear();
	sdo_upload_segment(false);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), UPLOAD_DOMAIN_SEGMENT_REPLY);

	uint32_t vendor_id;
	memcpy(&vendor_id, &reply->data_8bit[1], sizeof(vendor_id));
	TEST_ASSERT_EQ(vendor_id, CANOPEN_USEVOLT_VENDOR_ID);
}


TEST(sdo_standard_objects, the_node_id_object_reads_back_what_was_written) {
	canopen_test_env_reset();

	sdo_write_expedited(CONFIG_CANOPEN_NODEID_INDEX, 0, 0x1B, 1);
	canopen_test_tx_clear();
	sdo_read(CONFIG_CANOPEN_NODEID_INDEX, 0);

	const uv_can_message_st *reply = canopen_test_tx_last();
	TEST_ASSERT_NOT_NULL(reply);
	TEST_ASSERT_EQ(reply_cmd(reply), INITIATE_DOMAIN_UPLOAD);
	TEST_ASSERT_EQ(reply->data_8bit[4], 0x1B);
}


/* ---------------------------------------------------------------------------
 * sequencing
 * ------------------------------------------------------------------------ */

TEST(sdo_sequencing, back_to_back_expedited_requests_each_get_one_reply) {
	canopen_test_env_reset();
	canopen_test_data.u8 = 1;
	canopen_test_data.u16 = 2;
	canopen_test_data.u32 = 3;

	sdo_read(TEST_OBJ_U8, 0);
	sdo_read(TEST_OBJ_U16, 0);
	sdo_read(TEST_OBJ_U32, 0);

	TEST_ASSERT_EQ(canopen_test_tx_count(), 3);
	TEST_ASSERT_EQ(canopen_test_tx_at(0)->data_8bit[4], 1);
	TEST_ASSERT_EQ(canopen_test_tx_at(1)->data_8bit[4], 2);
	TEST_ASSERT_EQ(canopen_test_tx_at(2)->data_8bit[4], 3);
}


TEST(sdo_sequencing, an_expedited_request_needs_no_step_call) {
	canopen_test_env_reset();
	canopen_test_data.u32 = 0x2468;

	/* expedited transfers complete inside the rx handler. If they needed a
	 * step to finish, every caller would have to pump the stack. */
	sdo_read(TEST_OBJ_U32, 0);

	TEST_ASSERT_EQ(canopen_test_tx_count(), 1);
	TEST_ASSERT_EQ(canopen_test_tx_last()->data_32bit[1], 0x2468);
}


TEST(sdo_sequencing, stepping_an_idle_server_sends_nothing) {
	canopen_test_env_reset();

	for (uint32_t t = 0; t < CONFIG_CANOPEN_SDO_TIMEOUT_MS * 3; t += STEP_MS) {
		_uv_canopen_sdo_step(STEP_MS);
	}

	/* an idle server must not time out a transfer it never started */
	TEST_ASSERT_EQ(canopen_test_tx_count(), 0);
}
