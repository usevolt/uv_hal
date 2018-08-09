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

/// @brief: Extern declaration of the main application. This is used
/// to access uv_data_start and uv_data_end variables at build time.
extern CONFIG_APP_ST;


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
#define M_REPEAT20_ARG(X, ARG) M_REPEAT19_ARG X(19, ARG)

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

/// @brief: Delay function. Can be used to create different delays in a discrete cyclical step function system.
/// @param delay_ms The length of the delay in ms.
/// @param step_ms The duration between cyclic calls in ms.
/// @param p Pointer to a delay counter variable. Variable's value is increased every step and
/// @pre: uv_start_delay should have been called
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
	return (*p <= 0);
}

/// @brief: Triggers the delay right away
static inline void uv_delay_trigger(uv_delay_st *p) {
	*p = 0;
}

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
	uint8_t element_size;
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
		uint16_t buffer_size, uint8_t element_size);

/// @brief: Adds a new element into the ring buffer
uv_errors_e uv_ring_buffer_push(uv_ring_buffer_st *buffer, void *element);

/// @brief: Returns the next element without removing it from the buffer.
/// Can be used to check what's coming next in the buffer
uv_errors_e uv_ring_buffer_peek(uv_ring_buffer_st *buffer, void *dest);


/// @brief: Removes the last element from the ring buffer. The popped
/// element is stored into 'dest'
uv_errors_e uv_ring_buffer_pop(uv_ring_buffer_st *buffer, void *dest);


/// @brief: Clears the ring buffer to the initial state
static inline uv_errors_e uv_ring_buffer_clear(uv_ring_buffer_st *buffer) {
	return uv_ring_buffer_init(buffer, buffer->buffer, buffer->buffer_size, buffer->element_size);
}

/// @brief: Returns true if the ring buffer was empty
static inline bool uv_ring_buffer_empty(uv_ring_buffer_st *buffer) {
	return !buffer->element_count;
}

