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
#include "uv_json.h"

#include <string.h>

/// @file: Tests for the uv_json reader and writer.
///
/// uv_json parses into a caller supplied fixed size buffer with no dynamic
/// allocation, which makes buffer handling the interesting part: an overflow
/// here writes past a stack buffer on a device with no MMU. The reader also
/// mutates its input in place (uv_jsonreader_init strips whitespace), so the
/// tests hand it writable buffers rather than string literals.


#define JSON_BUF_LEN		512


/* ---------------------------------------------------------------------------
 * writer
 * ------------------------------------------------------------------------ */

TEST(json_writer, writes_an_empty_object) {
	char buf[JSON_BUF_LEN];
	uv_json_st json;

	TEST_ASSERT_EQ(uv_jsonwriter_init(&json, buf, sizeof(buf)), ERR_NONE);
	TEST_ASSERT_EQ(uv_jsonwriter_end(&json, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "{}");
}


TEST(json_writer, writes_integer_members) {
	char buf[JSON_BUF_LEN];
	uv_json_st json;

	uv_jsonwriter_init(&json, buf, sizeof(buf));
	TEST_ASSERT_EQ(uv_jsonwriter_add_int(&json, "a", 1), ERR_NONE);
	TEST_ASSERT_EQ(uv_jsonwriter_add_int(&json, "b", -2), ERR_NONE);
	TEST_ASSERT_EQ(uv_jsonwriter_end(&json, NULL), ERR_NONE);

	/* the trailing comma of the last member must be trimmed */
	TEST_ASSERT_STR_EQ(buf, "{\"a\":1,\"b\":-2}");
}


TEST(json_writer, writes_string_and_bool_members) {
	char buf[JSON_BUF_LEN];
	uv_json_st json;

	uv_jsonwriter_init(&json, buf, sizeof(buf));
	uv_jsonwriter_add_string(&json, "name", "uv0d");
	uv_jsonwriter_add_bool(&json, "on", true);
	uv_jsonwriter_add_bool(&json, "off", false);
	TEST_ASSERT_EQ(uv_jsonwriter_end(&json, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "{\"name\":\"uv0d\",\"on\":true,\"off\":false}");
}


TEST(json_writer, writes_hex_integers) {
	char buf[JSON_BUF_LEN];
	uv_json_st json;

	uv_jsonwriter_init(&json, buf, sizeof(buf));
	uv_jsonwriter_add_int_hex(&json, "id", 0x1AF);
	TEST_ASSERT_EQ(uv_jsonwriter_end(&json, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "{\"id\":\"0x1af\"}");
}


TEST(json_writer, writes_an_array) {
	char buf[JSON_BUF_LEN];
	uv_json_st json;

	uv_jsonwriter_init(&json, buf, sizeof(buf));
	TEST_ASSERT_EQ(uv_jsonwriter_begin_array(&json, "vals"), ERR_NONE);
	uv_jsonwriter_array_add_int(&json, 1);
	uv_jsonwriter_array_add_int(&json, 2);
	uv_jsonwriter_array_add_int(&json, 3);
	TEST_ASSERT_EQ(uv_jsonwriter_end_array(&json), ERR_NONE);
	TEST_ASSERT_EQ(uv_jsonwriter_end(&json, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "{\"vals\":[1,2,3]}");
}


TEST(json_writer, writes_an_empty_array) {
	char buf[JSON_BUF_LEN];
	uv_json_st json;

	uv_jsonwriter_init(&json, buf, sizeof(buf));
	uv_jsonwriter_begin_array(&json, "vals");
	uv_jsonwriter_end_array(&json);
	TEST_ASSERT_EQ(uv_jsonwriter_end(&json, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "{\"vals\":[]}");
}


TEST(json_writer, writes_a_string_array) {
	char buf[JSON_BUF_LEN];
	uv_json_st json;

	uv_jsonwriter_init(&json, buf, sizeof(buf));
	uv_jsonwriter_begin_array(&json, "names");
	uv_jsonwriter_array_add_string(&json, "a");
	uv_jsonwriter_array_add_string(&json, "b");
	uv_jsonwriter_end_array(&json);
	TEST_ASSERT_EQ(uv_jsonwriter_end(&json, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "{\"names\":[\"a\",\"b\"]}");
}


TEST(json_writer, rejects_writes_that_would_overflow_the_buffer) {
	char buf[24];
	uv_json_st json;
	uv_errors_e err = ERR_NONE;
	uint32_t written = 0;

	uv_jsonwriter_init(&json, buf, sizeof(buf));

	/* keep adding members until the writer refuses. It must refuse rather than
	 * run off the end of the caller's buffer - there is no MMU to catch it. */
	for (uint32_t i = 0; (i < 100) && (err == ERR_NONE); i++) {
		err = uv_jsonwriter_add_int(&json, "key", 12345);
		if (err == ERR_NONE) {
			written++;
		}
	}

	TEST_ASSERT_NE(err, ERR_NONE);
	TEST_ASSERT_TRUE(written < 100);
	TEST_ASSERT_TRUE(strlen(buf) < sizeof(buf));
}


TEST(json_writer, reports_an_unterminated_object) {
	char buf[JSON_BUF_LEN];
	uv_json_st json;
	uv_json_errors_e json_err = JSON_ERR_NONE;

	uv_jsonwriter_init(&json, buf, sizeof(buf));
	uv_jsonwriter_begin_array(&json, "vals");
	uv_jsonwriter_array_add_int(&json, 1);
	/* deliberately no uv_jsonwriter_end_array() */

	/* uv_errors_e carries the reporting module in the top byte, so the error
	 * code itself has to be masked out before comparing */
	uv_errors_e err = uv_jsonwriter_end(&json, &json_err);
	TEST_ASSERT_EQ(err & HAL_MODULE_MASK, ERR_INTERNAL);
	TEST_ASSERT_EQ(json_err, JSON_ERR_UNTERMINATED_OBJ);
}


/* ---------------------------------------------------------------------------
 * reader
 * ------------------------------------------------------------------------ */

/// @brief: Copies a JSON literal into a writable buffer and prepares it for
/// reading. uv_jsonreader_init strips whitespace in place, so the reader cannot
/// be pointed at a string literal.
///
/// @note: The length handed to uv_jsonreader_init is the size of the buffer, as
/// the API documents it - "the maximum length in bytes of the buffer".
static void reader_setup(char *dest, unsigned int dest_len, const char *src) {
	strncpy(dest, src, dest_len - 1);
	dest[dest_len - 1] = '\0';
	uv_jsonreader_init(dest, dest_len);
}


TEST(json_reader, finds_a_child_by_name) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{ \"a\" : 1, \"b\" : 22, \"c\" : 333 }");

	char *b = uv_jsonreader_find_child(buf, "b");
	TEST_ASSERT_NOT_NULL(b);
	TEST_ASSERT_EQ(uv_jsonreader_get_int(b), 22);

	char *c = uv_jsonreader_find_child(buf, "c");
	TEST_ASSERT_NOT_NULL(c);
	TEST_ASSERT_EQ(uv_jsonreader_get_int(c), 333);
}


TEST(json_reader, returns_null_for_a_missing_child) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{\"a\":1}");

	TEST_ASSERT_NULL(uv_jsonreader_find_child(buf, "nope"));
}


TEST(json_reader, reads_negative_integers) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{\"a\":-42}");

	char *a = uv_jsonreader_find_child(buf, "a");
	TEST_ASSERT_NOT_NULL(a);
	TEST_ASSERT_EQ(uv_jsonreader_get_int(a), -42);
}


TEST(json_reader, reads_hex_integers) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{\"a\":\"0x1af\"}");

	char *a = uv_jsonreader_find_child(buf, "a");
	TEST_ASSERT_NOT_NULL(a);
	TEST_ASSERT_EQ(uv_jsonreader_get_int(a), 0x1af);
}


TEST(json_reader, reads_strings) {
	char buf[JSON_BUF_LEN];
	char dest[32];
	reader_setup(buf, sizeof(buf), "{\"name\":\"uv0d_jhc\"}");

	char *name = uv_jsonreader_find_child(buf, "name");
	TEST_ASSERT_NOT_NULL(name);
	TEST_ASSERT_TRUE(uv_jsonreader_get_string(name, dest, sizeof(dest)));
	TEST_ASSERT_STR_EQ(dest, "uv0d_jhc");
}


TEST(json_reader, truncates_a_string_into_a_short_destination) {
	char buf[JSON_BUF_LEN];
	char dest[4];
	reader_setup(buf, sizeof(buf), "{\"name\":\"abcdefgh\"}");

	char *name = uv_jsonreader_find_child(buf, "name");
	TEST_ASSERT_NOT_NULL(name);
	(void) uv_jsonreader_get_string(name, dest, sizeof(dest));

	/* whatever the return value, the destination must stay a valid, terminated
	 * string inside its own bounds */
	TEST_ASSERT_TRUE(strlen(dest) < sizeof(dest));
}


TEST(json_reader, reads_bools) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{\"yes\":true,\"no\":false}");

	char *yes = uv_jsonreader_find_child(buf, "yes");
	char *no = uv_jsonreader_find_child(buf, "no");
	TEST_ASSERT_NOT_NULL(yes);
	TEST_ASSERT_NOT_NULL(no);
	TEST_ASSERT_TRUE(uv_jsonreader_get_bool(yes));
	TEST_ASSERT_FALSE(uv_jsonreader_get_bool(no));
}


TEST(json_reader, reports_the_value_type) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf),
			"{\"i\":1,\"s\":\"x\",\"b\":true,\"o\":{\"n\":1},\"a\":[1,2]}");

	TEST_ASSERT_EQ(uv_jsonreader_get_type(
			uv_jsonreader_find_child(buf, "i")), JSON_INT);
	TEST_ASSERT_EQ(uv_jsonreader_get_type(
			uv_jsonreader_find_child(buf, "s")), JSON_STRING);
	TEST_ASSERT_EQ(uv_jsonreader_get_type(
			uv_jsonreader_find_child(buf, "b")), JSON_BOOL);
	TEST_ASSERT_EQ(uv_jsonreader_get_type(
			uv_jsonreader_find_child(buf, "o")), JSON_OBJECT);
	TEST_ASSERT_EQ(uv_jsonreader_get_type(
			uv_jsonreader_find_child(buf, "a")), JSON_ARRAY);
}


