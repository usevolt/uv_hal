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

#ifndef UW_YAML_H_
#define UW_YAML_H_

#include <stdint.h>
#include "uv_hal_config.h"


#if CONFIG_YAML

/// @file: A lightweight YAML parser and writer, modeled after uv_json.
/// Like the JSON module, the reader doesn't allocate any memory: The document
/// is parsed in place from the buffer given to *uv_yamlreader_init* and the
/// nodes returned by the reader merely refer to that buffer. This means that
/// the buffer has to stay valid and unmodified as long as the nodes are used.
///
/// Supported subset of the YAML syntax:
///		* Block mappings			key: value
///		* Block sequences			- value
///		* Nesting of the above with indentation, including mappings which
///		  start on the sequence entry's own line (- key: value)
///		* Flow collections			[1, 2, 3] and {min: 0, max: 100}
///		* Plain, 'single quoted' and "double quoted" scalars
///		* Comments (# ...), which are removed by *uv_yamlreader_init*
///		* '---' and '...' lines, which are skipped
///
/// Not supported (to keep this module simple):
///		* Block scalars (| and >), i.e. multiline scalar values
///		* Flow collections spanning over multiple lines. A flow collection
///		  has to fit on a single line.
///		* Anchors (&), aliases (*), merge keys (<<) and tags (!!)
///		* Multiple documents in a single file. All '---' lines are skipped,
///		  which merges the documents into one.
///		* Floating point values, matching uv_json which doesn't support them either
///
/// Note that as YAML is indentation sensitive, tabs shouldn't be used for
/// indentation. If they are, a tab is counted as a single indentation step.


#include <uv_errors.h>
#include <stdbool.h>
#include <stddef.h>


#if !defined(CONFIG_YAML_INDENT)
/// @brief: The count of spaces which the writer indents each nesting level with.
/// Has to be 2 or more.
#define CONFIG_YAML_INDENT		2
#endif


/// @brief: Describes the different internal errors which the YAML parsing can cause
typedef enum {
	/// @brief: No errors detected
	YAML_ERR_NONE = 0,
	/// @brief: At least one of the mappings or sequences was not terminated,
	/// e.g. *uv_yamlwriter_end_map* is missing somewhere
	YAML_ERR_UNTERMINATED_OBJ,
	/// @brief: Value assigned to a node is of bad type
	YAML_ERR_BAD_VALUE,
	/// @brief: A unknown character encountered in the YAML file.
	/// The YAML syntax doesn't match.
	YAML_ERR_SYNTAX
} uv_yaml_errors_e;


/// @brief: Defines all supported value types
/// Values which can contain child nodes are negative and
/// other values are positive.
typedef enum {
	/// @brief: A mapping, i.e. a collection of named child nodes
	YAML_MAP = -100,
	/// @brief: A sequence, i.e. a collection of unnamed child nodes
	YAML_SEQ,
	YAML_UNSUPPORTED = 0,
	YAML_INT = 1,
	YAML_BOOL,
	YAML_STRING
} uv_yaml_types_e;

const char *uv_yaml_type_to_str(uv_yaml_types_e type);

/// @brief: Returns true if *type* is a mapping or a sequence, i.e. a node
/// which can contain children
static inline bool uv_yaml_is_mapseq(uv_yaml_types_e type) {
	return (type < YAML_UNSUPPORTED);
}


/// @brief: Refers to a single node in the YAML document. Since YAML is
/// indentation sensitive, a plain pointer is not enough to describe a node,
/// but the indentation of the node has to be carried along with it.
///
/// @note: All members are for the module's internal use. Use
/// *uv_yaml_node_is_valid* to check if the node refers to anything.
typedef struct {
	/// @brief: Points to the first character of this node, i.e. to the key
	/// of a mapping entry, to the '-' of a sequence entry, or to the value
	/// itself when the node is a member of a flow collection.
	char *ptr;
	/// @brief: The column of *ptr* on it's line. Negative for the document root.
	int16_t indent;
	/// @brief: True if this node is a member of a flow collection, in which
	/// case the indentation carries no meaning.
	bool flow;
} uv_yaml_node_st;


/// @brief: Returns true if *node* refers to an existing node. The reader
/// functions return an invalid node when the requested node was not found.
static inline bool uv_yaml_node_is_valid(uv_yaml_node_st node) {
	return (node.ptr != NULL);
}


/// @brief: The YAML data structure
/// contains status data from the YAML to be constructed
typedef struct {
	// pointer pointing to the start of the writing buffer
	char *start_ptr;
	unsigned int buffer_length;
	// the count of currently open mappings and sequences
	uint8_t depth;
	// bit mask telling which of the open levels are sequences
	uint32_t seq_mask;
	// the count of "- " prefixes which the next written line should start
	// with, i.e. how many mappings or sequences were started as an entry of
	// a sequence without writing anything yet. Nested sequence entries
	// ("- - value") need more than one.
	uint8_t pending_dashes;
} uv_yaml_st;


