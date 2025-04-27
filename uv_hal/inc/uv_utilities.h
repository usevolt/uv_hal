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


#ifndef UW_UTILITIES_H_
#define UW_UTILITIES_H_


#include "uv_hal_config.h"

#define CAT(a, ...) 			PRIMITIVE_CAT(a, __VA_ARGS__)
#define PRIMITIVE_CAT(a, ...) 	a ## __VA_ARGS__


/// @brief: Meant for internal use by REPEATx macros. User can use these if some problems occur
/// with nested calls to CAT macro.
#define CAT2(a, ...) 			PRIMITIVE_CAT2(a, __VA_ARGS__)
#define PRIMITIVE_CAT2(a, ...) 	a ## __VA_ARGS__
#define CAT3(a, ...) 			PRIMITIVE_CAT3(a, __VA_ARGS__)
#define PRIMITIVE_CAT3(a, ...) 	a ## __VA_ARGS__
#define CAT4(a, ...) 			PRIMITIVE_CAT4(a, __VA_ARGS__)
#define PRIMITIVE_CAT4(a, ...) 	a ## __VA_ARGS__
#define CAT5(a, ...) 			PRIMITIVE_CAT5(a, __VA_ARGS__)
#define PRIMITIVE_CAT5(a, ...) 	a ## __VA_ARGS__


#define _STRINGIFY(x)		#x
#define STRINGIFY(x)		_STRINGIFY(x)


/// @brief: A macro for compile time assertion. An error is
/// generated if the given expression evaluates as false
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ASSERT(e) enum { ASSERT_CONCAT(assert_line_, __LINE__) = 1/(!!(e)) }

/// @brief: Extern declaration of the main application. This is used
/// to access uv_data_start and uv_data_end variables at build time.
extern CONFIG_APP_ST;


#if !defined(MAX)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif
#if !defined(MIN)
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif


/// @brief: Program version should be defined in compile time
#if !defined(__UV_PROGRAM_VERSION)
#define __UV_PROGRAM_VERSION	0
#endif


#include "uv_can.h"
#include "uv_errors.h"
#if CONFIG_TARGET_LPC11C14
#include "LPC11xx.h"
#endif
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

/// @brief: Preprocessor CAT macro. Concatenates two arguments into one.
/// @example: CAT(A, 5)
///		      -> A5