TEST(json_reader, reads_the_object_name) {
	char buf[JSON_BUF_LEN];
	char dest[32];
	reader_setup(buf, sizeof(buf), "{\"the_name\":123}");

	char *obj = uv_jsonreader_find_child(buf, "the_name");
	TEST_ASSERT_NOT_NULL(obj);
	TEST_ASSERT_TRUE(uv_jsonreader_get_obj_name(obj, dest, sizeof(dest)));
	TEST_ASSERT_STR_EQ(dest, "the_name");
}


TEST(json_reader, walks_nested_objects) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf),
			"{\"outer\":{\"inner\":{\"leaf\":7}},\"after\":1}");

	char *outer = uv_jsonreader_find_child(buf, "outer");
	TEST_ASSERT_NOT_NULL(outer);
	char *inner = uv_jsonreader_find_child(outer, "inner");
	TEST_ASSERT_NOT_NULL(inner);
	char *leaf = uv_jsonreader_find_child(inner, "leaf");
	TEST_ASSERT_NOT_NULL(leaf);
	TEST_ASSERT_EQ(uv_jsonreader_get_int(leaf), 7);

	/* a nested object must not hide the members that follow it */
	char *after = uv_jsonreader_find_child(buf, "after");
	TEST_ASSERT_NOT_NULL(after);
	TEST_ASSERT_EQ(uv_jsonreader_get_int(after), 1);
}


