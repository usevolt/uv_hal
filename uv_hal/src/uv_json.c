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

#include "uv_json.h"

#if CONFIG_JSON

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>


/// @brief: Returns ERR_BUFFER_OVERFLOW if requested length overflows from JSON buffer
static uv_errors_e check_overflow(uv_json_st *json, unsigned int length_req) {
	uv_errors_e ret = ERR_NONE;
	if (strlen(json->start_ptr) + length_req >= json->buffer_length - 1) {
		ret = ERR_BUFFER_OVERFLOW;
	}

	return ret;
}


uv_errors_e uv_jsonreader_init(char *buffer_ptr, unsigned int buffer_length) {
	uv_errors_e ret = ERR_NONE;

	unsigned int count = 0;
	char *ptr;
	bool in_string = false;
	// remove all whitespace
	for (ptr = buffer_ptr; ptr != buffer_ptr + buffer_length; ptr++) {
		if (*ptr == '"') {
			in_string = !in_string;
		}
		if (!in_string && isspace((int) *ptr)) {
			count++;
		}
		else {
			*(ptr - count) = *ptr;
		}
	}
	*(buffer_ptr + buffer_length - count) = '\0';

	return ret;
}



uv_errors_e uv_jsonwriter_init(uv_json_st *json, char *buffer_ptr, unsigned int buffer_length) {
	json->start_ptr = buffer_ptr;
 	json->buffer_length = buffer_length;
	sprintf(json->start_ptr, "{");
	return ERR_NONE;
}



uv_errors_e uv_jsonwriter_end(uv_json_st *json, uv_json_errors_e *errors) {
	uv_errors_e ret = ERR_NONE;
	ret = check_overflow(json, 1);

	if (ret == ERR_NONE) {
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

		if (errors) {
			*errors = JSON_ERR_NONE;
		}
		if (count) {
			if (errors) {
				*errors = JSON_ERR_UNTERMINATED_OBJ;
			}
			ret = ERR_INTERNAL | HAL_MODULE_JSON;
		}
	}

	return ret;
}


uv_errors_e uv_jsonwriter_begin_object(uv_json_st *json, char *name) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = strlen(name) + 4;

	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":{", name);
	}

	return ret;
}

uv_errors_e uv_jsonwriter_end_object(uv_json_st *json) {
	uv_errors_e ret = ERR_NONE;

	ret = check_overflow(json, 2);

	if (ret == ERR_NONE) {
		unsigned int size = strlen(json->start_ptr);
		if (size && json->start_ptr[size - 1] == ',') {
			json->start_ptr[size - 1] = '\0';
		}
		strcat(json->start_ptr, "},");
	}

	return ret;
}

uv_errors_e uv_jsonwriter_begin_array(uv_json_st *json, char *name) {
	uv_errors_e ret = ERR_NONE;
	unsigned int len = strlen(name) + 4;

	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":[", name);
	}

	return ret;
}

uv_errors_e uv_jsonwriter_end_array(uv_json_st *json) {
	uv_errors_e ret = ERR_NONE;

	ret = check_overflow(json, 2);

	if (ret == ERR_NONE) {
		unsigned int size = strlen(json->start_ptr);
		if (size && json->start_ptr[size - 1] == ',') {
			json->start_ptr[size - 1] = '\0';
		}
		strcat(json->start_ptr, "],");
	}

	return ret;
}

uv_errors_e uv_jsonwriter_add_int(uv_json_st *json, char *name, int value) {
	uv_errors_e ret = ERR_NONE;

	char v[12];
	sprintf(v, "%i", value);
	unsigned int len = 4 + strlen(name) + strlen(v);

	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":%s,",
				name, v);
	}
	return ret;
}

uv_errors_e uv_jsonwriter_array_add_int(uv_json_st *json, int value) {
	uv_errors_e ret = ERR_NONE;

	char v[12];
	sprintf(v, "%i", value);
	unsigned int len = strlen(v) + 1;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len, "%s,", v);
	}

	return ret;
}