/// @brief: Returns the current element count in the buffer
static inline uint16_t uv_ring_buffer_get_element_count(uv_ring_buffer_st *buffer) {
	return buffer->element_count;
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
static inline uint16_t uv_vector_size(uv_vector_st *this) {
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



#if CONFIG_TARGET_LPC11C14
/// @brief: Defines the interrupts sources on this hardware
typedef enum {
	/******  Cortex-M0 Processor Exceptions Numbers ***************************************************/
	  INT_NMI           = -14,    /*!< 2 Non Maskable Interrupt                           */
	  INT_HARD_FAULT                = -13,    /*!< 3 Cortex-M0 Hard Fault Interrupt                   */
	  INT_SVCALL                   = -5,     /*!< 11 Cortex-M0 SV Call Interrupt                     */
	  INT_PENDSV                   = -2,     /*!< 14 Cortex-M0 Pend SV Interrupt                     */
	  INT_SYSTICK                  = -1,     /*!< 15 Cortex-M0 System Tick Interrupt                 */

	/******  CONFIG_TARGET_LPC11C14 or LPC11xx Specific Interrupt Numbers *******************************************************/
	  INT_WAKEUP0                  = 0,        /*!< All I/O pins can be used as wakeup source.       */
	  INT_WAKEUP1                  = 1,        /*!< There are 13 pins in total for LPC11xx           */
	  INT_WAKEUP2                  = 2,
	  INT_WAKEUP3                  = 3,
	  INT_WAKEUP4                  = 4,
	  INT_WAKEUP5                  = 5,
	  INT_WAKEUP6                  = 6,
	  INT_WAKEUP7                  = 7,
	  INT_WAKEUP8                  = 8,
	  INT_WAKEUP9                  = 9,
	  INT_WAKEUP10                 = 10,
	  INT_WAKEUP11                 = 11,
	  INT_WAKEUP12                 = 12,
	  INT_CAN                      = 13,       /*!< CAN Interrupt                                    */
	  INT_SSP1                     = 14,       /*!< SSP1 Interrupt                                   */
	  INT_I2C                      = 15,       /*!< I2C Interrupt                                    */
	  INT_TIMER_16_B_0               = 16,       /*!< 16-bit Timer0 Interrupt                          */
	  INT_TIMER_16_B_1               = 17,       /*!< 16-bit Timer1 Interrupt                          */
	  INT_TIMER_32_B_0               = 18,       /*!< 32-bit Timer0 Interrupt                          */
	  INT_TIMER_32_B_1               = 19,       /*!< 32-bit Timer1 Interrupt                          */
	  INT_SSP0                     = 20,       /*!< SSP0 Interrupt                                   */
	  INT_UART                     = 21,       /*!< UART Interrupt                                   */
	  INT_ADC                      = 24,       /*!< A/D Converter Interrupt                          */
	  INT_WDT                      = 25,       /*!< Watchdog timer Interrupt                         */
	  INT_BOD                      = 26,       /*!< Brown Out Detect(BOD) Interrupt                  */
	  INT_FMC                      = 27,       /*!< Flash Memory Controller Interrupt                */
	  INT_EINT3                    = 28,       /*!< External Interrupt 3 Interrupt                   */
	  INT_EINT2                    = 29,       /*!< External Interrupt 2 Interrupt                   */
	  INT_EINT1                    = 30,       /*!< External Interrupt 1 Interrupt                   */
	  INT_EINT0                    = 31,       /*!< External Interrupt 0 Interrupt                   */
} uv_int_sources_e;

enum {
	INT_LOWEST_PRIORITY = 3
};

#elif CONFIG_TARGET_LPC1785
/// @brief: Defines the interrupts sources on this hardware
typedef enum {
	/******  Cortex-M3 Processor Exceptions Numbers ***************************************************/
	  INT_NMI           			= -14,      /*!< 2 Non Maskable Interrupt                         */
	  INT_MEMORY_MANAGEMENT         = -12,      /*!< 4 Cortex-M3 Memory Management Interrupt          */
	  INT_BUS_FAULT					= -11,      /*!< 5 Cortex-M3 Bus Fault Interrupt                  */
	  INT_USAGE_FAULT               = -10,      /*!< 6 Cortex-M3 Usage Fault Interrupt                */
	  INT_SVCALL                   	= -5,       /*!< 11 Cortex-M3 SV Call Interrupt                   */
	  INT_DEBUG_MONITOR             = -4,       /*!< 12 Cortex-M3 Debug Monitor Interrupt             */
	  INT_PENDSV                   	= -2,       /*!< 14 Cortex-M3 Pend SV Interrupt                   */
	  INT_SYSTICK                  	= -1,       /*!< 15 Cortex-M3 System Tick Interrupt               */

	/******  LPC177x_8x Specific Interrupt Numbers *******************************************************/
	  INT_WDT                     	= 0,        /*!< Watchdog Timer Interrupt                         */
	  IMT_TIMER0					= 1,        /*!< Timer0 Interrupt                                 */
	  INT_TIMER1                   	= 2,        /*!< Timer1 Interrupt                                 */
	  INT_TIMER2                   	= 3,        /*!< Timer2 Interrupt                                 */
	  INT_TIMER3                   	= 4,        /*!< Timer3 Interrupt                                 */
	  INT_UART0                    	= 5,        /*!< UART0 Interrupt                                  */
	  INT_UART1                    	= 6,        /*!< UART1 Interrupt                                  */
	  INT_UART2                    	= 7,        /*!< UART2 Interrupt                                  */
	  INT_UART3                    	= 8,        /*!< UART3 Interrupt                                  */
	  INT_PWM1                     	= 9,        /*!< PWM1 Interrupt                                   */
	  INT_I2C0                     	= 10,       /*!< I2C0 Interrupt                                   */
	  INT_I2C1                     	= 11,       /*!< I2C1 Interrupt                                   */
	  INT_I2C2                     	= 12,       /*!< I2C2 Interrupt                                   */
	  INT_SSP0                     	= 14,       /*!< SSP0 Interrupt                                   */
	  INT_SSP1                     	= 15,       /*!< SSP1 Interrupt                                   */
	  INT_PLL0                     	= 16,       /*!< PLL0 Lock (Main PLL) Interrupt                   */
	  INT_RTC                      	= 17,       /*!< Real Time Clock Interrupt                        */
	  INT_EINT0                    	= 18,       /*!< External Interrupt 0 Interrupt                   */
	  INT_EINT1                    	= 19,       /*!< External Interrupt 1 Interrupt                   */
	  INT_EINT2                    	= 20,       /*!< External Interrupt 2 Interrupt                   */
	  INT_EINT3                    	= 21,       /*!< External Interrupt 3 Interrupt                   */
	  INT_ADC                      	= 22,       /*!< A/D Converter Interrupt                          */
	  INT_BOD                      	= 23,       /*!< Brown-Out Detect Interrupt                       */
	  INT_USB                      	= 24,       /*!< USB Interrupt                                    */
	  INT_CAN                      	= 25,       /*!< CAN Interrupt                                    */
	  INT_DMA                      	= 26,       /*!< General Purpose DMA Interrupt                    */
	  INT_I2S                      	= 27,       /*!< I2S Interrupt                                    */
	  INT_ENET                     	= 28,       /*!< Ethernet Interrupt                               */
	  INT_MCI                      	= 29,       /*!< SD/MMC card I/F Interrupt                        */
	  INT_MCPWM                    	= 30,       /*!< Motor Control PWM Interrupt                      */
	  INT_QEI                      	= 31,       /*!< Quadrature Encoder Interface Interrupt           */
	  INT_PLL1                     	= 32,       /*!< PLL1 Lock (USB PLL) Interrupt                    */
	  INT_USB_ACT              	   	= 33,       /*!< USB Activity interrupt                           */
	  INT_CAN_ACT              	   	= 34,       /*!< CAN Activity interrupt                           */
	  INT_UART4                    	= 35,       /*!< UART4 Interrupt                                  */
	  INT_SSP2                     	= 36,       /*!< SSP2 Interrupt                                   */
	  INT_LCD                      	= 37,       /*!< LCD Interrupt                                    */
	  INT_GPIO                     	= 38,       /*!< GPIO Interrupt                                   */
	  INT_PWM0					   	= 39,       /*!< PWM0 Interrupt                                   */
	  INT_EEPROM                   	= 40,       /*!< EEPROM Interrupt                           */

} uv_int_sources_e;

enum {
	INT_LOWEST_PRIORITY = 31
};
#elif CONFIG_TARGET_LPC1549

#endif



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