TEST(json_reader, does_not_find_grandchildren_as_direct_children) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{\"outer\":{\"leaf\":7}}");

	/* "leaf" belongs to "outer", not to the root */
	TEST_ASSERT_NULL(uv_jsonreader_find_child(buf, "leaf"));
}


TEST(json_reader, gets_children_by_index) {
	char buf[JSON_BUF_LEN];
	char name[32];
	reader_setup(buf, sizeof(buf), "{\"a\":1,\"b\":2,\"c\":3}");

	for (uint16_t i = 0; i < 3; i++) {
		char *child = uv_jsonreader_get_child(buf, i);
		TEST_ASSERT_NOT_NULL(child);
		TEST_ASSERT_TRUE(uv_jsonreader_get_obj_name(child, name, sizeof(name)));
		TEST_ASSERT_EQ(name[0], 'a' + i);
		TEST_ASSERT_EQ(uv_jsonreader_get_int(child), i + 1);
	}

	TEST_ASSERT_NULL(uv_jsonreader_get_child(buf, 3));
}


TEST(json_reader, walks_siblings) {
	char buf[JSON_BUF_LEN];
	char name[32];
	reader_setup(buf, sizeof(buf), "{\"a\":1,\"b\":2,\"c\":3}");

	char *child = uv_jsonreader_get_child(buf, 0);
	TEST_ASSERT_NOT_NULL(child);

	char *sibling = NULL;
	TEST_ASSERT_TRUE(uv_jsonreader_get_next_sibling(child, &sibling));
	TEST_ASSERT_TRUE(uv_jsonreader_get_obj_name(sibling, name, sizeof(name)));
	TEST_ASSERT_STR_EQ(name, "b");

	child = sibling;
	TEST_ASSERT_TRUE(uv_jsonreader_get_next_sibling(child, &sibling));
	TEST_ASSERT_TRUE(uv_jsonreader_get_obj_name(sibling, name, sizeof(name)));
	TEST_ASSERT_STR_EQ(name, "c");

	/* the last member has no sibling */
	TEST_ASSERT_FALSE(uv_jsonreader_get_next_sibling(sibling, &sibling));
}


