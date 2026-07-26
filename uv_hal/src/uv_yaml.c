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

#include "uv_yaml.h"

#if CONFIG_YAML

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>


/// @brief: The indentation of the document root. As the root is not written
/// on any line, it is defined to reside left from the first column.
#define ROOT_INDENT		-1

/// @brief: The maximum length of a scalar which can be converted into a number
#define NUMBUF_LEN		24


const char *uv_yaml_type_to_str(uv_yaml_types_e type) {
	const char *ret = "UNDEFINED";
	switch(type) {
	case YAML_SEQ:
		ret = "SEQUENCE";
		break;
	case YAML_BOOL:
		ret = "BOOL";
		break;
	case YAML_INT:
		ret = "INT";
		break;
	case YAML_MAP:
		ret = "MAPPING";
		break;
	case YAML_STRING:
		ret = "STRING";
		break;
	case YAML_UNSUPPORTED:
		ret = "UNSUPPORTED";
		break;
	default:
		break;
	}
	return ret;
}



/***** COMMON HELPER FUNCTIONS ******/


static uv_yaml_node_st invalid_node(void) {
	uv_yaml_node_st ret = {
			.ptr = NULL,
			.indent = 0,
			.flow = false
	};
	return ret;
}


static uv_yaml_node_st make_node(char *ptr, int16_t indent, bool flow) {
	uv_yaml_node_st ret = {
			.ptr = ptr,
			.indent = indent,
			.flow = flow
	};
	return ret;
}


static bool is_space(char c) {
	return (c == ' ' || c == '\t');
}


/// @brief: Returns true if *c* ends the line
static bool is_eol(char c) {
	return (c == '\n' || c == '\0');
}


static char *skip_space(char *ptr) {
	while (is_space(*ptr)) {
		ptr++;
	}
	return ptr;
}


/// @brief: Returns true if *ptr* points to a sequence entry's '-' character
static bool is_dash(const char *ptr) {
	return (*ptr == '-' && (is_space(ptr[1]) || is_eol(ptr[1])));
}


/// @brief: Returns true if *node* is the document root, i.e. a node which is
/// not written on any line of it's own
static bool is_root(uv_yaml_node_st node) {
	return (node.indent == ROOT_INDENT && !node.flow);
}


/// @brief: Compares *len* characters of *a* and *b* case insensitively.
/// *b* has to be given in lower case.
static bool str_case_eq(const char *a, const char *b, unsigned int len) {
	bool ret = true;

	for (unsigned int i = 0; i < len; i++) {
		if (tolower((int) a[i]) != (int) b[i]) {
			ret = false;
			break;
		}
	}

	return ret;
}


/// @brief: Returns the type which the scalar *str* of *len* characters
/// represents. Quoted scalars are always strings.
static uv_yaml_types_e scalar_type(const char *str, unsigned int len) {
	uv_yaml_types_e ret = YAML_STRING;

	if (*str == '"' || *str == '\'') {
		// a quoted scalar is always a string, an empty one included
		ret = YAML_STRING;
	}
	else if (len == 0) {
		ret = YAML_UNSUPPORTED;
	}
	else {
		// check for an integer, possibly signed and possibly hexadecimal
		unsigned int i = 0;
		if (str[i] == '-' || str[i] == '+') {
			i++;
		}
		bool hex = false;
		if (len > i + 2 && str[i] == '0' && (str[i + 1] == 'x' || str[i + 1] == 'X')) {
			hex = true;
			i += 2;
		}
		unsigned int digits = 0;
		while (i < len && (hex ? isxdigit((int) str[i]) : isdigit((int) str[i]))) {
			i++;
			digits++;
		}
		if (digits != 0 && i == len) {
			ret = YAML_INT;
		}
		else {
			// check for a boolean
			static const char *bools[] = {
					"true", "false", "yes", "no", "on", "off"
			};
			for (i = 0; i < sizeof(bools) / sizeof(bools[0]); i++) {
				if (strlen(bools[i]) == len && str_case_eq(str, bools[i], len)) {
					ret = YAML_BOOL;
					break;
				}
			}
		}
	}

	return ret;
}



/***** READING FUNCTIONS ******/