uv_errors_e uv_jsonwriter_add_string(uv_json_st *json, char *name, char *value) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = strlen(name) + strlen(value) + 6;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":\"%s\",",
				name, value);
	}
	return ret;
}

uv_errors_e uv_jsonwriter_array_add_string(uv_json_st *json, char *value) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = strlen(value) + 3;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\",", value);
	}
	return ret;
}


uv_errors_e uv_jsonwriter_add_bool(uv_json_st *json, char *name, bool value) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = strlen(name) + 9;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len, "\"%s\":%s,",
				name, (value) ? "true" : "false");
	}
	return ret;
}



uv_errors_e uv_jsonwriter_array_add_bool(uv_json_st *json, bool value) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = 6;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len, "%s,",
				(value) ? "true" : "false");
	}

	return ret;
}


char *jump_to(char *position, char* match) {
	bool is_string = false;
	bool found = false;
	while (*(position) != '\0') {
		if (*position == '"') {
			is_string = !is_string;
		}
		if (!is_string) {
			for (int32_t i = 0; i < strlen(match); i++) {
				if ((*position) == *(match + i)) {
					found = true;
					break;
				}
			}
		}
		if (found) {
			break;
		}
		position++;
	}
	return position;
}

char *next(char *position) {
	position++;
	if (*position == '"') {
		while (*(++position) != '"' && *position != '\0');
	}
	return position;
}


bool uv_jsonreader_get_next_sibling(char *object, char **dest) {
	bool ret = false;
	unsigned int count = 0;
	char *ptr;
	for (ptr = object; *ptr != '\0'; ptr++) {
		bool br = false;
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
			ret = true;
			br = true;
		}
		// if count is negative, starting object didn't have any more siblings
		else if (count < 0) {
			br = true;
		}
		else {

		}
		if (br) {
			break;
		}
	}

	return ret;
}



static char *objarray_to_end(char *obj) {
	unsigned int count = 0;
	char *ptr;
	for (ptr = obj; *ptr != '\0'; ptr = next(ptr)) {
		if (*ptr == '{' || *ptr == '[') {
			count++;
		}
		else if (*ptr == '}' || *ptr == ']') {
			count--;
		}
		// no more children
		if (count == 0 || (*ptr == '\0')) {
			break;
		}
	}
	return ptr;
}



char *uv_jsonreader_find_child(char *parent, char *child_name,
		int depth) {
	char *ret = NULL;
	unsigned int count = 0, name_len = strlen(child_name);
	char *ptr;
	// check if this object is of type object or array
	if (!uv_json_is_objarray(uv_jsonreader_get_type(parent))) {

		ret = false;
	}
	else {
		for (ptr = parent; *ptr != '\0'; ptr = next(ptr)) {
			bool br = false;

			if (*ptr == '{' || *ptr == '[') {
				count++;
			}
			else if (*ptr == '}' || *ptr == ']') {
				count--;
			}
			// no more children
			else if (count < 0) {
				br = true;
			}
			else if (*(ptr + 1) == ':') {
				if (depth < 0 || count <= depth) {
					while (*(--ptr) != '"');

					// child found, check if child has the name requested
					if (strncmp(ptr + 1, child_name, name_len) == 0 &&
							*(ptr + 1 + name_len) == '"') {
						ret = ptr;
						br = true;
					}
					else {
						ptr = jump_to(ptr, ",");
					}
				}
			}
			else {

			}
			if (br) {
				break;
			}
		}
	}
	return ret;
}




bool uv_jsonreader_get_obj_name(char *object, char **dest, unsigned int dest_length) {
	bool ret = true;
	unsigned int i = 0;
	object++;
	while (*object != '"') {
		if (object < *dest + dest_length) {
			(*dest)[i++] = *object;
		}
		else {
			ret = false;
			break;
		}
	}
	return ret;
}


static char *get_value_ptr(char *ptr) {
	while (*ptr != ':') {
		if (*ptr == '{') {
			ptr--;
			break;
		}
		else {
			ptr++;
		}
	};
	ptr++;
	return ptr;
}


