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
#include "uv_yaml.h"

#include <string.h>

/// @file: Tests for the uv_yaml writer and reader.
///
/// The two are each other's inverse in uvcan: every file it can write in YAML
/// it also has to be able to read back, so the round trip is what most of these
/// check. Like uv_json, the parser works in place in a caller supplied buffer.


#define YAML_BUF_LEN		1024


/* An empty collection cannot be written as a bare "name:": YAML reads that back
 * as a null value, not as an empty sequence, and a reader which asks for its
 * size or walks its entries then fails instead of simply finding nothing.
 *
 * This is what a saved parameter file looks like for a device which keeps its
 * parameters per operator: its "PARAMS" sequence is empty and everything is
 * under "OPERATORS". Written as a bare "PARAMS:", the whole file was rejected
 * on load with "Couldn't find array type object 'PARAMS'". */
TEST(yaml_writer, writes_an_empty_named_sequence_as_a_flow_sequence) {
	char buf[YAML_BUF_LEN];
	uv_yaml_st yaml;

	uv_yamlwriter_init(&yaml, buf, sizeof(buf));
	TEST_ASSERT_EQ(uv_yamlwriter_begin_seq(&yaml, "PARAMS"), ERR_NONE);
	TEST_ASSERT_EQ(uv_yamlwriter_end_seq(&yaml), ERR_NONE);
	TEST_ASSERT_EQ(uv_yamlwriter_add_int(&yaml, "after", 1), ERR_NONE);
	TEST_ASSERT_EQ(uv_yamlwriter_end(&yaml, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "PARAMS: []\nafter: 1\n");
}


TEST(yaml_writer, writes_an_empty_named_mapping_as_a_flow_mapping) {
	char buf[YAML_BUF_LEN];
	uv_yaml_st yaml;

	uv_yamlwriter_init(&yaml, buf, sizeof(buf));
	TEST_ASSERT_EQ(uv_yamlwriter_begin_map(&yaml, "OPTS"), ERR_NONE);
	TEST_ASSERT_EQ(uv_yamlwriter_end_map(&yaml), ERR_NONE);
	TEST_ASSERT_EQ(uv_yamlwriter_end(&yaml, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "OPTS: {}\n");
}


/* The reader's side of the same thing: an empty sequence has to read back as a
 * sequence of zero entries, not as a scalar. */
TEST(yaml_reader, reads_an_empty_sequence_back_as_an_empty_sequence) {
	char buf[YAML_BUF_LEN];
	uv_yaml_st yaml;

	uv_yamlwriter_init(&yaml, buf, sizeof(buf));
	uv_yamlwriter_begin_seq(&yaml, "PARAMS");
	uv_yamlwriter_end_seq(&yaml);
	uv_yamlwriter_add_int(&yaml, "after", 1);
	uv_yamlwriter_end(&yaml, NULL);

	TEST_ASSERT_EQ(uv_yamlreader_init(buf, sizeof(buf)), ERR_NONE);
	uv_yaml_node_st root = uv_yamlreader_get_root(buf);
	TEST_ASSERT_TRUE(uv_yaml_node_is_valid(root));

	uv_yaml_node_st params = uv_yamlreader_find_child(root, "PARAMS");
	TEST_ASSERT_TRUE(uv_yaml_node_is_valid(params));
	TEST_ASSERT_EQ(uv_yamlreader_get_type(params), YAML_SEQ);
	TEST_ASSERT_EQ(uv_yamlreader_seq_get_size(params), 0);

	/* the member after the empty sequence still belongs to the root */
	uv_yaml_node_st after = uv_yamlreader_find_child(root, "after");
	TEST_ASSERT_TRUE(uv_yaml_node_is_valid(after));
	TEST_ASSERT_EQ(uv_yamlreader_get_int(after), 1);
}


/* A collection with content is unaffected by the empty handling above. */
TEST(yaml_writer, writes_a_named_sequence_with_content_unchanged) {
	char buf[YAML_BUF_LEN];
	uv_yaml_st yaml;

	uv_yamlwriter_init(&yaml, buf, sizeof(buf));
	uv_yamlwriter_begin_seq(&yaml, "vals");
	uv_yamlwriter_seq_add_int(&yaml, 1);
	uv_yamlwriter_seq_add_int(&yaml, 2);
	uv_yamlwriter_end_seq(&yaml);
	TEST_ASSERT_EQ(uv_yamlwriter_end(&yaml, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "vals:\n  - 1\n  - 2\n");
}


/* An empty collection which is itself an entry of a sequence keeps its own
 * line, after the dash which introduces the entry. */
TEST(yaml_writer, writes_an_empty_sequence_entry_as_a_flow_collection) {
	char buf[YAML_BUF_LEN];
	uv_yaml_st yaml;

	uv_yamlwriter_init(&yaml, buf, sizeof(buf));
	uv_yamlwriter_begin_seq(&yaml, "devs");
	uv_yamlwriter_begin_map(&yaml, NULL);
	uv_yamlwriter_end_map(&yaml);
	uv_yamlwriter_end_seq(&yaml);
	TEST_ASSERT_EQ(uv_yamlwriter_end(&yaml, NULL), ERR_NONE);

	TEST_ASSERT_STR_EQ(buf, "devs:\n  - {}\n");
}