#define M_REPEAT0_(X)
#define M_REPEAT1_(X) X(0)
#define M_REPEAT2_(X) M_REPEAT1_(X) X(1)
#define M_REPEAT3_(X) M_REPEAT2_(X) X(2)
#define M_REPEAT4_(X) M_REPEAT3_(X) X(3)
#define M_REPEAT5_(X) M_REPEAT4_(X) X(4)
#define M_REPEAT6_(X) M_REPEAT5_(X) X(5)
#define M_REPEAT7_(X) M_REPEAT6_(X) X(6)
#define M_REPEAT8_(X) M_REPEAT7_(X) X(7)
#define M_REPEAT9_(X) M_REPEAT8_(X) X(8)
#define M_REPEAT10_(X) M_REPEAT9_(X) X(9)
#define M_REPEAT11_(X) M_REPEAT10_(X) X(10)
#define M_REPEAT12_(X) M_REPEAT11_(X) X(11)
#define M_REPEAT13_(X) M_REPEAT12_(X) X(12)
#define M_REPEAT14_(X) M_REPEAT13_(X) X(13)
#define M_REPEAT15_(X) M_REPEAT14_(X) X(14)
#define M_REPEAT16_(X) M_REPEAT15_(X) X(15)
#define M_REPEAT17_(X) M_REPEAT16_(X) X(16)
#define M_REPEAT18_(X) M_REPEAT17_(X) X(17)
#define M_REPEAT19_(X) M_REPEAT18_(X) X(18)
#define M_REPEAT20_(X) M_REPEAT19_(X) X(19)
#define M_REPEAT21_(X) M_REPEAT20_(X) X(20)
#define M_REPEAT22_(X) M_REPEAT21_(X) X(21)
#define M_REPEAT23_(X) M_REPEAT22_(X) X(22)
#define M_REPEAT24_(X) M_REPEAT23_(X) X(23)
#define M_REPEAT25_(X) M_REPEAT24_(X) X(24)
#define M_REPEAT26_(X) M_REPEAT25_(X) X(25)
#define M_REPEAT27_(X) M_REPEAT26_(X) X(26)
#define M_REPEAT28_(X) M_REPEAT27_(X) X(27)
#define M_REPEAT29_(X) M_REPEAT28_(X) X(28)
#define M_REPEAT30_(X) M_REPEAT29_(X) X(29)
#define M_REPEAT31_(X) M_REPEAT30_(X) X(30)
#define M_REPEAT32_(X) M_REPEAT31_(X) X(31)
#define M_REPEAT33_(X) M_REPEAT32_(X) X(32)
#define M_REPEAT34_(X) M_REPEAT33_(X) X(33)
#define M_REPEAT35_(X) M_REPEAT34_(X) X(34)
#define M_REPEAT36_(X) M_REPEAT35_(X) X(35)
#define M_REPEAT37_(X) M_REPEAT36_(X) X(36)
#define M_REPEAT38_(X) M_REPEAT37_(X) X(37)
#define M_REPEAT39_(X) M_REPEAT38_(X) X(38)
#define M_REPEAT40_(X) M_REPEAT39_(X) X(39)
#define M_REPEAT41_(X) M_REPEAT40_(X) X(40)
#define M_REPEAT42_(X) M_REPEAT41_(X) X(41)
#define M_REPEAT43_(X) M_REPEAT42_(X) X(42)
#define M_REPEAT44_(X) M_REPEAT43_(X) X(43)
#define M_REPEAT45_(X) M_REPEAT44_(X) X(44)
#define M_REPEAT46_(X) M_REPEAT45_(X) X(45)
#define M_REPEAT47_(X) M_REPEAT46_(X) X(46)
#define M_REPEAT48_(X) M_REPEAT47_(X) X(47)
#define M_REPEAT49_(X) M_REPEAT48_(X) X(48)
#define M_REPEAT50_(X) M_REPEAT49_(X) X(49)
#define M_REPEAT51_(X) M_REPEAT50_(X) X(50)
#define M_REPEAT52_(X) M_REPEAT51_(X) X(51)
#define M_REPEAT53_(X) M_REPEAT52_(X) X(52)
#define M_REPEAT54_(X) M_REPEAT53_(X) X(53)
#define M_REPEAT55_(X) M_REPEAT54_(X) X(54)
#define M_REPEAT56_(X) M_REPEAT55_(X) X(55)
#define M_REPEAT57_(X) M_REPEAT56_(X) X(56)
#define M_REPEAT58_(X) M_REPEAT57_(X) X(57)
#define M_REPEAT59_(X) M_REPEAT58_(X) X(58)
#define M_REPEAT60_(X) M_REPEAT59_(X) X(59)

/// @brief: Repeat macro. Can be used to repeat a function call multiple times.
///
/// @note: Function should take the repeat count as an parameter. If more parameters
/// are needed, the user should define one more macro which takes care of the actual
/// function call and give that macro as a parameter to REPEAT macro. When nested
/// with itself, CAT-macros don't expand the right way. When writing nested macros
/// with REPEAT, user should use according REPEATx with different REPEAT-nesting levels
/// to ensure the right expandation of CAT-macros.
#define REPEAT(count, func)		CAT2(M_REPEAT, CAT2(count, _(func)))
#define REPEAT2(count, func)	CAT3(M_REPEAT, CAT3(count, _(func)))
#define REPEAT3(count, func)	CAT4(M_REPEAT, CAT4(count, _(func)))
#define REPEAT4(count, func)	CAT5(M_REPEAT, CAT5(count, _(func)))