/// @brief: Removes the carriage returns, the comments and the trailing
/// whitespace from the buffer in place
static void yaml_clean(char *buffer_ptr, unsigned int len) {
	char *src = buffer_ptr;
	char *dest = buffer_ptr;
	// the quotation character which is currently open, or '\0'
	char quote = '\0';

	while (src < buffer_ptr + len) {
		char c = *src;

		if (quote != '\0') {
			// inside a quoted scalar everything is copied as is
			if (c == '\\' && quote == '"' && src + 1 < buffer_ptr + len) {
				*(dest++) = *(src++);
				*(dest++) = *(src++);
				continue;
			}
			else if (c == quote) {
				quote = '\0';
			}
			else if (c == '\n') {
				// a quoted scalar cannot span over lines, don't get stuck to it
				quote = '\0';
			}
			else {

			}
			*(dest++) = *(src++);
		}
		else if (c == '\r') {
			src++;
		}
		else if (c == '#' &&
				(dest == buffer_ptr || is_space(*(dest - 1)) || *(dest - 1) == '\n')) {
			// comment, skip to the end of the line
			while (src < buffer_ptr + len && *src != '\n') {
				src++;
			}
		}
		else {
			// a quoted scalar can start only at the start of a value,
			// elsewhere a quote is just a character of a plain scalar
			if ((c == '"' || c == '\'') &&
					(dest == buffer_ptr || is_space(*(dest - 1)) ||
							*(dest - 1) == '\n' ||
							strchr(":,[{-", *(dest - 1)) != NULL)) {
				quote = c;
			}
			if (c == '\n') {
				// remove the trailing whitespace of the line which just ended
				while (dest > buffer_ptr && is_space(*(dest - 1))) {
					dest--;
				}
			}
			*(dest++) = *(src++);
		}
	}
	// remove the trailing whitespace of the last line
	while (dest > buffer_ptr && is_space(*(dest - 1))) {
		dest--;
	}
	*dest = '\0';
}


uv_errors_e uv_yamlreader_init(char *buffer_ptr, unsigned int buffer_length) {
	uv_errors_e ret = ERR_NONE;

	if (buffer_ptr == NULL) {
		ret = ERR_NULL_PTR;
	}
	else {
		unsigned int len = 0;
		while (len < buffer_length && buffer_ptr[len] != '\0') {
			len++;
		}
		yaml_clean(buffer_ptr, len);
	}

	return ret;
}


uv_yaml_node_st uv_yamlreader_get_root(char *buffer_ptr) {
	return make_node(buffer_ptr, ROOT_INDENT, false);
}


/// @brief: Returns a pointer to the end of the line which *ptr* is on
static char *line_end(char *ptr) {
	while (!is_eol(*ptr)) {
		ptr++;
	}
	return ptr;
}


/// @brief: Returns a pointer to the start of the line following the line
/// which *ptr* is on, or NULL if the document ended
static char *next_line(char *ptr) {
	char *ret = line_end(ptr);
	if (*ret == '\0') {
		ret = NULL;
	}
	else {
		ret++;
	}
	return ret;
}


/// @brief: Parses the content and the indentation of the line starting at *line*
///
/// @return: false if the line should be ignored, i.e. it is empty or it is a
/// document start or end marker
static bool line_info(char *line, char **content, int16_t *indent) {
	char *c = skip_space(line);
	bool ret = true;

	*content = c;
	*indent = (int16_t) (c - line);

	if (is_eol(*c) ||
			(*c == '#') ||
			((strncmp(c, "---", 3) == 0 || strncmp(c, "...", 3) == 0) &&
					(is_space(c[3]) || is_eol(c[3])))) {
		ret = false;
	}

	return ret;
}


/// @brief: Returns a pointer to the ':' which separates the key of *ptr* from
/// it's value, or NULL if the node doesn't have a key. Only the line which
/// *ptr* is on is searched.
static char *find_colon(char *ptr) {
	char *ret = NULL;
	char quote = '\0';
	int16_t depth = 0;

	while (!is_eol(*ptr)) {
		if (quote != '\0') {
			if (*ptr == '\\' && quote == '"' && !is_eol(ptr[1])) {
				ptr++;
			}
			else if (*ptr == quote) {
				quote = '\0';
			}
			else {

			}
		}
		else if (*ptr == '"' || *ptr == '\'') {
			quote = *ptr;
		}
		else if (*ptr == '[' || *ptr == '{') {
			depth++;
		}
		else if (*ptr == ']' || *ptr == '}') {
			if (depth == 0) {
				// the flow collection which this node belongs to ended
				break;
			}
			depth--;
		}
		else if (*ptr == ',' && depth == 0) {
			// this node ended before a colon was found
			break;
		}
		else if (*ptr == ':' && depth == 0 && (is_space(ptr[1]) || is_eol(ptr[1]) ||
				ptr[1] == ',' || ptr[1] == ']' || ptr[1] == '}')) {
			ret = ptr;
			break;
		}
		else {

		}
		ptr++;
	}

	return ret;
}


