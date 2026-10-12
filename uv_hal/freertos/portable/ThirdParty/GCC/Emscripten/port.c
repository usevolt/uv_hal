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

/*
 * How it runs
 * -----------
 * Every task is an Emscripten fiber with a C stack and an Asyncify stack of its
 * own, both allocated here. The kernel's own stack array only holds the fiber's
 * bookkeeping (at its top, the way the Posix port keeps its thread there), so
 * the kernel's stack checks never see the stack the task actually runs on.
 *
 * A context switch is vTaskSwitchContext() followed by a fiber swap. There is no
 * preemption: a task runs until it blocks, delays or yields, which is what
 * every uv_hal task does in its loop anyway.
 *
 * The browser is a fiber too: the context xPortStartScheduler() was called in.
 * Whenever the kernel would switch to the idle task - nothing else is ready -
 * control goes back there instead. That loop hands the thread to the browser
 * for a moment (emscripten_sleep(), which is where the page draws and its
 * events, WebSocket messages and fetches are delivered), feeds the tick with
 * the milliseconds that passed meanwhile, and then switches to whatever task
 * is due, running the idle task once each round so that deleted tasks are
 * freed.
 *
 * A task which never blocks keeps the browser from ever running: the page
 * freezes. On a desktop the same task would starve the lower priorities.
 */

#include "FreeRTOS.h"
#include "task.h"

#if defined(__EMSCRIPTEN__)

#include <emscripten.h>
#include <emscripten/fiber.h>
#include <stdlib.h>
#include <stdio.h>


/* Sizes of each task's own stacks. Wasm keeps most locals in its own registers,
 * so the C stack only holds what has its address taken and the larger structs.
 * The Asyncify stack holds the task's call stack while it is switched out, so
 * it has to fit the deepest call chain a task yields from. */
#define portFIBER_C_STACK_SIZE			( 256U * 1024U )
#define portFIBER_ASYNCIFY_STACK_SIZE	( 128U * 1024U )

/* Alignment of the stacks. The sizes above are multiples of it. */
#define portFIBER_STACK_ALIGN			16U

/* How long the browser keeps the thread each round, in milliseconds. A timer is
 * clamped to a few milliseconds anyway, so this is as often as it gets. */
#define portBROWSER_SLEEP_MS			1

/* The longest the tasks keep the thread from the browser when they never all
 * block at once - when some task only yields, or delays for less than a tick,
 * in a loop. Past this the browser gets its turn at the next context switch
 * anyway: the page has to draw and take its events, and the tick only advances
 * on the browser's turns. */
#define portMAX_BUSY_MS					50.0

/* Ticks fed in one round at most. A page in a background tab gets its timers
 * throttled to once a second or less, and this is what keeps a long pause from
 * being replayed tick by tick. */
#define portMAX_CATCH_UP_TICKS			10000U


typedef struct {
	emscripten_fiber_t fiber;
	TaskFunction_t code;
	void *params;
	void *c_stack;
	void *asyncify_stack;
} PortTask_t;


static emscripten_fiber_t xBrowserFiber;
static uint8_t ucBrowserAsyncifyStack[ portFIBER_ASYNCIFY_STACK_SIZE ]
		__attribute__( ( aligned( portFIBER_STACK_ALIGN ) ) );
static UBaseType_t uxCriticalNesting;
static double dLastTickMs;
static double dLastBrowserMs;


/* The fiber of a task. The first member of a TCB is its pxTopOfStack, and the
 * PortTask_t sits right above it. */
static inline PortTask_t *prvGetPortTask( TaskHandle_t xTask )
{
	StackType_t *pxTopOfStack = *( StackType_t ** ) xTask;
	return ( PortTask_t * ) ( pxTopOfStack + 1 );
}


static void prvTaskEntry( void *pvArg )
{
	PortTask_t *pxPortTask = ( PortTask_t * ) pvArg;
	pxPortTask->code( pxPortTask->params );
	/* A FreeRTOS task must not return. Treat it as one that deleted itself. */
	vTaskDelete( NULL );
}