#define M_REPEAT0_ARG(X, ARG)
#define M_REPEAT1_ARG(X, ARG) X(0, ARG)
#define M_REPEAT2_ARG(X, ARG) M_REPEAT1_ARG(X, ARG) X(1, ARG)
#define M_REPEAT3_ARG(X, ARG) M_REPEAT2_ARG(X, ARG) X(2, ARG)
#define M_REPEAT4_ARG(X, ARG) M_REPEAT3_ARG(X, ARG) X(3, ARG)
#define M_REPEAT5_ARG(X, ARG) M_REPEAT4_ARG(X, ARG) X(4, ARG)
#define M_REPEAT6_ARG(X, ARG) M_REPEAT5_ARG(X, ARG) X(5, ARG)
#define M_REPEAT7_ARG(X, ARG) M_REPEAT6_ARG(X, ARG) X(6, ARG)
#define M_REPEAT8_ARG(X, ARG) M_REPEAT7_ARG(X, ARG) X(7, ARG)
#define M_REPEAT9_ARG(X, ARG) M_REPEAT8_ARG(X, ARG) X(8, ARG)
#define M_REPEAT10_ARG(X, ARG) M_REPEAT9_ARG(X, ARG) X(9, ARG)
#define M_REPEAT11_ARG(X, ARG) M_REPEAT10_ARG(X, ARG) X(10, ARG)
#define M_REPEAT12_ARG(X, ARG) M_REPEAT11_ARG(X, ARG) X(11, ARG)
#define M_REPEAT13_ARG(X, ARG) M_REPEAT12_ARG(X, ARG) X(12, ARG)
#define M_REPEAT14_ARG(X, ARG) M_REPEAT13_ARG(X, ARG) X(13, ARG)
#define M_REPEAT15_ARG(X, ARG) M_REPEAT14_ARG(X, ARG) X(14, ARG)
#define M_REPEAT16_ARG(X, ARG) M_REPEAT15_ARG(X, ARG) X(15, ARG)
#define M_REPEAT17_ARG(X, ARG) M_REPEAT16_ARG(X, ARG) X(16, ARG)
#define M_REPEAT18_ARG(X, ARG) M_REPEAT17_ARG(X, ARG) X(17, ARG)
#define M_REPEAT19_ARG(X, ARG) M_REPEAT18_ARG(X, ARG) X(18, ARG)
#define M_REPEAT20_ARG(X, ARG) M_REPEAT19_ARG(X, ARG) X(19, ARG)
#define M_REPEAT21_ARG(X, ARG) M_REPEAT20_ARG(X, ARG) X(20, ARG)
#define M_REPEAT22_ARG(X, ARG) M_REPEAT21_ARG(X, ARG) X(21, ARG)
#define M_REPEAT23_ARG(X, ARG) M_REPEAT22_ARG(X, ARG) X(22, ARG)
#define M_REPEAT24_ARG(X, ARG) M_REPEAT23_ARG(X, ARG) X(23, ARG)
#define M_REPEAT25_ARG(X, ARG) M_REPEAT24_ARG(X, ARG) X(24, ARG)
#define M_REPEAT26_ARG(X, ARG) M_REPEAT25_ARG(X, ARG) X(25, ARG)
#define M_REPEAT27_ARG(X, ARG) M_REPEAT26_ARG(X, ARG) X(26, ARG)
#define M_REPEAT28_ARG(X, ARG) M_REPEAT27_ARG(X, ARG) X(27, ARG)
#define M_REPEAT29_ARG(X, ARG) M_REPEAT28_ARG(X, ARG) X(28, ARG)
#define M_REPEAT30_ARG(X, ARG) M_REPEAT29_ARG(X, ARG) X(29, ARG)
#define M_REPEAT31_ARG(X, ARG) M_REPEAT30_ARG(X, ARG) X(30, ARG)
#define M_REPEAT32_ARG(X, ARG) M_REPEAT31_ARG(X, ARG) X(31, ARG)
#define M_REPEAT33_ARG(X, ARG) M_REPEAT32_ARG(X, ARG) X(32, ARG)
#define M_REPEAT34_ARG(X, ARG) M_REPEAT33_ARG(X, ARG) X(33, ARG)
#define M_REPEAT35_ARG(X, ARG) M_REPEAT34_ARG(X, ARG) X(34, ARG)
#define M_REPEAT36_ARG(X, ARG) M_REPEAT35_ARG(X, ARG) X(35, ARG)
#define M_REPEAT37_ARG(X, ARG) M_REPEAT36_ARG(X, ARG) X(36, ARG)
#define M_REPEAT38_ARG(X, ARG) M_REPEAT37_ARG(X, ARG) X(37, ARG)
#define M_REPEAT39_ARG(X, ARG) M_REPEAT38_ARG(X, ARG) X(38, ARG)
#define M_REPEAT40_ARG(X, ARG) M_REPEAT39_ARG(X, ARG) X(39, ARG)
#define M_REPEAT41_ARG(X, ARG) M_REPEAT40_ARG(X, ARG) X(40, ARG)
#define M_REPEAT42_ARG(X, ARG) M_REPEAT41_ARG(X, ARG) X(41, ARG)
#define M_REPEAT43_ARG(X, ARG) M_REPEAT42_ARG(X, ARG) X(42, ARG)
#define M_REPEAT44_ARG(X, ARG) M_REPEAT43_ARG(X, ARG) X(43, ARG)
#define M_REPEAT45_ARG(X, ARG) M_REPEAT44_ARG(X, ARG) X(44, ARG)
#define M_REPEAT46_ARG(X, ARG) M_REPEAT45_ARG(X, ARG) X(45, ARG)
#define M_REPEAT47_ARG(X, ARG) M_REPEAT46_ARG(X, ARG) X(46, ARG)
#define M_REPEAT48_ARG(X, ARG) M_REPEAT47_ARG(X, ARG) X(47, ARG)
#define M_REPEAT49_ARG(X, ARG) M_REPEAT48_ARG(X, ARG) X(48, ARG)
#define M_REPEAT50_ARG(X, ARG) M_REPEAT49_ARG(X, ARG) X(49, ARG)
#define M_REPEAT51_ARG(X, ARG) M_REPEAT50_ARG(X, ARG) X(50, ARG)
#define M_REPEAT52_ARG(X, ARG) M_REPEAT51_ARG(X, ARG) X(51, ARG)
#define M_REPEAT53_ARG(X, ARG) M_REPEAT52_ARG(X, ARG) X(52, ARG)
#define M_REPEAT54_ARG(X, ARG) M_REPEAT53_ARG(X, ARG) X(53, ARG)
#define M_REPEAT55_ARG(X, ARG) M_REPEAT54_ARG(X, ARG) X(54, ARG)
#define M_REPEAT56_ARG(X, ARG) M_REPEAT55_ARG(X, ARG) X(55, ARG)
#define M_REPEAT57_ARG(X, ARG) M_REPEAT56_ARG(X, ARG) X(56, ARG)
#define M_REPEAT58_ARG(X, ARG) M_REPEAT57_ARG(X, ARG) X(57, ARG)
#define M_REPEAT59_ARG(X, ARG) M_REPEAT58_ARG(X, ARG) X(59, ARG)
#define M_REPEAT60_ARG(X, ARG) M_REPEAT59_ARG X(60, ARG)