/// @brief: Returns a pointer to the first character of the node's value, or
/// NULL if the value is not on the node's own line, i.e. it is a nested
/// mapping or sequence, or the node has no value at all.
static char *value_ptr(uv_yaml_node_st node) {
	char *ret = NULL;

	if (node.ptr != NULL && !is_root(node)) {
		char *p = node.ptr;
		if (is_dash(p)) {
			p = skip_space(p + 1);
		}
		else {
			char *colon = find_colon(p);
			if (colon != NULL) {
				p = skip_space(colon + 1);
			}
			else {
				// no key, the node is a value of it's own
			}
		}
		if (!is_eol(*p) &&
				!(node.flow && (*p == ',' || *p == ']' || *p == '}'))) {
			ret = p;
		}
	}

	return ret;
}


/// @brief: Returns the length of the scalar value starting at *value*.
/// For quoted scalars the length of the content between the quotes is returned.
static unsigned int scalar_len(char *value, bool flow) {
	unsigned int ret = 0;

	if (*value == '"' || *value == '\'') {
		char quote = *value;
		char *p = value + 1;
		while (!is_eol(*p) && *p != quote) {
			if (*p == '\\' && quote == '"' && !is_eol(p[1])) {
				p++;
				ret++;
			}
			p++;
			ret++;
		}
	}
	else {
		char *p = value;
		while (!is_eol(*p) &&
				!(flow && (*p == ',' || *p == ']' || *p == '}'))) {
			p++;
		}
		// plain scalars don't contain trailing whitespace
		while (p > value && is_space(*(p - 1))) {
			p--;
		}
		ret = (unsigned int) (p - value);
	}

	return ret;
}


/// @brief: Returns the first child of a node which children are written on
/// the following lines
static uv_yaml_node_st block_first_child(uv_yaml_node_st node) {
	uv_yaml_node_st ret = invalid_node();
	// the root node is not written on any line, thus it's own line is
	// already the first candidate for a child
	char *line = is_root(node) ? node.ptr : next_line(node.ptr);

	while (line != NULL) {
		char *content;
		int16_t indent;
		if (line_info(line, &content, &indent)) {
			if (indent > node.indent) {
				ret = make_node(content, indent, false);
			}
			else if (indent == node.indent && is_dash(content) &&
					!is_root(node) && !is_dash(node.ptr)) {
				// a sequence is allowed to be indented to the same column
				// with the mapping key which it belongs to
				ret = make_node(content, indent, false);
			}
			else {
				// the node ended without children
			}
			break;
		}
		line = next_line(line);
	}

	return ret;
}


/// @brief: Returns the first child of a flow collection starting at *value*
static uv_yaml_node_st flow_first_child(char *value) {
	uv_yaml_node_st ret = invalid_node();
	char *p = skip_space(value + 1);

	if (!is_eol(*p) && *p != ']' && *p != '}') {
		ret = make_node(p, 0, true);
	}

	return ret;
}


static uv_yaml_node_st first_child(uv_yaml_node_st node) {
	uv_yaml_node_st ret = invalid_node();

	if (node.ptr != NULL) {
		char *value = value_ptr(node);

		if (value == NULL) {
			ret = block_first_child(node);
		}
		else if (*value == '[' || *value == '{') {
			ret = flow_first_child(value);
		}
		else if (is_dash(node.ptr) && (is_dash(value) || find_colon(value) != NULL)) {
			// a mapping or a sequence which starts on the sequence entry's
			// own line. The rest of it is written on the following lines,
			// indented to the same column with this first child.
			ret = make_node(value,
					node.indent + (int16_t) (value - node.ptr), false);
		}
		else {
			// a scalar value doesn't have children
		}
	}

	return ret;
}


