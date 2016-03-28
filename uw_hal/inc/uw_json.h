/*
 * uw_json.h
 *
 *  Created on: Mar 4, 2016
 *      Author: usevolt
 */

#ifndef UW_JSON_H_
#define UW_JSON_H_


#include "uw_hal_config.h"


/// @file: A lightweight JSON parser capable of creating or parsing JSON strings.
/// Note that floating points aren't currently supported.


#include <uw_errors.h>
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
} uw_json_errors_e;


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
} uw_json_types_e;


/// @brief: The JSON data structure
/// contains status data from the JSON to be constructed
typedef struct {
	// pointer pointing to the start of the writing buffer
	char *start_ptr;
	unsigned int buffer_length;
} uw_json_st;


/// @brief: Init's a JSON reader. This function should be called before
/// any other calls
///
/// @return: A uw_errors_e value describing possible encountered errors. Refer to
/// uw_errors_h for more details and error handling.
///
/// @param json: A pointer to the JSON object which is used to construct the JSON file.
/// @param buffer_ptr: A pointer to a string buffer which contains the JSON to be parsed
/// OR a string where the constructed json will be saved.
/// @param buffer_length: The maximum length in bytes of the buffer.
uw_errors_e uw_json_init(uw_json_st *json, char *buffer_ptr, unsigned int buffer_length);




/***** WRITING FUNCTIONS ******/

/// @brief: Should be called after uw_json_init when starting to write JSON file
/// Can also be used to reset a writing JSON and start over
uw_errors_e uw_json_write_begin(uw_json_st *json);


/// @brief: Should be called as the last function when finishing the writing to JSON.
/// At this point, all objects and arrays insinde the JSON should be terminated accordingly.
///
/// @return: ERR_NONE if JSON was successfully ended, otherwise ERR_INTERNAL.
/// If errors-parameter was given, it will be written with the encountered error enum.
///
/// @param errors: A pointer to variable which will be written with a detailed error
/// encountered. Pass NULL if not used.
uw_errors_e uw_json_write_end(uw_json_st *json, uw_json_errors_e *errors);


/// @brief: Starts to write a JSON object
///
/// @param name: The name of the object
uw_errors_e uw_json_begin_object(uw_json_st *json, char *name);

/// @brief: Ends a write of a JSON object
///
/// @param name: The name of the object
uw_errors_e uw_json_end_object(uw_json_st *json);

/// @brief: Starts to write a JSON array
///
/// @param name: The name of the object
uw_errors_e uw_json_begin_array(uw_json_st *json, char *name);

/// @brief: Ends a write of a JSON array
///
/// @param name: The name of the object
uw_errors_e uw_json_end_array(uw_json_st *json);

/// @brief: Writes an integer to a JSON key-value pair
///
/// @param name: The name of the object
/// @param value: The value
uw_errors_e uw_json_add_int(uw_json_st *json, char *name, int value);

/// @brief: Writes a string to a JSON key-value pair
///
/// @param name: The name of the object
/// @param value: The value
uw_errors_e uw_json_add_string(uw_json_st *json, char *name, char *value);

/// @brief: Writes a boolean to a JSON key-value pair
///
/// @param name: The name of the object
/// @param value: The value
uw_errors_e uw_json_add_bool(uw_json_st *json, char *name, bool value);


/***** READING FUNCTIONS ******/

/// @brief: Gives a pointer pointing to the start to the next sibling coming after 'object'
///
/// @return: true if the next sibling could be found, false otherwise.
///
/// @param object: Pointer to the object whose siblings are searched.
/// @param dest: A pointer to a char pointer where a reference to the found sibling
/// will be stored. If sibling couldn't be found, this function doesn't modify dest at all.
bool uw_json_get_next_sibling(char *object, char **dest);


/// @brief: Gives a pointer pointing to the start to the next child
///
/// @return: true if the next child could be found, false otherwise.
///
/// @param parent: A pointer to the parent object whose child will be looped trough.
/// @param child_index: A index number of the child which is requested
/// @param dest: A pointer to a char pointer where a reference to the found child
/// will be stored. If sibling couldn't be found, this function doesn't modify dest at all.
bool uw_json_get_child(char *parent, unsigned int child_index, char **dest);


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
bool uw_json_find_child(char *parent, char *child_name,
		int depth, char **dest);



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
bool uw_json_get_obj_name(char *object, char **dest, unsigned int dest_length);


/// @brief: Returns the type of this JSON object
uw_json_types_e uw_json_get_type(char *object);


/// @brief: Returns the object's value as an integer
int uw_json_get_int(char *object);


/// @brief: Passes the object's value as a null-terminated string to 'dest'
/// If the string is longer than dest_length (including the termination '\0' char),
/// returns false.
bool uw_json_get_string(char *object, char **dest, unsigned int dest_length);


/// @brief: Returns the object's value as a bool
/// All other values than 'true' are evaluated as false
bool uw_json_get_bool(char *object);


#endif /* UW_JSON_H_ */