#define VILLE	varpelaide




/// @brief: Repeat macro with an argument. The argument is given to the
/// repeated function. The function should take two arguments, first the index and next the argument.
#define REPEAT_ARG(count, func, arg)		CAT2(M_REPEAT, CAT2(count, _ARG(func, arg)))
#define REPEAT_ARG2(count, func, arg)		CAT3(M_REPEAT, CAT3(count, _ARG(func, arg)))
#define REPEAT_ARG3(count, func, arg)		CAT4(M_REPEAT, CAT3(count, _ARG(func, arg)))
#define REPEAT_ARG4(count, func, arg)		CAT5(M_REPEAT, CAT3(count, _ARG(func, arg)))


#define INC(x) PRIMITIVE_CAT(INC_, x)
#define INC_0 1
#define INC_1 2
#define INC_2 3
#define INC_3 4
#define INC_4 5
#define INC_5 6
#define INC_6 7
#define INC_7 8
#define INC_8 9
#define INC_9 10
#define INC_10 11
#define INC_11 12
#define INC_12 13
#define INC_13 14
#define INC_14 15
#define INC_15 16
#define INC_16 17
#define INC_17 18
#define INC_18 19
#define INC_19 20
#define INC_20 20

#define DEC(x) PRIMITIVE_CAT(DEC_, x)
#define DEC_0 0
#define DEC_1 0
#define DEC_2 1
#define DEC_3 2
#define DEC_4 3
#define DEC_5 4
#define DEC_6 5
#define DEC_7 6
#define DEC_8 7
#define DEC_9 8
#define DEC_10 9
#define DEC_11 10
#define DEC_12 11
#define DEC_13 12
#define DEC_14 13
#define DEC_15 14
#define DEC_16 15
#define DEC_17 16
#define DEC_18 17
#define DEC_19 18
#define DEC_20 19