/// @brief: Returns the next sibling of a member of a flow collection
static bool flow_next_sibling(uv_yaml_node_st node, uv_yaml_node_st *dest) {
	bool ret = false;
	char *p = node.ptr;
	char quote = '\0';
	int16_t depth = 0;

	while (!is_eol(*p)) {
		if (quote != '\0') {
			if (*p == '\\' && quote == '"' && !is_eol(p[1])) {
				p++;
			}
			else if (*p == quote) {
				quote = '\0';
			}
			else {

			}
		}
		else if (*p == '"' || *p == '\'') {
			quote = *p;
		}
		else if (*p == '[' || *p == '{') {
			depth++;
		}
		else if (*p == ']' || *p == '}') {
			if (depth == 0) {
				// the flow collection ended
				break;
			}
			depth--;
		}
		else if (*p == ',' && depth == 0) {
			char *sibling = skip_space(p + 1);
			if (!is_eol(*sibling) && *sibling != ']' && *sibling != '}') {
				if (dest != NULL) {
					*dest = make_node(sibling, 0, true);
				}
				ret = true;
			}
			break;
		}
		else {

		}
		p++;
	}

	return ret;
}


bool uv_yamlreader_get_next_sibling(uv_yaml_node_st node, uv_yaml_node_st *dest) {
	bool ret = false;

	if (node.ptr != NULL && !is_root(node)) {
		if (node.flow) {
			ret = flow_next_sibling(node, dest);
		}
		else {
			bool node_is_seq = is_dash(node.ptr);
			char *line = next_line(node.ptr);

			while (line != NULL) {
				char *content;
				int16_t indent;
				if (line_info(line, &content, &indent)) {
					if (indent > node.indent) {
						// the line belongs to the children of this node
					}
					else if (indent < node.indent) {
						// the collection which this node belongs to ended
						break;
					}
					else if (is_dash(content) && !node_is_seq) {
						// a sequence indented to the same column with this
						// mapping key belongs to this key's value
					}
					else if (!is_dash(content) && node_is_seq) {
						// the sequence which this node belongs to ended
						break;
					}
					else {
						if (dest != NULL) {
							*dest = make_node(content, indent, false);
						}
						ret = true;
						break;
					}
				}
				line = next_line(line);
			}
		}
	}

	return ret;
}


uv_yaml_node_st uv_yamlreader_find_child(uv_yaml_node_st parent, const char *child_name) {
	uv_yaml_node_st ret = invalid_node();
	unsigned int name_len = (child_name != NULL) ? strlen(child_name) : 0;
	uv_yaml_node_st child = first_child(parent);

	while (uv_yaml_node_is_valid(child)) {
		char *colon = is_dash(child.ptr) ? NULL : find_colon(child.ptr);
		if (colon != NULL) {
			char *key = child.ptr;
			unsigned int key_len = (unsigned int) (colon - key);
			// strip the possible quotes around the key
			if (key_len >= 2 && (*key == '"' || *key == '\'') &&
					key[key_len - 1] == *key) {
				key++;
				key_len -= 2;
			}
			if (key_len == name_len && strncmp(key, child_name, name_len) == 0) {
				ret = child;
				break;
			}
		}
		else {
			// sequence entries are evaluated as empty names
			if (name_len == 0) {
				ret = child;
				break;
			}
		}
		if (!uv_yamlreader_get_next_sibling(child, &child)) {
			break;
		}
	}

	return ret;
}


uv_yaml_node_st uv_yamlreader_get_child(uv_yaml_node_st parent, uint16_t index) {
	uv_yaml_node_st ret = invalid_node();
	uv_yaml_node_st child = first_child(parent);
	uint16_t i = 0;

	while (uv_yaml_node_is_valid(child)) {
		if (i == index) {
			ret = child;
			break;
		}
		i++;
		if (!uv_yamlreader_get_next_sibling(child, &child)) {
			break;
		}
	}

	return ret;
}


unsigned int uv_yamlreader_get_child_count(uv_yaml_node_st parent) {
	unsigned int ret = 0;
	uv_yaml_node_st child = first_child(parent);

	while (uv_yaml_node_is_valid(child)) {
		ret++;
		if (!uv_yamlreader_get_next_sibling(child, &child)) {
			break;
		}
	}

	return ret;
}


bool uv_yamlreader_get_obj_name(uv_yaml_node_st node, char *dest, unsigned int dest_length) {
	bool ret = true;

	if (node.ptr != NULL && dest != NULL && dest_length != 0) {
		char *colon = (is_root(node) || is_dash(node.ptr)) ?
				NULL : find_colon(node.ptr);
		unsigned int len = 0;
		char *key = node.ptr;

		if (colon != NULL) {
			len = (unsigned int) (colon - key);
			if (len >= 2 && (*key == '"' || *key == '\'') && key[len - 1] == *key) {
				key++;
				len -= 2;
			}
		}
		if (len >= dest_length) {
			len = dest_length - 1;
			ret = false;
		}
		memcpy(dest, key, len);
		dest[len] = '\0';
	}
	else {
		ret = false;
	}

	return ret;
}