/// @brief: Init's a YAML reader. This function should be called before
/// any other reading calls. It modifies *buffer_ptr* in place by removing
/// the comments, the carriage returns and the trailing whitespace.
///
/// @return: A uv_errors_e value describing possible encountered errors. Refer to
/// uv_errors_h for more details and error handling.
///
/// @param buffer_ptr: A pointer to a null-terminated string buffer which
/// contains the YAML document to be parsed.
/// @param buffer_length: The maximum length in bytes of the buffer.
uv_errors_e uv_yamlreader_init(char *buffer_ptr, unsigned int buffer_length);


/// @brief: Init's a YAML writer. This function should be called before
/// any other writing calls.
///
/// @return: A uv_errors_e value describing possible encountered errors. Refer to
/// uv_errors_h for more details and error handling.
///
/// @param yaml: A pointer to the YAML object which is used to construct the YAML file.
/// @param buffer_ptr: A pointer to a string where the constructed YAML will be saved.
/// @param buffer_length: The maximum length in bytes of the buffer.
uv_errors_e uv_yamlwriter_init(uv_yaml_st *yaml, char *buffer_ptr,
		unsigned int buffer_length);



/***** WRITING FUNCTIONS ******/


/// @brief: Should be called as the last function when finishing the writing to YAML.
/// At this point, all mappings and sequences inside the YAML should be
/// terminated accordingly.
///
/// @return: ERR_NONE if YAML was successfully ended, otherwise ERR_INTERNAL.
/// If errors-parameter was given, it will be written with the encountered error enum.
///
/// @param errors: A pointer to variable which will be written with a detailed error
/// encountered. Pass NULL if not used.
uv_errors_e uv_yamlwriter_end(uv_yaml_st *yaml, uv_yaml_errors_e *errors);


/// @brief: Starts to write a YAML mapping
///
/// @param name: The name of the mapping. Pass NULL or an empty string when the
/// mapping is written as an entry of a sequence.
uv_errors_e uv_yamlwriter_begin_map(uv_yaml_st *yaml, const char *name);

/// @brief: Ends a write of a YAML mapping
uv_errors_e uv_yamlwriter_end_map(uv_yaml_st *yaml);

/// @brief: Starts to write a YAML sequence
///
/// @param name: The name of the sequence. Pass NULL or an empty string when the
/// sequence is written as an entry of another sequence.
uv_errors_e uv_yamlwriter_begin_seq(uv_yaml_st *yaml, const char *name);

/// @brief: Ends a write of a YAML sequence
uv_errors_e uv_yamlwriter_end_seq(uv_yaml_st *yaml);

/// @brief: Writes an integer to a YAML key-value pair
///
/// @param name: The name of the key
/// @param value: The value
uv_errors_e uv_yamlwriter_add_int(uv_yaml_st *yaml, const char *name, int value);

/// @brief: As *uv_yamlwriter_add_int* except adds the integer as a hexadecimal value.
///
/// @note: *value* is considered as unsigned value
uv_errors_e uv_yamlwriter_add_int_hex(uv_yaml_st *yaml, const char *name, uint32_t value);

/// @brief: Writes an integer value to a sequence
uv_errors_e uv_yamlwriter_seq_add_int(uv_yaml_st *yaml, int value);

/// @brief: Writes a hexadecimal integer value to a sequence
uv_errors_e uv_yamlwriter_seq_add_int_hex(uv_yaml_st *yaml, uint32_t value);

/// @brief: Writes a string to a YAML key-value pair. The string is quoted
/// and escaped if the YAML syntax requires it.
///
/// @param name: The name of the key
/// @param value: The value
uv_errors_e uv_yamlwriter_add_string(uv_yaml_st *yaml, const char *name, const char *value);

/// @brief: Writes a string to a sequence
uv_errors_e uv_yamlwriter_seq_add_string(uv_yaml_st *yaml, const char *value);

/// @brief: Writes a boolean to a YAML key-value pair
///
/// @param name: The name of the key
/// @param value: The value
uv_errors_e uv_yamlwriter_add_bool(uv_yaml_st *yaml, const char *name, bool value);

/// @brief: Writes a boolean to a sequence
uv_errors_e uv_yamlwriter_seq_add_bool(uv_yaml_st *yaml, bool value);


/// @brief: Appends YAML data to the YAML file. Every line of *data* is
/// indented to the current nesting level, i.e. *data* should be written as if
/// it was a document of it's own. The application is responsible that appending
/// the data actually results in a valid YAML file.
///
/// @return: True on success, false if the YAML buffer would overflow
bool uv_yamlwriter_append_yaml(uv_yaml_st *yaml, const char *data);



