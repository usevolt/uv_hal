/* 
 * This file is part of the uv_hal distribution (www.usevolt.fi).
 * Copyright (c) 2017 Usevolt Oy.
 * 
 * This program is free software: you can redistribute it and/or modify  
 * it under the terms of the GNU General Public License as published by  
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but 
 * WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef UW_JSON_H_
#define UW_JSON_H_


#include "uv_hal_config.h"


#if CONFIG_JSON

/// @file: A lightweight JSON parser capable of creating or parsing JSON strings.
/// Note that floating points aren't currently supported.


#include <uv_errors.h>
#include <stdbool.h>


/// @brief: Describes the different internal errors which the JSON parsing can cause
typedef enum {
	/// @brief: No errors detected
	JSON_ERR_NONE = 0,
	/// @brief: At least one of the objects in JSON was not terminated,
	/// e.g. '}' is missing somewhere
	JSON_ERR_UNTERMINATED_OBJ,
	/// @brief: Value assigned to a object or key is of bad type.
	/// For example, string without " characters at end and beginning.
	JSON_ERR_BAD_VALUE,
	/// @brief: A unknown character encountered in the JSON file.
	/// The JSON syntax doesn't match.
	JSON_ERR_SYNTAX
} uv_json_errors_e;


/// @brief: Defines all supported value types
/// Values which can contain child key-value pairs are negative and
/// other values are positive.
typedef enum {
	JSON_OBJECT = -100,
	JSON_ARRAY,
	JSON_UNSUPPORTED = 0,
	JSON_INT = 1,
	JSON_BOOL,
	JSON_STRING
} uv_json_types_e;

static inline bool uv_json_is_objarray(uv_json_types_e type) {
	return (type < JSON_UNSUPPORTED);
}


/// @brief: The JSON data structure
/// contains status data from the JSON to be constructed
typedef struct {
	// pointer pointing to the start of the writing buffer
	char *start_ptr;
	unsigned int buffer_length;
} uv_json_st;


/// @brief: Init's a JSON reader. This function should be called before
/// any other calls
///
/// @return: A uv_errors_e value describing possible encountered errors. Refer to
/// uv_errors_h for more details and error handling.
///
/// @param json: A pointer to the JSON object which is used to construct the JSON file.
/// @param buffer_ptr: A pointer to a string buffer which contains the JSON to be parsed
/// OR a string where the constructed json will be saved.
/// @param buffer_length: The maximum length in bytes of the buffer.
uv_errors_e uv_jsonreader_init(char *buffer_ptr, unsigned int buffer_length);


/// @brief: Init's a JSON writer. This function should be called before
/// any other calls
///
/// @return: A uv_errors_e value describing possible encountered errors. Refer to
/// uv_errors_h for more details and error handling.
///
/// @param json: A pointer to the JSON object which is used to construct the JSON file.
/// @param buffer_ptr: A pointer to a string buffer which contains the JSON to be parsed
/// OR a string where the constructed json will be saved.
/// @param buffer_length: The maximum length in bytes of the buffer.
uv_errors_e uv_jsonwriter_init(uv_json_st *json, char *buffer_ptr, unsigned int buffer_length);



/***** WRITING FUNCTIONS ******/


/// @brief: Should be called as the last function when finishing the writing to JSON.
/// At this point, all objects and arrays insinde the JSON should be terminated accordingly.
///
/// @return: ERR_NONE if JSON was successfully ended, otherwise ERR_INTERNAL.
/// If errors-parameter was given, it will be written with the encountered error enum.
///
/// @param errors: A pointer to variable which will be written with a detailed error
/// encountered. Pass NULL if not used.
uv_errors_e uv_jsonwriter_end(uv_json_st *json, uv_json_errors_e *errors);


/// @brief: Starts to write a JSON object
///
/// @param name: The name of the object
uv_errors_e uv_jsonwriter_begin_object(uv_json_st *json, char *name);

/// @brief: Ends a write of a JSON object
///
/// @param name: The name of the object
uv_errors_e uv_jsonwriter_end_object(uv_json_st *json);

/// @brief: Starts to write a JSON array
///
/// @param name: The name of the object
uv_errors_e uv_jsonwriter_begin_array(uv_json_st *json, char *name);

/// @brief: Ends a write of a JSON array
///
/// @param name: The name of the object
uv_errors_e uv_jsonwriter_end_array(uv_json_st *json);