uv_yaml_types_e uv_yamlreader_get_type(uv_yaml_node_st node) {
	uv_yaml_types_e ret = YAML_UNSUPPORTED;

	if (node.ptr != NULL) {
		char *value = value_ptr(node);

		if (value == NULL) {
			if (node.flow) {
				// a member of a flow collection without a value
			}
			else {
				// the value is nested on the following lines
				uv_yaml_node_st child = block_first_child(node);
				if (uv_yaml_node_is_valid(child)) {
					ret = is_dash(child.ptr) ? YAML_SEQ : YAML_MAP;
				}
				else {
					// an empty value
				}
			}
		}
		else if (*value == '[') {
			ret = YAML_SEQ;
		}
		else if (*value == '{') {
			ret = YAML_MAP;
		}
		else if (is_dash(node.ptr) && is_dash(value)) {
			// a sequence which starts on the sequence entry's own line
			ret = YAML_SEQ;
		}
		else if (is_dash(node.ptr) && find_colon(value) != NULL) {
			// a mapping which starts on the sequence entry's own line
			ret = YAML_MAP;
		}
		else {
			ret = scalar_type(value, scalar_len(value, node.flow));
		}
	}

	return ret;
}


char *uv_yamlreader_get_string_ptr(uv_yaml_node_st node) {
	char *ret = value_ptr(node);

	if (ret != NULL) {
		if (*ret == '[' || *ret == '{') {
			// mappings and sequences don't have a string value
			ret = NULL;
		}
		else if (*ret == '"' || *ret == '\'') {
			ret++;
		}
		else {

		}
	}

	return ret;
}


unsigned int uv_yamlreader_get_string_len(uv_yaml_node_st node) {
	unsigned int ret = 0;
	char *value = value_ptr(node);

	if (value != NULL && *value != '[' && *value != '{') {
		ret = scalar_len(value, node.flow);
	}

	return ret;
}


bool uv_yamlreader_get_string(uv_yaml_node_st node, char *dest, unsigned int dest_length) {
	bool ret = false;
	char *value = value_ptr(node);

	if (dest != NULL && dest_length != 0) {
		dest[0] = '\0';

		if (value != NULL && *value != '[' && *value != '{') {
			bool escaped = (*value == '"');
			char *src = uv_yamlreader_get_string_ptr(node);
			unsigned int len = scalar_len(value, node.flow);
			unsigned int i = 0;
			unsigned int d = 0;
			ret = true;

			while (i < len) {
				char c = src[i];
				if (escaped && c == '\\' && i + 1 < len) {
					i++;
					switch (src[i]) {
					case 'n':
						c = '\n';
						break;
					case 't':
						c = '\t';
						break;
					case 'r':
						c = '\r';
						break;
					case '0':
						c = '\0';
						break;
					default:
						c = src[i];
						break;
					}
				}
				if (d + 1 >= dest_length) {
					// the value doesn't fit into 'dest'
					ret = false;
					break;
				}
				dest[d++] = c;
				i++;
			}
			dest[d] = '\0';
		}
	}

	return ret;
}


int uv_yamlreader_get_int(uv_yaml_node_st node) {
	int ret = 0;
	char *value = uv_yamlreader_get_string_ptr(node);

	if (value != NULL) {
		unsigned int len = uv_yamlreader_get_string_len(node);
		char str[NUMBUF_LEN];
		if (len >= sizeof(str)) {
			len = sizeof(str) - 1;
		}
		memcpy(str, value, len);
		str[len] = '\0';
		ret = strtol(str, NULL, 0);
	}

	return ret;
}


bool uv_yamlreader_get_bool(uv_yaml_node_st node) {
	bool ret = false;
	char *value = uv_yamlreader_get_string_ptr(node);

	if (value != NULL) {
		unsigned int len = uv_yamlreader_get_string_len(node);
		if ((len == 4 && str_case_eq(value, "true", 4)) ||
				(len == 3 && str_case_eq(value, "yes", 3)) ||
				(len == 2 && str_case_eq(value, "on", 2))) {
			ret = true;
		}
	}

	return ret;
}


unsigned int uv_yamlreader_seq_get_size(uv_yaml_node_st seq) {
	return uv_yamlreader_get_child_count(seq);
}


uv_yaml_node_st uv_yamlreader_seq_at(uv_yaml_node_st seq, unsigned int index) {
	return uv_yamlreader_get_child(seq, index);
}