/// @brief: returns the masked value of 'value'
/// @example: GET_MASKED(0b111000, 0b100001);	// returns 0b100000;
#define GET_MASKED(value, mask)		(value & mask)




/// @brief: Macro which can be used to inherit objects from this class.
/// HAS TO BE DECLARED IN THE BEGINNING OF THE STRUCT, BEFORE ANY MEMBER VARIABLES.
#define EXTENDS(x)	x super


/// @brief: Checks *x* towards the limits *min* and *max* and clamps the output to them.
#define LIMITS(x, min, max) do { if (x < (min)) { x = (min); } \
	else if (x > (max)) { x = (max); } else { } } while (0)

/// @brief: Checks that *x* cannot be greater than *max*
#define LIMIT_MAX(x, __max) do { if (x > (__max)) { x = (__max); } } while (0)

/// @brief: Checks that *x* cannot be smaller than *min*
#define LIMIT_MIN(x, __min) do { if (x < (__min)) { x = (__min); } } while (0)

typedef int32_t uv_delay_st;

/// @brief: Initializes a delay.
/// @param delay_ms The desired length of the delay
/// @param p A pointer to variable which will hold the current delay count
static inline void uv_delay_init(uv_delay_st* p, uint16_t delay_ms) {
	*p = delay_ms;
}

/// @brief: Ends the delay
static inline void uv_delay_end(uv_delay_st *p) {
	*p = -1;
}


static inline int32_t uv_delay_get_time(uv_delay_st *p) {
	return (int32_t) *p;
}

/// @brief: Delay function. Can be used to create different delays in a discrete cyclical step function system.
/// @param delay_ms The length of the delay in ms.
/// @param step_ms The duration between cyclic calls in ms.
/// @param p Pointer to a delay counter variable. Variable's value is increased every step and
/// @pre: uv_delay_init should have been called
/// when it exceeds delay_ms, true is returned.
/// @example:
/// 	static int p;
/// 	uv_delay_init(1500, &p);
/// 	while (true) {
///			if (uv_delay(1, &p)) {
///				...
///			}
/// 	}
bool uv_delay(uv_delay_st* p, uint16_t step_ms);

/// @brief: returns true if the delay has ended
static inline bool uv_delay_has_ended(uv_delay_st* p) {
	return (*p < 0);
}


/// @brief: returns true for 1 step cycle when the delay triggers the end
static inline bool uv_delay_triggered(uv_delay_st *p) {
	return (*p == 0);
}

/// @brief: Triggers the delay right away
static inline void uv_delay_trigger(uv_delay_st *p) {
	*p = 0;
}


bool uv_isdigit(char c);

/// @brief: Set's the user's application pointer.
/// User can set a pointer to any variable which will be passed to all this library's
/// callback functions. This makes it easier to write object oriented code, since
/// the callback function has the object's instace as a parameter.
void uv_set_application_ptr(void *ptr);



/// @brief: Returns this controller's name as a null-terminated string
char *uv_get_hardware_name();

/// @brief: A simple ring buffer
typedef struct {
	/// @brief: Buffer for holding the data
	char *buffer;
	/// @brief: The max size of the buffer in element count
	uint16_t buffer_size;
	/// @brief: Keeps the track of how many elements there are in the buffer
	uint16_t element_count;
	/// @brief: The element size in bytes
	uint16_t element_size;
	/// @brief: Pointer to the head of the data
	char *head;
	/// @brief: Pointer to the tail of the data
	char *tail;
} uv_ring_buffer_st;

