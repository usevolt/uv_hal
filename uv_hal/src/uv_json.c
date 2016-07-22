/*
 * uv_json.c
 *
 *  Created on: Mar 4, 2016
 *      Author: usevolt
 */

#include "uv_json.h"


#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>


/// @brief: Returns ERR_BUFFER_OVERFLOW if requested length overflows from JSON buffer
static uv_errors_e check_overflow(uv_json_st *json, unsigned int length_req) {
	if (strlen(json->start_ptr) + length_req >= json->buffer_length - 1) {
		__uv_err_throw(ERR_BUFFER_OVERFLOW | HAL_MODULE_JSON);
	}

	return uv_err(ERR_NONE);
}


uv_errors_e uv_jsonreader_init(uv_json_st *json, char *buffer_ptr, unsigned int buffer_length) {
	json->start_ptr = buffer_ptr;
 	json->buffer_length = buffer_length;
	unsigned int count = 0;
	char *ptr;
	// remove all whitespace
	for (ptr = json->start_ptr; ptr < json->start_ptr + json->buffer_length; ptr++) {
		if (isspace((int) *ptr)) {
			count++;
		}
		else {
			*(ptr - count) = *ptr;
		}
	}
	*(json->start_ptr + json->buffer_length - count + 1) = '\0';

	return uv_err(ERR_NONE);
}



uv_errors_e uv_jsonwriter_init(uv_json_st *json, char *buffer_ptr, unsigned int buffer_length) {
	json->start_ptr = buffer_ptr;
 	json->buffer_length = buffer_length;
	sprintf(json->start_ptr, "{");
	return uv_err(ERR_NONE);
}



uv_errors_e uv_jsonwriter_end(uv_json_st *json, uv_json_errors_e *errors) {
	uv_err_pass(check_overflow(json, 1));

	unsigned int size = strlen(json->start_ptr);
	if (size && json->start_ptr[size - 1] == ',') {
		json->start_ptr[size - 1] = '\0';
	}
	strcat(json->start_ptr, "}");

	// check for unterminated objects
	unsigned int count = 0;
	char *ptr;
	for (ptr = json->start_ptr; ptr < json->start_ptr + strlen(json->start_ptr); ptr++) {
		if (*ptr == '{' || *ptr == '[') {
			count++;
		}
		else if (*ptr == '}' || *ptr == ']') {
			count--;
		}
	}

	if (count) {
		if (errors) {
			*errors = JSON_ERR_UNTERMINATED_OBJ;
		}
		__uv_err_throw(ERR_INTERNAL | HAL_MODULE_JSON);
	}
	if (errors) {
		*errors = JSON_ERR_NONE;
	}

	return uv_err(ERR_NONE);
}


uv_errors_e uv_jsonwriter_begin_object(uv_json_st *json, char *name) {
	unsigned int len = strlen(name) + 4;

	uv_err_pass(check_overflow(json, len));

	snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":{", name);

	return uv_err(ERR_NONE);
}

uv_errors_e uv_jsonwriter_end_object(uv_json_st *json) {
	uv_err_pass(check_overflow(json, 2));

	unsigned int size = strlen(json->start_ptr);
	if (size && json->start_ptr[size - 1] == ',') {
		json->start_ptr[size - 1] = '\0';
	}
	strcat(json->start_ptr, "},");

	return uv_err(ERR_NONE);
}

uv_errors_e uv_jsonwriter_begin_array(uv_json_st *json, char *name) {
	unsigned int len = strlen(name) + 4;

	uv_err_pass(check_overflow(json, len));

	snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":[", name);

	return uv_err(ERR_NONE);
}

uv_errors_e uv_jsonwriter_end_array(uv_json_st *json) {
	uv_err_pass(check_overflow(json, 2));

	unsigned int size = strlen(json->start_ptr);
	if (size && json->start_ptr[size - 1] == ',') {
		json->start_ptr[size - 1] = '\0';
	}
	strcat(json->start_ptr, "],");

	return uv_err(ERR_NONE);
}

uv_errors_e uv_jsonwriter_add_int(uv_json_st *json, char *name, int value) {
	char v[12];
	itoa(value, v, 10);
	unsigned int len = 4 + strlen(name) + strlen(v);

	uv_err_pass(check_overflow(json, len));

	snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":%s,",
			name, v);
	return uv_err(ERR_NONE);
}

uv_errors_e uv_jsonwriter_array_add_int(uv_json_st *json, int value) {
	char v[12];
	itoa(value, v, 10);
	unsigned int len = strlen(v) + 1;
	uv_err_pass(check_overflow(json, len));

	snprintf(json->start_ptr + strlen(json->start_ptr), len, "%s,", v);

	return uv_err(ERR_NONE);
}


uv_errors_e uv_jsonwriter_add_string(uv_json_st *json, char *name, char *value) {
	unsigned int len = strlen(name) + strlen(value) + 6;
	uv_err_pass(check_overflow(json, len));

	snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":\"%s\",",
			name, value);
	return uv_err(ERR_NONE);
}

uv_errors_e uv_jsonwriter_array_add_string(uv_json_st *json, char *value) {
	unsigned int len = strlen(value) + 3;
	uv_err_pass(check_overflow(json, len));

	snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\",", value);
	return uv_err(ERR_NONE);
}


uv_errors_e uv_jsonwriter_add_bool(uv_json_st *json, char *name, bool value) {
	unsigned int len = strlen(name) + 9;
	uv_err_pass(check_overflow(json, len));

	snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":%s,",
			name, (value) ? "true" : "false");
	return uv_err(ERR_NONE);
}


