/*
    FreeRTOS V7.1.0 - Copyright (C) 2011 Real Time Engineers Ltd.
	

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS tutorial books are available in pdf and paperback.        *
     *    Complete, revised, and edited pdf reference manuals are also       *
     *    available.                                                         *
     *                                                                       *
     *    Purchasing FreeRTOS documentation will not only help you, by       *
     *    ensuring you get running as quickly as possible and with an        *
     *    in-depth knowledge of how to use FreeRTOS, it will also help       *
     *    the FreeRTOS project to continue with its mission of providing     *
     *    professional grade, cross platform, de facto standard solutions    *
     *    for microcontrollers - completely free of charge!                  *
     *                                                                       *
     *    >>> See http://www.FreeRTOS.org/Documentation for details. <<<     *
     *                                                                       *
     *    Thank you for using FreeRTOS, and thank you for your support!      *
     *                                                                       *
    ***************************************************************************


    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation AND MODIFIED BY the FreeRTOS exception.
    >>>NOTE<<< The modification to the GPL is included to allow you to
    distribute a combined work that includes FreeRTOS without being obliged to
    provide the source code for proprietary components outside of the FreeRTOS
    kernel.  FreeRTOS is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
    more details. You should have received a copy of the GNU General Public
    License and the FreeRTOS license exception along with FreeRTOS; if not it
    can be viewed here: http://www.freertos.org/a00114.html and also obtained
    by writing to Richard Barry, contact details for whom are available on the
    FreeRTOS WEB site.

    1 tab == 4 spaces!

    http://www.FreeRTOS.org - Documentation, latest information, license and
    contact details.

    http://www.SafeRTOS.com - A version that is certified for use in safety
    critical systems.

    http://www.OpenRTOS.com - Commercial support, development, porting,
    licensing and training services.
*/

#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#include "uv_hal_config.h"

#if CONFIG_RTOS


#ifndef __IASMARM__
/* For SystemCoreClock */
#if CONFIG_TARGET_LPC1549
#include "chip.h"
#endif
#endif

#if CONFIG_TARGET_LPC1549
/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.
 *----------------------------------------------------------*/
#define configUSE_PREEMPTION		1
#define configUSE_IDLE_HOOK			1
#define configCPU_CLOCK_HZ			( ( unsigned long ) SystemCoreClock )
#define configMAX_PRIORITIES		( 8 )
#define configUSE_TICK_HOOK			0
#define configTICK_RATE_HZ			( ( portTickType ) 1000 )
#define configMINIMAL_STACK_SIZE	( ( unsigned short ) 128 )
#define configSUPPORT_DYNAMIC_ALLOCATION	1
#define configTOTAL_HEAP_SIZE 		CONFIG_RTOS_HEAP_SIZE
#define configMAX_TASK_NAME_LEN		( 20 )
#define configUSE_TRACE_FACILITY	1
#define configUSE_16_BIT_TICKS		0
#define configIDLE_SHOULD_YIELD		1
#define configUSE_CO_ROUTINES 		0
#define configUSE_MUTEXES			1
#define configUSE_TICKLESS_IDLE		0

#define configMAX_CO_ROUTINE_PRIORITIES ( 2 )

#define configUSE_COUNTING_SEMAPHORES 	1
#define configUSE_ALTERNATIVE_API 		0
#define configCHECK_FOR_STACK_OVERFLOW	0
#define configUSE_RECURSIVE_MUTEXES		1
#define configQUEUE_REGISTRY_SIZE		10
#define configGENERATE_RUN_TIME_STATS	0

#define configSUPPORT_STATIC_ALLOCATION	0

/* Software timer definitions. */
#define configUSE_TIMERS				0
#define configTIMER_TASK_PRIORITY		( 2 )
#define configTIMER_QUEUE_LENGTH		2
#define configTIMER_TASK_STACK_DEPTH	( 80 )

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_vTaskPrioritySet			1
#define INCLUDE_uxTaskPriorityGet			1
#define INCLUDE_vTaskDelete					1
#define INCLUDE_vTaskCleanUpResources		0
#define INCLUDE_vTaskSuspend				1
#define INCLUDE_vTaskDelayUntil				1
#define INCLUDE_vTaskDelay					1
#define INCLUDE_uxTaskGetStackHighWaterMark	1