uv_yaml_types_e uv_yamlreader_seq_get_type(uv_yaml_node_st seq, unsigned int index) {
	return uv_yamlreader_get_type(uv_yamlreader_seq_at(seq, index));
}


int uv_yamlreader_seq_get_int(uv_yaml_node_st seq, unsigned int index) {
	return uv_yamlreader_get_int(uv_yamlreader_seq_at(seq, index));
}


bool uv_yamlreader_seq_get_bool(uv_yaml_node_st seq, unsigned int index) {
	return uv_yamlreader_get_bool(uv_yamlreader_seq_at(seq, index));
}


bool uv_yamlreader_seq_get_string(uv_yaml_node_st seq, unsigned int index,
		char *dest, unsigned int dest_length) {
	return uv_yamlreader_get_string(uv_yamlreader_seq_at(seq, index), dest, dest_length);
}



/***** WRITING FUNCTIONS ******/


/// @brief: Returns ERR_BUFFER_OVERFLOW if requested length overflows from YAML buffer
static uv_errors_e check_overflow(uv_yaml_st *yaml, unsigned int length_req) {
	uv_errors_e ret = ERR_NONE;
	if (strlen(yaml->start_ptr) + length_req >= yaml->buffer_length - 1) {
		ret = ERR_BUFFER_OVERFLOW;
	}

	return ret;
}


/// @brief: Returns the count of characters which the indentation of the
/// next line requires. Each pending sequence entry's dash is written in
/// place of one indentation step, so the width is the same either way.
static unsigned int line_start_len(uv_yaml_st *yaml) {
	return yaml->depth * CONFIG_YAML_INDENT;
}


/// @brief: Writes the indentation of a new line, prefixed with the sequence
/// entries' '-' characters if any are pending
static void write_line_start(uv_yaml_st *yaml) {
	char *ptr = yaml->start_ptr + strlen(yaml->start_ptr);
	uint8_t dashes = (yaml->pending_dashes < yaml->depth) ?
			yaml->pending_dashes : yaml->depth;
	unsigned int indent = (yaml->depth - dashes) * CONFIG_YAML_INDENT;

	memset(ptr, ' ', indent);
	ptr += indent;
	for (uint8_t i = 0; i < dashes; i++) {
		*(ptr++) = '-';
		// pad the dash to the width of a full indentation step
		memset(ptr, ' ', CONFIG_YAML_INDENT - 1);
		ptr += CONFIG_YAML_INDENT - 1;
	}
	yaml->pending_dashes = 0;
	*ptr = '\0';
}


/// @brief: Returns true if this level of the YAML is a sequence
static bool in_seq(uv_yaml_st *yaml) {
	return (yaml->depth != 0 &&
			(yaml->seq_mask & (1u << (yaml->depth - 1))) != 0);
}


/// @brief: Returns true if *str* has to be quoted in order to be a valid
/// YAML scalar which reads back as a string
static bool needs_quotes(const char *str) {
	bool ret = false;
	unsigned int len = strlen(str);

	if (len == 0) {
		ret = true;
	}
	else if (is_space(str[0]) || is_space(str[len - 1])) {
		ret = true;
	}
	else if (strchr("-?:,[]{}#&*!|>'\"%@`", str[0]) != NULL) {
		ret = true;
	}
	else if (scalar_type(str, len) != YAML_STRING) {
		// the string would read back as a number or as a boolean
		ret = true;
	}
	else {
		for (unsigned int i = 0; i < len; i++) {
			char c = str[i];
			if ((c == ':' && (i + 1 == len || is_space(str[i + 1]))) ||
					(c == '#' && i != 0 && is_space(str[i - 1])) ||
					(strchr(",[]{}", c) != NULL) ||
					(c == '\n' || c == '\t' || c == '\r')) {
				ret = true;
				break;
			}
		}
	}

	return ret;
}


/// @brief: Returns the count of characters which writing *str* requires
///
/// @param quoted: True if the scalar is written inside quotes regardless of
/// what the YAML syntax would require
static unsigned int scalar_write_len(const char *str, bool quoted) {
	unsigned int ret = strlen(str);

	if (quoted || needs_quotes(str)) {
		// the quotes and the worst case escaping of every character
		ret = ret * 2 + 2;
	}

	return ret;
}