TEST(json_reader, reads_int_arrays) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{\"vals\":[10,20,30]}");

	char *vals = uv_jsonreader_find_child(buf, "vals");
	TEST_ASSERT_NOT_NULL(vals);
	TEST_ASSERT_EQ(uv_jsonreader_get_type(vals), JSON_ARRAY);

	TEST_ASSERT_EQ(uv_jsonreader_array_get_int(vals, 0), 10);
	TEST_ASSERT_EQ(uv_jsonreader_array_get_int(vals, 1), 20);
	TEST_ASSERT_EQ(uv_jsonreader_array_get_int(vals, 2), 30);
	TEST_ASSERT_NULL(uv_jsonreader_array_at(vals, 3));
}


TEST(json_reader, reads_string_arrays) {
	char buf[JSON_BUF_LEN];
	char dest[16];
	reader_setup(buf, sizeof(buf), "{\"names\":[\"aa\",\"bb\"]}");

	char *names = uv_jsonreader_find_child(buf, "names");
	TEST_ASSERT_NOT_NULL(names);

	TEST_ASSERT_TRUE(uv_jsonreader_array_get_string(names, 0, dest, sizeof(dest)));
	TEST_ASSERT_STR_EQ(dest, "aa");
	TEST_ASSERT_TRUE(uv_jsonreader_array_get_string(names, 1, dest, sizeof(dest)));
	TEST_ASSERT_STR_EQ(dest, "bb");
}


TEST(json_reader, reports_array_element_types) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{\"mixed\":[1,\"s\",true]}");

	char *mixed = uv_jsonreader_find_child(buf, "mixed");
	TEST_ASSERT_NOT_NULL(mixed);

	TEST_ASSERT_EQ(uv_jsonreader_array_get_type(mixed, 0), JSON_INT);
	TEST_ASSERT_EQ(uv_jsonreader_array_get_type(mixed, 1), JSON_STRING);
	TEST_ASSERT_EQ(uv_jsonreader_array_get_type(mixed, 2), JSON_BOOL);
}