uv_json_types_e uv_jsonreader_get_type(char *object) {
	uv_json_types_e ret = JSON_INT;
	object = get_value_ptr(object);

	if (*object == '\0') {
		ret = JSON_UNSUPPORTED;
	}
	else {
		// object now points to the first character of the value
		switch (*object) {
		case '{':
			ret = JSON_OBJECT;
			break;
		case '[':
			ret = JSON_ARRAY;
			break;
		case '"':
			ret = JSON_STRING;
			break;
		case 't':
		case 'f':
			ret = JSON_BOOL;
			break;
		default:
			ret = JSON_INT;
			break;
		}
	}
	return ret;
}


unsigned int uv_jsonreader_array_get_size(char *array) {
	unsigned int ret = 0;

	// jump to the start of array
	array = jump_to(array, "[");
	array++;

	// specially check first child
	if (*array != ']') {
		ret++;
	}

	for (char *ptr = array; *ptr != '\0'; ptr = next(ptr)) {

		// check for children
		if ((*ptr == ',') && (*(ptr + 1) != ']')) {
			ret++;
		}


		// jump over objects and arrays
		if ((*ptr == '{') || (*ptr == '[')) {
			ptr = objarray_to_end(ptr);
		}
		// jump over strings
		else if (*ptr == '"') {
			while ((*ptr != '\0') && (*ptr != '"')) {
				ptr++;
			}
		}
		else {

		}

		// check for array to end
		if ((*ptr == ']') || (*ptr == '\0')) {
			break;
		}
	}

	return ret;
}

/// @brief: Returns the pointer to the array's 'inedex'th child
static char *array_index(char *array, unsigned int index) {
	// jump to the start of array
	array = jump_to(array, "[");
	array++;

	// as long as the index is not zero, find the next child
	while (index) {
		array = jump_to(array, "{[,");
		if (*array == '{' || *array == '[') {
			array = objarray_to_end(array);
		}
		else {
			array++;
			index--;
		}
	}
	return array;
}


int uv_jsonreader_get_int(char *object) {
	return strtol(get_value_ptr(object), NULL, 0);
}


int uv_json_array_get_int(char *object, unsigned int index) {
	return strtol(array_index(object, index), NULL, 0);
}



bool uv_jsonreader_get_string(char *object, char *dest, unsigned int dest_length) {
	bool ret = false;

	object = get_value_ptr(object) + 1;
	uint16_t i;
	for (i = 0; i < dest_length; i++) {
		if (object[i] != '"') {
			(dest)[i] = object[i];
		}
		else {
			(dest)[i] = '\0';
			ret = true;
			break;
		}
	}
	if (!ret) {
		// ending here means that the value didn't fit into 'dest'
		(dest)[dest_length - 1] = '\0';
	}
	return ret;
}


bool uv_jsonreader_array_get_string(char *object, unsigned int index,
		char **dest, unsigned int dest_length) {
	bool ret = false;

	object = array_index(object, index) + 1;
	uint16_t i;
	for (i = 0; i < dest_length; i++) {
		if (object[i] != '"') {
			(*dest)[i] = object[i];
		}
		else {
			(*dest)[i] = '\0';
			ret = true;
			break;
		}
	}
	if (!ret) {
		// ending here means that the value didn't fit into 'dest'
		(*dest)[dest_length - 1] = '\0';
	}
	return ret;
}


bool uv_jsonreader_get_bool(char *object) {
	bool ret;

	object = get_value_ptr(object);
	if (strncmp(object, "true", 4) == 0) {
		ret = true;
	}
	else {
		ret = false;
	}
	return ret;
}


bool uv_jsonreader_array_get_bool(char *object, unsigned int index) {
	bool ret;

	object = array_index(object, index);
	if (strncmp(object, "true", 4) == 0) {
		ret = true;
	}
	else {
		ret = false;
	}
	return ret;
}



char *uv_jsonreader_array_at(char *object, unsigned int index) {
	char *ret = NULL;

	if (index < uv_jsonreader_array_get_size(object)) {
		ret = array_index(object, index);
	}
	else {
		ret = NULL;
	}

	return ret;
}

#endif