/// @brief: Writes *str* to the end of the YAML buffer, quoting and escaping
/// it if the YAML syntax requires it
///
/// @param quoted: True if the scalar is written inside quotes regardless of
/// what the YAML syntax would require
static void write_scalar(uv_yaml_st *yaml, const char *str, bool quoted) {
	char *ptr = yaml->start_ptr + strlen(yaml->start_ptr);

	if (!quoted && !needs_quotes(str)) {
		strcpy(ptr, str);
	}
	else {
		*(ptr++) = '"';
		for (const char *c = str; *c != '\0'; c++) {
			switch (*c) {
			case '"':
			case '\\':
				*(ptr++) = '\\';
				*(ptr++) = *c;
				break;
			case '\n':
				*(ptr++) = '\\';
				*(ptr++) = 'n';
				break;
			case '\t':
				*(ptr++) = '\\';
				*(ptr++) = 't';
				break;
			case '\r':
				*(ptr++) = '\\';
				*(ptr++) = 'r';
				break;
			default:
				*(ptr++) = *c;
				break;
			}
		}
		*(ptr++) = '"';
		*ptr = '\0';
	}
}


/// @brief: Writes the start of a line and the possible key of the entry.
/// After this the value can be appended to the buffer.
///
/// @param name: The key of the entry. Pass NULL for the entries of a sequence.
/// @param value_len: The count of characters which the value requires
static uv_errors_e write_entry_start(uv_yaml_st *yaml, const char *name,
		unsigned int value_len) {
	uv_errors_e ret = ERR_NONE;
	bool seq = in_seq(yaml);
	unsigned int len = line_start_len(yaml) + value_len + 1;

	if (seq) {
		// the entries of a sequence are prefixed with a dash
		len += 2;
	}
	if (name != NULL && strlen(name) != 0) {
		len += scalar_write_len(name, false) + 2;
	}

	ret = check_overflow(yaml, len);

	if (ret == ERR_NONE) {
		write_line_start(yaml);
		if (seq) {
			strcat(yaml->start_ptr, "- ");
		}
		if (name != NULL && strlen(name) != 0) {
			write_scalar(yaml, name, false);
			strcat(yaml->start_ptr, ":");
			if (value_len != 0) {
				strcat(yaml->start_ptr, " ");
			}
		}
	}

	return ret;
}


uv_errors_e uv_yamlwriter_init(uv_yaml_st *yaml, char *buffer_ptr,
		unsigned int buffer_length) {
	uv_errors_e ret = ERR_NONE;

	if (yaml != NULL && buffer_ptr != NULL) {
		yaml->start_ptr = buffer_ptr;
		yaml->buffer_length = buffer_length;
		yaml->depth = 0;
		yaml->seq_mask = 0;
		yaml->pending_dashes = 0;
		yaml->quote_strings = false;
		yaml->start_ptr[0] = '\0';
	}
	else {
		ret = ERR_NULL_PTR;
	}

	return ret;
}


uv_errors_e uv_yamlwriter_end(uv_yaml_st *yaml, uv_yaml_errors_e *errors) {
	uv_errors_e ret = ERR_NONE;

	if (errors != NULL) {
		*errors = YAML_ERR_NONE;
	}
	if (yaml->depth != 0) {
		if (errors != NULL) {
			*errors = YAML_ERR_UNTERMINATED_OBJ;
		}
		ret = ERR_INTERNAL | HAL_MODULE_YAML;
	}

	return ret;
}


/// @brief: Starts a mapping or a sequence
static uv_errors_e begin_mapseq(uv_yaml_st *yaml, const char *name, bool seq) {
	uv_errors_e ret = ERR_NONE;

	if (yaml->depth >= 32) {
		ret = ERR_BUFFER_OVERFLOW;
	}
	else {
		if (name == NULL || strlen(name) == 0) {
			// an entry of a sequence. The dash is written together with
			// the first entry of this collection
			if (in_seq(yaml)) {
				yaml->pending_dashes++;
			}
		}
		else {
			ret = write_entry_start(yaml, name, 0);
			if (ret == ERR_NONE) {
				strcat(yaml->start_ptr, "\n");
			}
		}

		if (ret == ERR_NONE) {
			yaml->depth++;
			if (seq) {
				yaml->seq_mask |= (1u << (yaml->depth - 1));
			}
			else {
				yaml->seq_mask &= ~(1u << (yaml->depth - 1));
			}
		}
	}

	return ret;
}