/* uv_jsonreader_init() takes the size of the caller's buffer and calls
 * json_remove_whitespace(), which compacts the JSON in place and then writes the
 * terminating NUL.
 *
 * It used to write that NUL at (buffer + buffer_len - count), where count is the
 * number of whitespace characters removed. When the JSON contains no strippable
 * whitespace - exactly what uv_jsonwriter produces, and what any machine
 * generated JSON looks like - count is zero and the NUL landed on
 * buffer[buffer_len]: one byte past the end of the caller's buffer, silently
 * clobbering whatever sat next to it in RAM on a target with no MMU.
 *
 * The scan also ran the whole buffer_len instead of stopping at the string
 * terminator, so it walked over uninitialised bytes past the JSON and could pick
 * up stray whitespace there, shifting where the NUL was written. */
TEST(json_reader, init_does_not_write_past_the_end_of_the_buffer) {
	/* the array is larger than the buffer length handed to the reader, so the
	 * stray write lands on a canary rather than on unrelated memory */
	char storage[64];
	const unsigned int buffer_len = 32;

	memset(storage, 0, sizeof(storage));
	strcpy(storage, "{\"a\":1}");
	storage[buffer_len] = 0x5A;

	uv_jsonreader_init(storage, buffer_len);

	TEST_ASSERT_EQ(storage[buffer_len], 0x5A);
}


TEST(json_reader, find_child_on_an_empty_object_returns_null) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{}");

	TEST_ASSERT_NULL(uv_jsonreader_find_child(buf, "anything"));
}


/* uv_jsonreader_get_child() walks the members of an object and returns NULL once
 * the index runs past the last one.
 *
 * For an *empty* object there is no first member to start from: the function
 * steps past the opening '{' and finds itself looking at the closing '}'. That
 * used not to be checked for, so it handed back a pointer to the closing brace
 * as if it were a child, and any caller looping
 * `for (i = 0; (c = get_child(o, i)) != NULL; i++)` saw one bogus member in
 * every empty object it walked. */
TEST(json_reader, get_child_on_an_empty_object_returns_null) {
	char buf[JSON_BUF_LEN];
	reader_setup(buf, sizeof(buf), "{}");

	TEST_ASSERT_NULL(uv_jsonreader_get_child(buf, 0));
}


/* ---------------------------------------------------------------------------
 * round trip
 * ------------------------------------------------------------------------ */

TEST(json, writer_output_can_be_read_back) {
	char buf[JSON_BUF_LEN];
	char str[32];
	uv_json_st json;

	uv_jsonwriter_init(&json, buf, sizeof(buf));
	uv_jsonwriter_add_int(&json, "revision", 2);
	uv_jsonwriter_add_string(&json, "name", "uv0d");
	uv_jsonwriter_add_bool(&json, "metric", true);
	uv_jsonwriter_begin_array(&json, "nodes");
	uv_jsonwriter_array_add_int(&json, 13);
	uv_jsonwriter_array_add_int(&json, 127);
	uv_jsonwriter_end_array(&json);
	TEST_ASSERT_EQ(uv_jsonwriter_end(&json, NULL), ERR_NONE);

	uv_jsonreader_init(buf, sizeof(buf));

	char *revision = uv_jsonreader_find_child(buf, "revision");
	TEST_ASSERT_NOT_NULL(revision);
	TEST_ASSERT_EQ(uv_jsonreader_get_int(revision), 2);

	char *name = uv_jsonreader_find_child(buf, "name");
	TEST_ASSERT_NOT_NULL(name);
	TEST_ASSERT_TRUE(uv_jsonreader_get_string(name, str, sizeof(str)));
	TEST_ASSERT_STR_EQ(str, "uv0d");

	char *metric = uv_jsonreader_find_child(buf, "metric");
	TEST_ASSERT_NOT_NULL(metric);
	TEST_ASSERT_TRUE(uv_jsonreader_get_bool(metric));

	char *nodes = uv_jsonreader_find_child(buf, "nodes");
	TEST_ASSERT_NOT_NULL(nodes);
	TEST_ASSERT_EQ(uv_jsonreader_array_get_int(nodes, 0), 13);
	TEST_ASSERT_EQ(uv_jsonreader_array_get_int(nodes, 1), 127);
}