/***** READING FUNCTIONS ******/


/// @brief: Returns the root node of the document. The root node is a mapping
/// or a sequence which contains all the top level nodes of the document as
/// it's children.
///
/// @param buffer_ptr: The buffer which was given to *uv_yamlreader_init*
uv_yaml_node_st uv_yamlreader_get_root(char *buffer_ptr);


/// @brief: Gives the next sibling coming after *node*
///
/// @return: true if the next sibling could be found, false otherwise.
///
/// @param node: The node whose siblings are searched.
/// @param dest: A pointer to a node where the found sibling will be stored.
/// If the sibling couldn't be found, this function doesn't modify dest at all.
bool uv_yamlreader_get_next_sibling(uv_yaml_node_st node, uv_yaml_node_st *dest);


/// @brief: Finds a child node with a key *child_name* from the *parent* node.
///
/// @note: Since sequence entries don't have names, they are evaluated as
/// empty strings.
///
/// @return: The child node, or an invalid node if the child couldn't be found.
/// Use *uv_yaml_node_is_valid* to check the return value.
///
/// @param parent: The parent node which children are searched.
/// @param child_name: The name of the child which is searched.
uv_yaml_node_st uv_yamlreader_find_child(uv_yaml_node_st parent, const char *child_name);


/// @brief: Returns the *index*'th child of the *parent* node, or an invalid
/// node if the parent doesn't have that many children.
uv_yaml_node_st uv_yamlreader_get_child(uv_yaml_node_st parent, uint16_t index);


/// @brief: Returns the count of children which *parent* has
unsigned int uv_yamlreader_get_child_count(uv_yaml_node_st parent);


/// @brief: Stores the name of the *node* to 'dest'.
/// If the node is an entry of a sequence, it doesn't have a name. In this case
/// a null string is returned.
///
/// @return: true if the name could be stored in 'dest'. false if the name
/// was too long to fit into 'dest'.
///
/// @param node: The node which name will be stored.
/// @param dest: A pointer to string where the name will be stored.
/// @param dest_length: The max size of dest. If the name didn't fit dest,
/// false is returned. However, 'dest_length' bytes will be stored to dest anyway.
bool uv_yamlreader_get_obj_name(uv_yaml_node_st node, char *dest, unsigned int dest_length);


/// @brief: Returns the type of this YAML node
uv_yaml_types_e uv_yamlreader_get_type(uv_yaml_node_st node);


/// @brief: Returns the node's value as an integer.
/// Hexadecimal values prefixed with '0x' are supported.
int uv_yamlreader_get_int(uv_yaml_node_st node);


/// @brief: Passes the node's value as a null-terminated string to 'dest'.
/// Quotes are stripped and the basic escape sequences of a double quoted
/// scalar are resolved.
/// If the string is longer than dest_length (including the termination '\0' char),
/// returns false.
bool uv_yamlreader_get_string(uv_yaml_node_st node, char *dest, unsigned int dest_length);

/// @brief: Returns a pointer to the string value of **node**.
/// Note: The string is **not** null-terminated and possible escape
/// sequences are **not** resolved!
char *uv_yamlreader_get_string_ptr(uv_yaml_node_st node);

/// @brief: Returns the length of the string value, i.e. the count of the
/// characters which *uv_yamlreader_get_string_ptr* refers to.
unsigned int uv_yamlreader_get_string_len(uv_yaml_node_st node);


/// @brief: Returns the node's value as a bool.
/// 'true', 'yes' and 'on' are evaluated as true, all other values as false.
bool uv_yamlreader_get_bool(uv_yaml_node_st node);


/// @brief: Returns the sequence's entry count
unsigned int uv_yamlreader_seq_get_size(uv_yaml_node_st seq);

/// @brief: Indexes the sequence's entries
uv_yaml_node_st uv_yamlreader_seq_at(uv_yaml_node_st seq, unsigned int index);

/// @brief: Returns the type of the sequence entry at index *index*
uv_yaml_types_e uv_yamlreader_seq_get_type(uv_yaml_node_st seq, unsigned int index);

/// @brief: Returns the sequence entry's value as an integer
int uv_yamlreader_seq_get_int(uv_yaml_node_st seq, unsigned int index);

/// @brief: Returns the sequence entry's value as a bool
bool uv_yamlreader_seq_get_bool(uv_yaml_node_st seq, unsigned int index);

/// @brief: Passes the sequence entry's value as a null-terminated string to 'dest'.
/// If the string is longer than dest_length, returns false.
bool uv_yamlreader_seq_get_string(uv_yaml_node_st seq, unsigned int index,
		char *dest, unsigned int dest_length);


#endif

#endif /* UW_YAML_H_ */