#define configUSE_PORT_OPTIMISED_TASK_SELECTION	1

#define configGENERATE_RUN_TIME_STATS			0


/* Use the system definition, if there is one */
#ifdef __NVIC_PRIO_BITS
	#define configPRIO_BITS       __NVIC_PRIO_BITS
#else
	#define configPRIO_BITS       5        /* 32 priority levels */
#endif

#if defined(CORE_M3)

/* The lowest interrupt priority that can be used in a call to a "set priority"
function. */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY            0xf

/* The highest interrupt priority that can be used by any interrupt service
routine that makes calls to interrupt safe FreeRTOS API functions.  DO NOT CALL
INTERRUPT SAFE FREERTOS API FUNCTIONS FROM ANY INTERRUPT THAT HAS A HIGHER
PRIORITY THAN THIS! (higher priorities are lower numeric values. */
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY   1

/* Interrupt priorities used by the kernel port layer itself.  These are generic
to all Cortex-M ports, and do not rely on any particular library functions. */
#define configKERNEL_INTERRUPT_PRIORITY        ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
/* !!!! configMAX_SYSCALL_INTERRUPT_PRIORITY must not be set to zero !!!!
See http://www.FreeRTOS.org/RTOS-Cortex-M3-M4.html. */
#define configMAX_SYSCALL_INTERRUPT_PRIORITY   ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )
#endif /* defined(CORE_M3) */

/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names - or at least those used in the unmodified vector table. */
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler


#else

/*-----------------------------------------------------------
 * Application specific definitions.
 *
 * These definitions should be adjusted for your particular hardware and
 * application requirements.
 *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.  See
 * http://www.freertos.org/a00110.html
 *----------------------------------------------------------*/

#define configUSE_PREEMPTION					1
#define configUSE_PORT_OPTIMISED_TASK_SELECTION	0
#define configUSE_IDLE_HOOK						0
#define configUSE_TICK_HOOK						1
#define configUSE_DAEMON_TASK_STARTUP_HOOK		0
#define configTICK_RATE_HZ						( 1000 )
#define configMINIMAL_STACK_SIZE				( ( unsigned short ) 1024 )
#define configTOTAL_HEAP_SIZE					(  CONFIG_RTOS_HEAP_SIZE )
#define configMAX_TASK_NAME_LEN					( 12 )
#define configUSE_TRACE_FACILITY				1
#define configUSE_16_BIT_TICKS					0
#define configIDLE_SHOULD_YIELD					1
#define configUSE_MUTEXES						1
#define configCHECK_FOR_STACK_OVERFLOW			0
#define configUSE_RECURSIVE_MUTEXES				1
#define configQUEUE_REGISTRY_SIZE				20
#define configUSE_APPLICATION_TASK_TAG			1
#define configUSE_COUNTING_SEMAPHORES			1
#define configUSE_ALTERNATIVE_API				0
#define configUSE_QUEUE_SETS					1
#define configUSE_TASK_NOTIFICATIONS			1
#define configSUPPORT_STATIC_ALLOCATION			0

/* Software timer related configuration options.  The maximum possible task
priority is configMAX_PRIORITIES - 1.  The priority of the timer task is
deliberately set higher to ensure it is correctly capped back to
configMAX_PRIORITIES - 1. */
#define configUSE_TIMERS						1
#define configTIMER_TASK_PRIORITY				( configMAX_PRIORITIES - 1 )
#define configTIMER_QUEUE_LENGTH				20
#define configTIMER_TASK_STACK_DEPTH			( configMINIMAL_STACK_SIZE * 2 )

#define configMAX_PRIORITIES					( 7 )

/* Run time stats gathering configuration options. */
unsigned long ulGetRunTimeCounterValue( void ); /* Prototype of function that returns run time counter. */
void vConfigureTimerForRunTimeStats( void );	/* Prototype of function that initialises the run time counter. */
#define configGENERATE_RUN_TIME_STATS			1