/// @brief: Ends a mapping or a sequence
static uv_errors_e end_mapseq(uv_yaml_st *yaml, bool seq) {
	uv_errors_e ret = ERR_NONE;

	if (yaml->depth == 0) {
		ret = ERR_INTERNAL | HAL_MODULE_YAML;
	}
	else {
		if (yaml->pending_dashes != 0) {
			// the collection was written as a sequence entry but it was
			// left empty. Write it as an empty flow collection.
			ret = check_overflow(yaml, line_start_len(yaml) + 3);
			if (ret == ERR_NONE) {
				write_line_start(yaml);
				strcat(yaml->start_ptr, seq ? "[]\n" : "{}\n");
			}
		}
		yaml->depth--;
	}

	return ret;
}


uv_errors_e uv_yamlwriter_begin_map(uv_yaml_st *yaml, const char *name) {
	return begin_mapseq(yaml, name, false);
}


uv_errors_e uv_yamlwriter_end_map(uv_yaml_st *yaml) {
	return end_mapseq(yaml, false);
}


uv_errors_e uv_yamlwriter_begin_seq(uv_yaml_st *yaml, const char *name) {
	return begin_mapseq(yaml, name, true);
}


uv_errors_e uv_yamlwriter_end_seq(uv_yaml_st *yaml) {
	return end_mapseq(yaml, true);
}


/// @brief: Writes a key-value pair which value is already formatted
/// as a valid YAML scalar
static uv_errors_e add_value(uv_yaml_st *yaml, const char *name, const char *value) {
	uv_errors_e ret = write_entry_start(yaml, name, strlen(value));

	if (ret == ERR_NONE) {
		strcat(yaml->start_ptr, value);
		strcat(yaml->start_ptr, "\n");
	}

	return ret;
}


uv_errors_e uv_yamlwriter_add_int(uv_yaml_st *yaml, const char *name, int value) {
	char v[NUMBUF_LEN];
	snprintf(v, sizeof(v), "%i", value);

	return add_value(yaml, name, v);
}


uv_errors_e uv_yamlwriter_add_int_hex(uv_yaml_st *yaml, const char *name, uint32_t value) {
	char v[NUMBUF_LEN];
	snprintf(v, sizeof(v), "0x%x", value);

	return add_value(yaml, name, v);
}


uv_errors_e uv_yamlwriter_seq_add_int(uv_yaml_st *yaml, int value) {
	return uv_yamlwriter_add_int(yaml, NULL, value);
}


uv_errors_e uv_yamlwriter_seq_add_int_hex(uv_yaml_st *yaml, uint32_t value) {
	return uv_yamlwriter_add_int_hex(yaml, NULL, value);
}


uv_errors_e uv_yamlwriter_add_bool(uv_yaml_st *yaml, const char *name, bool value) {
	return add_value(yaml, name, value ? "true" : "false");
}


uv_errors_e uv_yamlwriter_seq_add_bool(uv_yaml_st *yaml, bool value) {
	return uv_yamlwriter_add_bool(yaml, NULL, value);
}


void uv_yamlwriter_set_quote_strings(uv_yaml_st *yaml, bool value) {
	yaml->quote_strings = value;
}


uv_errors_e uv_yamlwriter_add_string(uv_yaml_st *yaml, const char *name, const char *value) {
	bool quoted = yaml->quote_strings;
	uv_errors_e ret = write_entry_start(yaml, name, scalar_write_len(value, quoted));

	if (ret == ERR_NONE) {
		write_scalar(yaml, value, quoted);
		strcat(yaml->start_ptr, "\n");
	}

	return ret;
}


uv_errors_e uv_yamlwriter_seq_add_string(uv_yaml_st *yaml, const char *value) {
	return uv_yamlwriter_add_string(yaml, NULL, value);
}


bool uv_yamlwriter_append_yaml(uv_yaml_st *yaml, const char *data) {
	bool ret = true;
	const char *line = data;

	while (*line != '\0' && ret) {
		const char *end = line;
		while (!is_eol(*end)) {
			end++;
		}
		unsigned int len = (unsigned int) (end - line);

		// empty lines are written without any indentation
		if (len != 0) {
			if (check_overflow(yaml, line_start_len(yaml) + len + 1) != ERR_NONE) {
				ret = false;
			}
			else {
				write_line_start(yaml);
				char *ptr = yaml->start_ptr + strlen(yaml->start_ptr);
				memcpy(ptr, line, len);
				ptr[len] = '\n';
				ptr[len + 1] = '\0';
			}
		}
		else if (check_overflow(yaml, 1) != ERR_NONE) {
			ret = false;
		}
		else {
			strcat(yaml->start_ptr, "\n");
		}

		line = (*end == '\0') ? end : (end + 1);
	}

	return ret;
}


#endif
