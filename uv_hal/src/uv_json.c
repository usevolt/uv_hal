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

#include "uv_json.h"

#if CONFIG_JSON

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdint.h>



const char *uv_json_type_to_str(uv_json_types_e type) {
	const char *ret = "UNDEFINED";
	switch(type) {
	case JSON_ARRAY:
		ret = "ARRAY";
		break;
	case JSON_BOOL:
		ret = "BOOL";
		break;
	case JSON_INT:
		ret = "INT";
		break;
	case JSON_OBJECT:
		ret = "OBJECT";
		break;
	case JSON_STRING:
		ret = "STRING";
		break;
	case JSON_UNSUPPORTED:
		ret = "UNSUPPORTED";
		break;
	default:
		break;
	}
	return ret;
}


/// @brief: Returns ERR_BUFFER_OVERFLOW if requested length overflows from JSON buffer
static uv_errors_e check_overflow(uv_json_st *json, unsigned int length_req) {
	uv_errors_e ret = ERR_NONE;
	if (strlen(json->start_ptr) + length_req >= json->buffer_length - 1) {
		ret = ERR_BUFFER_OVERFLOW;
	}

	return ret;
}


static void json_remove_whitespace(char *buffer_ptr, unsigned int buffer_len) {
	unsigned int count = 0;
	char *ptr;
	bool in_string = false;
	// remove all whitespace
	for (ptr = buffer_ptr; ptr != buffer_ptr + buffer_len; ptr++) {
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
	*(buffer_ptr + buffer_len - count) = '\0';
}


uv_errors_e uv_jsonreader_init(char *buffer_ptr, unsigned int buffer_length) {
	uv_errors_e ret = ERR_NONE;

	json_remove_whitespace(buffer_ptr, buffer_length);

	return ret;
}



uv_errors_e uv_jsonwriter_init(uv_json_st *json, char *buffer_ptr,
		unsigned int buffer_length) {
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


uv_errors_e uv_jsonwriter_begin_object(uv_json_st *json) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = 1;

	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "{");
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
	unsigned int len = strlen(name) + 5;

	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		if (strlen(name) == 0) {
			snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "[");
		}
		else {
			snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "\"%s\":[", name);
		}
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
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "\"%s\":%s,",
				name, v);
	}
	return ret;
}

uv_errors_e uv_jsonwriter_add_int_hex(uv_json_st *json, char *name, uint32_t value) {
	uv_errors_e ret = ERR_NONE;

	char v[14];
	sprintf(v, "0x%x", value);
	unsigned int len = 6 + strlen(name) + strlen(v);

	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "\"%s\":\"%s\",",
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
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "%s,", v);
	}

	return ret;
}

uv_errors_e uv_jsonwriter_array_add_int_hex(uv_json_st *json, int value) {
	uv_errors_e ret = ERR_NONE;

	char v[12];
	sprintf(v, "0x%x", value);
	unsigned int len = strlen(v) + 3;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "\"%s\",", v);
	}

	return ret;
}



uv_errors_e uv_jsonwriter_add_string(uv_json_st *json, char *name, char *value) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = strlen(name) + strlen(value) + 6;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "\"%s\":\"%s\",",
				name, value);
	}
	return ret;
}

uv_errors_e uv_jsonwriter_array_add_string(uv_json_st *json, char *value) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = strlen(value) + 3;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "\"%s\",", value);
	}
	return ret;
}


uv_errors_e uv_jsonwriter_add_bool(uv_json_st *json, char *name, bool value) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = strlen(name) + 9;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "\"%s\":%s,",
				name, (value) ? "true" : "false");
	}
	return ret;
}



uv_errors_e uv_jsonwriter_array_add_bool(uv_json_st *json, bool value) {
	uv_errors_e ret = ERR_NONE;

	unsigned int len = 6;
	ret = check_overflow(json, len);

	if (ret == ERR_NONE) {
		snprintf(json->start_ptr + strlen(json->start_ptr), len + 1, "%s,",
				(value) ? "true" : "false");
	}

	return ret;
}