/* Co-routine related configuration options. */
#define configUSE_CO_ROUTINES 					0
#define configMAX_CO_ROUTINE_PRIORITIES			( 2 )

/* This demo can use of one or more example stats formatting functions.  These
format the raw data provided by the uxTaskGetSystemState() function in to human
readable ASCII form.  See the notes in the implementation of vTaskList() within
FreeRTOS/Source/tasks.c for limitations. */
#define configUSE_STATS_FORMATTING_FUNCTIONS	0

/* Enables the test whereby a stack larger than the total heap size is
requested. */
#define configSTACK_DEPTH_TYPE uint32_t

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function.  In most cases the linker will remove unused
functions anyway. */
#define INCLUDE_vTaskPrioritySet				1
#define INCLUDE_uxTaskPriorityGet				1
#define INCLUDE_vTaskDelete						1
#define INCLUDE_vTaskCleanUpResources			0
#define INCLUDE_vTaskSuspend					1
#define INCLUDE_vTaskDelayUntil					1
#define INCLUDE_vTaskDelay						1
#define INCLUDE_uxTaskGetStackHighWaterMark		1
#define INCLUDE_uxTaskGetStackHighWaterMark2	1
#define INCLUDE_xTaskGetSchedulerState			1
#define INCLUDE_xTimerGetTimerDaemonTaskHandle	1
#define INCLUDE_xTaskGetIdleTaskHandle			1
#define INCLUDE_xTaskGetHandle					1
#define INCLUDE_eTaskGetState					1
#define INCLUDE_xSemaphoreGetMutexHolder		1
#define INCLUDE_xTimerPendFunctionCall			1
#define INCLUDE_xTaskAbortDelay					1

#define configINCLUDE_MESSAGE_BUFFER_AMP_DEMO	0
#if ( configINCLUDE_MESSAGE_BUFFER_AMP_DEMO == 1 )
	extern void vGenerateCoreBInterrupt( void * xUpdatedMessageBuffer );
	#define sbSEND_COMPLETED( pxStreamBuffer ) vGenerateCoreBInterrupt( pxStreamBuffer )
#endif /* configINCLUDE_MESSAGE_BUFFER_AMP_DEMO */

extern void vAssertCalled( const char * const pcFileName,  unsigned long ulLine );

/* It is a good idea to define configASSERT() while developing.  configASSERT()
uses the same semantics as the standard C assert() macro.  Don't define
configASSERT() when performing code coverage tests though, as it is not
intended to asserts() to fail, some some code is intended not to run if no
errors are present. */
#define configASSERT( x ) if( ( x ) == 0 ) vAssertCalled(  __FILE__, __LINE__ )

#define configUSE_MALLOC_FAILED_HOOK			1

/* networking definitions */
#define configMAC_ISR_SIMULATOR_PRIORITY	( configMAX_PRIORITIES - 1 )

/* Prototype for the function used to print out.  In this case it prints to the
console before the network is connected then a UDP port after the network has
connected. */
extern void vLoggingPrintf( const char *pcFormatString, ... );

/* Set to 1 to print out debug messages.  If ipconfigHAS_DEBUG_PRINTF is set to
1 then FreeRTOS_debug_printf should be defined to the function used to print
out the debugging messages. */
#define ipconfigHAS_DEBUG_PRINTF	1
#if( ipconfigHAS_DEBUG_PRINTF == 1 )
	#define FreeRTOS_debug_printf(X)	vLoggingPrintf X
#endif

/* Set to 1 to print out non debugging messages, for example the output of the
FreeRTOS_netstat() command, and ping replies.  If ipconfigHAS_PRINTF is set to 1
then FreeRTOS_printf should be set to the function used to print out the
messages. */
#define ipconfigHAS_PRINTF			0
#if( ipconfigHAS_PRINTF == 1 )
	#define FreeRTOS_printf(X)			vLoggingPrintf X
#endif
#endif /* FREERTOS_CONFIG_H */


#endif /* FREERTOS_CONFIG_H */

#endif