StackType_t *pxPortInitialiseStack( StackType_t *pxTopOfStack,
		TaskFunction_t pxCode, void *pvParameters )
{
	/* The kernel gives the highest usable address. The bookkeeping goes there,
	 * and the kernel's idea of the top of the stack is right below it. */
	PortTask_t *pxPortTask = ( ( PortTask_t * ) ( pxTopOfStack + 1 ) ) - 1;
	pxPortTask = ( PortTask_t * ) ( ( ( uintptr_t ) pxPortTask ) &
			~( ( uintptr_t ) ( sizeof( void * ) - 1U ) ) );

	pxPortTask->code = pxCode;
	pxPortTask->params = pvParameters;
	/* The compiler takes the stack pointer to be 16 byte aligned, and builds
	 * addresses on the stack with that in mind (an "| 8" for a "+ 8"). malloc()
	 * only promises 8, and a task on a stack it gave runs into nonsense such as
	 * a printf() which never returns. */
	pxPortTask->c_stack = aligned_alloc( portFIBER_STACK_ALIGN, portFIBER_C_STACK_SIZE );
	pxPortTask->asyncify_stack = aligned_alloc( portFIBER_STACK_ALIGN,
			portFIBER_ASYNCIFY_STACK_SIZE );
	configASSERT( ( pxPortTask->c_stack != NULL ) &&
			( pxPortTask->asyncify_stack != NULL ) );
	emscripten_fiber_init( &pxPortTask->fiber, &prvTaskEntry, pxPortTask,
			pxPortTask->c_stack, portFIBER_C_STACK_SIZE,
			pxPortTask->asyncify_stack, portFIBER_ASYNCIFY_STACK_SIZE );

	return ( ( StackType_t * ) pxPortTask ) - 1;
}


void vPortCleanUpTCB( void *pxTCB )
{
	/* Never the running task: a task which deletes itself is freed by the idle
	 * task later on, and another one is switched out by definition. */
	PortTask_t *pxPortTask = prvGetPortTask( ( TaskHandle_t ) pxTCB );
	free( pxPortTask->c_stack );
	free( pxPortTask->asyncify_stack );
	pxPortTask->c_stack = NULL;
	pxPortTask->asyncify_stack = NULL;
}


/* Feeds the tick with the time that has passed since the last call. */
static void prvCatchUpTicks( void )
{
	double dNow = emscripten_get_now();
	double dElapsed = dNow - dLastTickMs;
	uint32_t ulTicks = 0U;

	if( dElapsed >= ( double ) portTICK_PERIOD_MS )
	{
		ulTicks = ( uint32_t ) ( dElapsed / ( double ) portTICK_PERIOD_MS );
		dLastTickMs += ( double ) ulTicks * ( double ) portTICK_PERIOD_MS;
		if( ulTicks > portMAX_CATCH_UP_TICKS )
		{
			ulTicks = portMAX_CATCH_UP_TICKS;
			dLastTickMs = dNow;
		}
	}

	for( uint32_t i = 0U; i < ulTicks; i++ )
	{
		( void ) xTaskIncrementTick();
	}
}


BaseType_t xPortStartScheduler( void )
{
	emscripten_fiber_init_from_current_context( &xBrowserFiber,
			ucBrowserAsyncifyStack, sizeof( ucBrowserAsyncifyStack ) );
	dLastTickMs = emscripten_get_now();
	dLastBrowserMs = dLastTickMs;

	for( ;; )
	{
		/* Run the tasks until only the idle task is left with anything to do.
		 * The current task is the idle task when the scheduler has just started
		 * with nothing else created, and it is run every round anyway. */
		emscripten_fiber_swap( &xBrowserFiber,
				&prvGetPortTask( xTaskGetCurrentTaskHandle() )->fiber );

		emscripten_sleep( portBROWSER_SLEEP_MS );
		dLastBrowserMs = emscripten_get_now();

		prvCatchUpTicks();
		vTaskSwitchContext();
	}

	/* Not reached. */
	return pdFALSE;
}


void vPortEndScheduler( void )
{
	/* A page cannot end its event loop. uv_hal ends the scheduler only on its
	 * way out of the program, which is the same as stopping here. */
	emscripten_force_exit( 0 );
}


void vPortYield( void )
{
	TaskHandle_t xOld = xTaskGetCurrentTaskHandle();

	/* The tick only advances while the browser runs, which is what makes a task
	 * which yields to its peers in a loop see time pass at all. */
	prvCatchUpTicks();
	vTaskSwitchContext();

	TaskHandle_t xNew = xTaskGetCurrentTaskHandle();
	if( ( emscripten_get_now() - dLastBrowserMs ) > portMAX_BUSY_MS )
	{
		/* The tasks have not let go for too long: the browser's turn, whoever
		 * is due next. It is switched to when the browser hands back. */
		emscripten_fiber_swap( &prvGetPortTask( xOld )->fiber, &xBrowserFiber );
	}
	else if( xNew == xTaskGetIdleTaskHandle() )
	{
		/* Nothing but the idle task is ready: the browser's turn. The idle task
		 * itself is run once the browser hands the thread back. */
		emscripten_fiber_swap( &prvGetPortTask( xOld )->fiber, &xBrowserFiber );
	}
	else if( xNew != xOld )
	{
		emscripten_fiber_swap( &prvGetPortTask( xOld )->fiber,
				&prvGetPortTask( xNew )->fiber );
	}
	else
	{
		/* the same task goes on */
	}
}


void vPortEnterCritical( void )
{
	uxCriticalNesting++;
}


void vPortExitCritical( void )
{
	if( uxCriticalNesting > 0U )
	{
		uxCriticalNesting--;
	}
}

#endif /* __EMSCRIPTEN__ */