/// @brief: Writes an integer to a JSON key-value pair
///
/// @param name: The name of the object
/// @param value: The value
uv_errors_e uv_jsonwriter_add_int(uv_json_st *json, char *name, int value);

/// @brief: Writes an integer value to an array
uv_errors_e uv_jsonwriter_array_add_int(uv_json_st *json, int value);

/// @brief: Writes a string to a JSON key-value pair
///
/// @param name: The name of the object
/// @param value: The value
uv_errors_e uv_jsonwriter_add_string(uv_json_st *json, char *name, char *value);

/// @brief: Writes a string to an array
uv_errors_e uv_jsonwriter_array_add_string(uv_json_st *json, char *value);

/// @brief: Writes a boolean to a JSON key-value pair
///
/// @param name: The name of the object
/// @param value: The value
uv_errors_e uv_jsonwriter_add_bool(uv_json_st *json, char *name, bool value);

/// @brief: Writes a boolean to an array
uv_errors_e uv_jsonwriter_array_add_bool(uv_json_st *json, bool value);


/***** READING FUNCTIONS ******/

/// @brief: Gives a pointer pointing to the start to the next sibling coming after 'object'
///
/// @return: true if the next sibling could be found, false otherwise.
///
/// @param object: Pointer to the object whose siblings are searched.
/// @param dest: A pointer to a char pointer where a reference to the found sibling
/// will be stored. If sibling couldn't be found, this function doesn't modify dest at all.
bool uv_jsonreader_get_next_sibling(char *object, char **dest);



/// @brief: Finds and stores a child object with a key 'key' from 'object' parent to 'dest'.
/// Child is searched recursively 'depth' many times. For unlimited depth, pass -1.
///
/// @note: Since array-type object's children don't have names, they are evaluated as an
/// empty strings.
///
/// @return: true if matching child could be found, false otherwise.
///
/// @param parent: Pointer to the parent object which child's are parsed recursively
/// @param child_name: The name of the child which is searched.
/// @param depth: The depth of how deep recursively will be searched. For unlimited depth,
/// pass a negative value.
/// @param dest: If the child is found, it's pointer is stored in here. Otherwise this will be
/// left untouched. Passing NULL here allows to use this function to only check is a child exists
/// without getting a pointer to it.
char *uv_jsonreader_find_child(char *parent, char *child_name,
		int depth);



/// @brief: Stores the name of the object 'object' to 'dest'
/// If the object is a cell in array, it doesn't have a name. In this case
/// a null string is returned.
///
/// @return: true if name could be stored in 'dest'. false if the name
/// was too long to fit into 'dest'.
///
/// @param object: The object which name will be stored.
/// @param dest: A pointer to string where the name will be stored.
/// @param dest_length: The max size of dest. If the name didn't fit dest,
/// false is returned. However, 'dest_length' bytes will be stored to dest anyway.
bool uv_jsonreader_get_obj_name(char *object, char **dest, unsigned int dest_length);


/// @brief: Returns the type of this JSON object
uv_json_types_e uv_jsonreader_get_type(char *object);


/// @brief: Returns the object's value as an integer
int uv_jsonreader_get_int(char *object);

/// @brief: Returns the array's cell value as an integer
///
/// @note: Do not overindex!
int uv_jsonreader_array_get_int(char *object, unsigned int index);


/// @brief: Passes the object's value as a null-terminated string to 'dest'
/// If the string is longer than dest_length (including the termination '\0' char),
/// returns false.
bool uv_jsonreader_get_string(char *object, char *dest, unsigned int dest_length);

/// @brief: Passes the array cell's value as a null-terminated string to 'dest'
/// If the string is longer than dest_length, returns false.
///
/// @note: Do not overindex!
bool uv_jsonreader_array_get_string(char *object, unsigned int index, char **dest, unsigned int dest_length);


/// @brief: Returns the object's value as a bool
/// All other values than 'true' are evaluated as false
bool uv_jsonreader_get_bool(char *object);

/// @brief: Returns the array cell's value as a bool
///
/// @note: Do not overindex!
bool uv_jsonreader_array_get_bool(char *object, unsigned int index);

/// @brief: indexes arrays values
char *uv_jsonreader_array_at(char *object, unsigned int index);

/// @brief: Returns the array's child count
unsigned int uv_jsonreader_array_get_size(char *array);

#endif

#endif /* UW_JSON_H_ */