/// @brief: Initializes the ring buffer
/// @param buffer_ptr: Pointer to the ring buffer structure
/// @param buffer: Pointer to the memory location where the raw data is to be stored
/// @param buffer_size: The size of the buffer in elements
/// @param element_size: The size of the individual element in bytes
uv_errors_e uv_ring_buffer_init(uv_ring_buffer_st *buffer_ptr, void *buffer,
		uint16_t buffer_size, uint16_t element_size);

/// @brief: Adds a new element into the ring buffer. If the buffer was full
/// returns an error and new value is not pushed
uv_errors_e uv_ring_buffer_push(uv_ring_buffer_st *buffer, void *element);

/// @brief: Adds a new element into the ring buffer. If the buffer was full,
/// the last value is discarded and new is pushed in any case.
void uv_ring_buffer_push_force(uv_ring_buffer_st *buffer, void *element);

/// @brief: Returns the next element without removing it from the buffer.
/// Can be used to check what's coming next in the buffer
uv_errors_e uv_ring_buffer_peek(uv_ring_buffer_st *buffer, void *dest);


/// @brief: Removes the last element from the ring buffer. The popped
/// element is stored into 'dest'
uv_errors_e uv_ring_buffer_pop(uv_ring_buffer_st *buffer, void *dest);

static inline uv_errors_e uv_ring_buffer_pop_back(uv_ring_buffer_st *buffer, void *dest) {
	return uv_ring_buffer_pop(buffer, dest);
}

/// @brief: Removes the first element from the ring buffer, i.e. the one pushed last
uv_errors_e uv_ring_buffer_pop_front(uv_ring_buffer_st *buffer, void *dest);


/// @brief: Clears the ring buffer to the initial state
static inline uv_errors_e uv_ring_buffer_clear(uv_ring_buffer_st *buffer) {
	return uv_ring_buffer_init(buffer, buffer->buffer, buffer->buffer_size, buffer->element_size);
}

/// @brief: Returns true if the ring buffer was empty
static inline bool uv_ring_buffer_empty(uv_ring_buffer_st *buffer) {
	return !buffer->element_count;
}

static inline bool uv_ring_buffer_is_full(uv_ring_buffer_st *buffer) {
	return buffer->element_count == buffer->buffer_size;
}

/// @brief: Returns the current element count in the buffer
static inline uint16_t uv_ring_buffer_get_element_count(uv_ring_buffer_st *buffer) {
	return buffer->element_count;
}

static inline uint16_t uv_ring_buffer_get_element_max_count(uv_ring_buffer_st *buffer) {
	return buffer->buffer_size;
}



/// @brief: Simple vector data structure.
/// Pushing or popping data to the back of vector is fast, from front is slow.
typedef struct {
	uint8_t *buffer;
	uint16_t buffer_size;
	uint16_t len;
	uint16_t element_size;
} uv_vector_st;

#define UV_VECTOR_INIT(buf, bf_size, el_size)	{.len = 0,\
	.element_size = el_size, \
	.buffer_size = bf_size, \
	.buffer = buf }

/// @brief: Initializes a vector
///
/// @param buffer: Pointer to the data buffer
/// @param buffer_size: The size of the buffer in elements
/// @param element_size: The size of the element in bytes
void uv_vector_init(uv_vector_st *this, void *buffer,
		uint16_t buffer_size, uint16_t element_size);


/// @brief: Adds a new element to the back of the vector. This is a relatively
/// fast operation.
///
/// @param dest: Pointer to where form the data is copied
uv_errors_e uv_vector_push_back(uv_vector_st *this, void *src);


/// @brief: Adds a new element to the front of the vector. This is a relatively
/// slow operation since all other elements have to be moved.
///
/// @param dest: Pointer to where form the data is copied
uv_errors_e uv_vector_push_front(uv_vector_st *this, void *src);


/// @brief: inserts data to any index in the vector
uv_errors_e uv_vector_insert(uv_vector_st *this, uint16_t index, void *src);


