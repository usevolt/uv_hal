/*
 * FreeRTOS Kernel V10.4.3 port for Emscripten (WebAssembly in a web browser)
 * Copyright (c) 2026 Usevolt Oy.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <limits.h>

/*-----------------------------------------------------------
 * A browser gives a page one thread, and a page which keeps it never draws
 * anything nor sees a single event. So the tasks are Emscripten fibers
 * (coroutines built on Asyncify): only one runs at a time, a context switch is
 * a fiber swap, and nothing is ever preempted - a task runs until it blocks,
 * delays or yields. Whenever only the idle task has anything to do, control
 * goes back to the browser, which is also where the tick comes from. See
 * port.c.
 *
 * Build with -sASYNCIFY and configUSE_PREEMPTION 0.
 *-----------------------------------------------------------*/

/* Type definitions. */
#define portCHAR		char
#define portFLOAT		float
#define portDOUBLE		double
#define portLONG		long
#define portSHORT		short
#define portSTACK_TYPE	uint32_t
#define portBASE_TYPE	long
#define portPOINTER_SIZE_TYPE uintptr_t

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

typedef uint32_t TickType_t;
#define portMAX_DELAY ( TickType_t ) UINT32_MAX

#define portTICK_TYPE_IS_ATOMIC 1

/*-----------------------------------------------------------*/

/* Architecture specifics. */
#define portSTACK_GROWTH			( -1 )
#define portTICK_PERIOD_MS			( ( TickType_t ) 1000 / configTICK_RATE_HZ )
#define portBYTE_ALIGNMENT			16
/*-----------------------------------------------------------*/

/* Scheduler utilities. */
extern void vPortYield( void );

#define portYIELD() vPortYield()

#define portEND_SWITCHING_ISR( xSwitchRequired ) if( xSwitchRequired != pdFALSE ) vPortYield()
#define portYIELD_FROM_ISR( x ) portEND_SWITCHING_ISR( x )
/*-----------------------------------------------------------*/

/* Critical section management. There is one thread and nothing interrupts it,
 * so these only keep the nesting count the kernel expects. */
extern void vPortEnterCritical( void );
extern void vPortExitCritical( void );
#define portSET_INTERRUPT_MASK_FROM_ISR()		0
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(x)	( void ) ( x )
#define portDISABLE_INTERRUPTS()
#define portENABLE_INTERRUPTS()
#define portENTER_CRITICAL()					vPortEnterCritical()
#define portEXIT_CRITICAL()						vPortExitCritical()

/*-----------------------------------------------------------*/

/* A task's fiber stacks are allocated beside the FreeRTOS stack, so they go
 * when the TCB does. */
extern void vPortCleanUpTCB( void *pxTCB );
#define portCLEAN_UP_TCB( pxTCB )	vPortCleanUpTCB( pxTCB )
/*-----------------------------------------------------------*/

#define portTASK_FUNCTION_PROTO( vFunction, pvParameters ) void vFunction( void *pvParameters )
#define portTASK_FUNCTION( vFunction, pvParameters ) void vFunction( void *pvParameters )
/*-----------------------------------------------------------*/

#define portMEMORY_BARRIER() __asm volatile( "" ::: "memory" )

#define portNOP()

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