bool uv_jsonwriter_append_json(uv_json_st *json, char *data) {
	bool ret = true;
	json_remove_whitespace(data, strlen(data));
	uint32_t len = strlen(data);
	uint32_t jsonlen = strlen(json->start_ptr);
	if (jsonlen + len + 2 > json->buffer_length) {
		ret = false;
	}
	else {
		snprintf(json->start_ptr + jsonlen, len + 2, "%s,", data);
		printf("%s\n", json->start_ptr + strlen(json->start_ptr) - 1);
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

// returns the next character. If a string is detected, it is jumped over
// and the next character after the ending \" is returned.
char *next(char *position) {
	if (*position == '"') {
		do {
			position++;
		}
		while (*position != '"' && *position != '\0');
	}
	position++;
	return position;
}


static char *get_value_ptr(char *ptr);


bool uv_jsonreader_get_next_sibling(char *object, char **dest) {
	bool ret = false;
	int count = 0;
	char *ptr;
	for (ptr = object; *ptr != '\0'; ptr = next(ptr)) {
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
		else {

		}
		if (br || count < 0) {
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



char *uv_jsonreader_find_child(char *parent, char *child_name) {
	char *ret = NULL;
	unsigned int name_len = strlen(child_name);
	char *ptr;

	// check if this object is of type object or array
	if (!uv_json_is_objarray(uv_jsonreader_get_type(parent))) {

		ret = false;
	}
	else {
		// jump to the start of this object
		parent = get_value_ptr(parent);
		ptr = parent;
		if (*ptr == '{') {
			ptr++;
		}
		while (*ptr != '\0') {
			bool br = false;

			// child found, check if child has the name requested
			if (strncmp(ptr + 1, child_name, name_len) == 0 &&
					*(ptr + 1 + name_len) == '"') {
				ret = ptr;
				br = true;
			}
			else {
				if (!uv_jsonreader_get_next_sibling(ptr, &ptr)) {
					br = true;
				}
			}
			if (br) {
				break;
			}
		}
	}
	return ret;
}



char *uv_jsonreader_get_child(char *parent, uint16_t index) {
	char *ret = NULL;
	char *ptr;

	// check if this object is of type object or array
	if (!uv_json_is_objarray(uv_jsonreader_get_type(parent))) {
		ret = false;
	}
	else {
		// jump to the start of this object
		parent = get_value_ptr(parent);
		ptr = parent;
		if (*ptr == '{') {
			ptr++;
		}
		uint16_t i = 0;
		while (*ptr != '\0') {
			bool br = false;

			// child found, check if child has the name requested
			if (i == index) {
				ret = ptr;
				br = true;
			}
			else {
				i++;
				if (!uv_jsonreader_get_next_sibling(ptr, &ptr)) {
					br = true;
				}
			}
			if (br) {
				break;
			}
		}
	}
	return ret;
}



bool uv_jsonreader_get_obj_name(char *object, char *dest, unsigned int dest_length) {
	bool ret = true;
	if (object) {
		object++;
		int16_t i = 0;
		while (*object != '"') {
			if (i == dest_length) {
				ret = false;
				break;
			}
			else {
				dest[i++] = *(object++);
			}
		}
		dest[i] = '\0';
	}
	else {
		return false;
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
	uv_json_types_e ret = JSON_UNSUPPORTED;
	if (object != NULL) {
		object = get_value_ptr(object);

		if (*object != '\0') {
			// object now points to the first character of the value
			switch (*object) {
			case '{':
				ret = JSON_OBJECT;
				break;
			case '[':
				ret = JSON_ARRAY;
				break;
			case '"':
			{
				char *str = object + 1;
				if (*str == '"') {
					// empty string
					ret = JSON_STRING;
				}
				else {
					// hexadecimals can be written as strings
					bool hex = false;
					if (strncmp(str, "0x", 2) == 0) {
						hex = true;
						str += 2;
					}
					while ((hex) ? isxdigit(*str) : isdigit(*str)) {
						str++;
					}
					if (*str == '"') {
						ret = JSON_INT;
					}
					else {
						ret = JSON_STRING;
					}
				}
				break;
			}
			case 't':
			case 'f':
				ret = JSON_BOOL;
				break;
			default:
				ret = JSON_INT;
				break;
			}
		}
	}
	return ret;
}


unsigned int uv_jsonreader_array_get_size(char *array) {
	unsigned int ret = 0;

	if (array != NULL) {
		// jump to the start of array
		array = jump_to(array, "[");

		// if the array was not empty, add 1
		if (*(array + 1) != ']') {
			ret++;
		}

		for (char *ptr = next(array); *ptr != '\0'; ptr = next(ptr)) {
			// check for children
			if ((*ptr == ',') && (*(ptr + 1) != ']')) {
				ret++;
			}
			else {
				// jump over objects and arrays
				if ((*ptr == '{') || (*ptr == '[')) {
					ptr = objarray_to_end(ptr);
				}
				// check for array to end
				else if ((*ptr == ']') || (*ptr == '\0')) {
					break;
				}
				else {

				}

			}
		}
	}

	return ret;
}



/// @brief: Returns the pointer to the array's 'inedex'th child
static char *array_index(char *array, unsigned int index) {
	if (array != NULL) {
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
	}
	return array;
}


int uv_jsonreader_get_int(char *object) {
	int ret = 0;
	if (object != NULL) {
		uv_json_types_e type = uv_jsonreader_get_type(object);
		// as hex values are stored as strings, we need to check the type
		if (type == JSON_INT) {
			char *c = get_value_ptr(object);
			// hexvalues are defined as strings
			if (*c == '"') {
				c++;
			}
			ret = strtol(c, NULL, 0);
		}
		else if (type == JSON_STRING) {
			ret = strtol(uv_jsonreader_get_string_ptr(object), NULL, 0);
		}
		else {

		}

	}
	return ret;
}


int uv_json_array_get_int(char *object, unsigned int index) {
	return strtol(array_index(object, index), NULL, 0);
}



bool uv_jsonreader_get_string(char *object, char *dest, unsigned int dest_length) {
	bool ret = false;
	if (object != NULL) {
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
	}
	return ret;
}


char *uv_jsonreader_get_string_ptr(char *object) {
	char *ret = NULL;
	if (object != NULL) {
		object = get_value_ptr(object);
		if (*object == '"') {
			ret = ++object;
		}
	}
	return ret;
}


uint32_t uv_jsonreader_get_string_len(char *object) {
	uint32_t ret = 0;
	if (object != NULL) {
		object = get_value_ptr(object);
		if (*object == '"') {
			object++;
			while (*(object++) != '"') {
				ret++;
			}
		}
	}
	return ret;
}



bool uv_jsonreader_array_get_string(char *object, unsigned int index,
		char *dest, unsigned int dest_length) {
	bool ret = false;

	if (object != NULL) {
		object = array_index(object, index) + 1;
		uint16_t i;
		for (i = 0; i < dest_length; i++) {
			if (object[i] != '"') {
				dest[i] = object[i];
			}
			else {
				dest[i] = '\0';
				ret = true;
				break;
			}
		}
		if (!ret) {
			// ending here means that the value didn't fit into 'dest'
			dest[dest_length - 1] = '\0';
		}
	}
	return ret;
}


bool uv_jsonreader_get_bool(char *object) {
	bool ret = false;

	if (object != NULL) {
		object = get_value_ptr(object);
		if (strncmp(object, "true", 4) == 0) {
			ret = true;
		}
	}
	return ret;
}


bool uv_jsonreader_array_get_bool(char *object, unsigned int index) {
	bool ret = false;

	if (object != NULL) {
		object = array_index(object, index);
		if (strncmp(object, "true", 4) == 0) {
			ret = true;
		}
		else {
			ret = false;
		}
	}
	return ret;
}



int uv_jsonreader_array_get_int(char *object, unsigned int index) {
	int ret = 0;

	if (object != NULL) {
		object = array_index(object, index);
		// hex values are stored as strings, thus hop over quotes
		if (*object == '"') {
			object++;
		}
		ret = strtol(object, NULL, 0);
	}

	return ret;
}



char *uv_jsonreader_array_at(char *object, unsigned int index) {
	char *ret = NULL;

	if (object != NULL) {
		if (index < uv_jsonreader_array_get_size(object)) {
			ret = array_index(object, index);
		}
	}

	return ret;
}



uv_json_types_e uv_jsonreader_array_get_type(char *array, unsigned int index) {
	uv_json_types_e ret = JSON_UNSUPPORTED;

	if (array != NULL) {
		char *obj = uv_jsonreader_array_at(array, index);
		if (obj != NULL) {
			// object now points to the first character of the value
			switch (*obj) {
			case '{':
				ret = JSON_OBJECT;
				break;
			case '[':
				ret = JSON_ARRAY;
				break;
			case '"':
				char *str = obj + 1;
				if (*str == '"') {
					// empty string
					ret = JSON_STRING;
				}
				else {
					// hexadecimals can be written as strings
					bool hex = false;
					if (strncmp(str, "0x", 2) == 0) {
						hex = true;
						str += 2;
					}
					while ((hex) ? isxdigit(*str) : isdigit(*str)) {
						str++;
					}
					if (*str == '"') {
						ret = JSON_INT;
					}
					else {
						ret = JSON_STRING;
					}
				}
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
	}

	return ret;
}


#endif