/// @brief: Removes the last element from the vector. This is a relatively
/// fast operation.
///
/// @param dest: Pointer to the destination address where the data is copied
uv_errors_e uv_vector_pop_back(uv_vector_st *this, void *dest);


/// @brief: Removes the first element from the vector. This is a slow operation
/// since all the other elements have to be moved.
///
/// @param dest: Pointer to the destination address where the data is copied
uv_errors_e uv_vector_pop_front(uv_vector_st *this, void *dest);


/// @brief: Erases a *count* many elements from any index of the vector
uv_errors_e uv_vector_remove(uv_vector_st *this, uint16_t index, uint16_t count);


/// @brief: Returns the pointer to the *index*'th element of the vector
///
/// @note: Doesn't check for buffer overflows!
static inline void *uv_vector_at(uv_vector_st *this, int16_t index) {
	return &this->buffer[index * this->element_size];
}

/// @brief: Returns the current length of the vector, e.g. the element count
static inline int16_t uv_vector_size(uv_vector_st *this) {
	return this->len;
}

/// @brief: Returns the maximum size of the vector
static inline uint16_t uv_vector_max_size(uv_vector_st *this) {
	return this->buffer_size;
}

/// @brief: Empties the whole vector
static inline void uv_vector_clear(uv_vector_st *this) {
	this->len = 0;
}


/// @brief: Linear interpolation for floating points.
///
/// @param t: "Lerping scale". Should be between 0.0f ... 1.0f
/// @param a: The value at t=0.0f
/// @param b: The value at t=1.0f
float uv_lerpf(float t, float a, float b);

/// @brief: Linear interpolation for integers.
///
/// @param t: "Lerping scale". Should be between 0 ... 1000
/// @param a: The value at t=0
/// @param b: The value at t=1000
int uv_lerpi(int t, int a, int b);


/// @brief: Returns the relative value of t in relation to a and b.
/// Typical use case: a = min value, b = max value, t = value between. Returns the relative
/// value of t.
///
/// @note: Should be min <= t <= max and min != max
float uv_relf(float t, float min, float max);


/// @brief: Returns the relative value (parts per thousand) of t in relation to a and b.
/// Typical use case: a = min value, b = max value, t = value between. Returns the relative
/// value of t.
///
/// @note: Should be min <= t <= max and min != max
int32_t uv_reli(int32_t t, int32_t min, int32_t max);

/// @brief: Returns the bigger argument
int32_t uv_maxi(int32_t a, int32_t b);

/// @brief: Returns the smaller argument
int32_t uv_mini(int32_t a, int32_t b);


/// @brief: Calculates and returns the trailing zeroes in *a*.
/// Useful for "reversing" bit shifting, i.e. returning *x* from **var = (1 << x)**
uint32_t uv_ctz(uint32_t a);


/// @brief: Calculates the integer square root from *value*.
/// The return value is rounded.
///
/// @note: This algorithm is based on an example given in
/// https://stackoverflow.com/questions/1100090/looking-
/// for-an-efficient-integer-square-root-algorithm-for-arm-thumb2
uint64_t uv_isqrt(uint64_t value);


/// @brief: Calculates the number of *bit* bits in *a*.
/// If *bit* is true, calculates the count of 1's.
/// Otherwise calculates the count of zeroes.
uint32_t uv_countofbit(uint32_t a, uint8_t bit);


/// @brief: Returns true if *x* is in power of two
static inline bool uv_ipot(uint32_t x) {
    return (x & (x - 1)) == 0;
}

/// @bref: Set's the interrupt sources priority. If the priority is not available on
/// the hardware, an error is returned and logged to stdout.
#define uv_set_int_priority(src, priority) NVIC_SetPriority(src, priority)



/**** PROTECTED FUNCTIONS ******/
/* These are meant only for HAL library's internal usage */

/// @brief: Get's the user's application pointer
/// User can set a pointer to any variable which will be passed to all this library's
/// callback functions. This makes it easier to write object oriented code.
void *__uv_get_user_ptr();


#endif /* UW_UTILITIES_H_ */