uv_errors_e uv_jsonwriter_array_add_bool(uv_json_st *json, bool value) {
	unsigned int len = 6;
	uv_err_pass(check_overflow(json, len));

	snprintf(json->start_ptr + strlen(json->start_ptr), len, "%s,",
			(value) ? "true" : "false");

	return uv_err(ERR_NONE);
}


bool uv_jsonreader_get_next_sibling(char *object, char **dest) {
	unsigned int count = 0;
	char *ptr;
	for (ptr = object; *ptr != '\0'; ptr++) {
		// cycle to the end of this object
		if (*ptr == '{' || *ptr == '[') {
			count++;
		}
		else if (*ptr == '}' || *ptr == ']') {
			count--;
		}
		// comma separates objects. If count is zero, it means that the starting object
		// was finished and we are in the start of the next sibling.
		else if (*ptr == ',' && count == 0) {
			if (dest) {
				*dest = ++ptr;
			}
			return true;
		}
		// if count is negative, starting object didn't have any more siblings
		else if (count < 0) {
			break;
		}
	}

	return false;
}



bool uv_jsonreader_get_child(char *parent, unsigned int child_index, char **dest) {
	unsigned int count = 0, child_count = 0;
	char *ptr;
	// check if this object is of type object or array
	if (uv_jsonreader_get_type(parent) >= 0) {
		return false;
	}

	for (ptr = parent; *ptr != '\0'; ptr++) {
		if (*ptr == '{' || *ptr == '[') {
			count++;
		}
		else if (*ptr == '}' || *ptr == ']') {
			count--;
		}
		if (count == 1 && *(ptr + 2) == ':') {
			// child found, check if this is the index requested
			if (++child_count == child_index) {
				// step back to the start of child's name
				while (*ptr != '"') {
					ptr--;
				}
				if (dest) {
					*dest = ptr;
				}
				return true;
			}
		}
		// no more childs
		else if (count < 0) {
			break;
		}
	}
	return false;
}


bool uv_jsonreader_find_child(char *parent, char *child_name,
		int depth, char **dest) {
	unsigned int count = 0, name_len = strlen(child_name);
	char *ptr;
	// check if this object is of type object or array
	if (uv_jsonreader_get_type(parent) >= 0) {
		return false;
	}
	for (ptr = parent; *ptr != '\0'; ptr++) {
		if (*ptr == '{' || *ptr == '[') {
			count++;
		}
		else if (*ptr == '}' || *ptr == ']') {
			count--;
		}
		// no more children
		else if (count < 0) {
			break;
		}
		else if (*(ptr + 2) == ':') {
			if (depth < 0 || count <= depth) {
				while (*ptr != '"') {
					ptr--;
				}
				// child found, check if child has the name requested
				if (strncmp(ptr + 1, child_name, name_len) == 0) {
					if (dest) {
						*dest = ptr;
					}
					return true;
				}
			}
		}
	}
	return false;
}




bool uv_jsonreader_get_obj_name(char *object, char **dest, unsigned int dest_length) {
	unsigned int i = 0;
	object++;
	while (*object != '"') {
		if (object < *dest + dest_length) {
			(*dest)[i++] = *object;
		}
		else {
			return false;
		}
	}
	return true;
}


static char *get_value_ptr(char *ptr) {
	while (*ptr != ':') {
		ptr++;
	};
	ptr++;
	return ptr;
}


uv_json_types_e uv_jsonreader_get_type(char *object) {
	object = get_value_ptr(object);
	if (*object == '\0') {
		return JSON_UNSUPPORTED;
	}
	// object now points to the first character of the value
	switch (*object) {
	case '{':
		return JSON_OBJECT;
	case '[':
		return JSON_ARRAY;
	case '"':
		return JSON_STRING;
	case 't':
	case 'f':
		return JSON_BOOL;
	default:
		return JSON_INT;
	}
}


/// @brief: Returns the pointer to the array's 'inedex'th child
static char *array_index(char *array, unsigned int index) {
	// as long as the index is not zero, find the next child
	while (index) {
		while(*array != ',') {
			array++;
		}
		array++;
		index--;
	}
	return array;
}


int uv_json_get_int(char *object) {
	return atoi(get_value_ptr(object));
}


int uv_json_array_get_int(char *object, unsigned int index) {
	return atoi(array_index(object, index));
}



bool uv_jsonreader_get_string(char *object, char **dest, unsigned int dest_length) {
	object = get_value_ptr(object) + 1;
	uint16_t i;
	for (i = 0; i < dest_length; i++) {
		if (object[i] != '"') {
			(*dest)[i] = object[i];
		}
		else {
			(*dest)[i] = '\0';
			return true;
		}
	}
	// ending here means that the value didn't fit into 'dest'
	(*dest)[dest_length - 1] = '\0';
	return false;
}


bool uv_jsonreader_array_get_string(char *object, unsigned int index, char **dest, unsigned int dest_length) {
	object = array_index(object, index) + 1;
	uint16_t i;
	for (i = 0; i < dest_length; i++) {
		if (object[i] != '"') {
			(*dest)[i] = object[i];
		}
		else {
			(*dest)[i] = '\0';
			return true;
		}
	}
	// ending here means that the value didn't fit into 'dest'
	(*dest)[dest_length - 1] = '\0';
	return false;
}


bool uv_jsonreader_get_bool(char *object) {
	object = get_value_ptr(object);
	if (strncmp(object, "true", 4) == 0) {
		return true;
	}
	else {
		return false;
	}
}


bool uv_jsonreader_array_get_bool(char *object, unsigned int index) {
	object = array_index(object, index);
	if (strncmp(object, "true", 4) == 0) {
		return true;
	}
	else {
		return false;
	}
}

