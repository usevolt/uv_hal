/*
 * uw_utilities.h
 * Reusable utility functions
 *
 *  Created on: Feb 18, 2015
 *      Author: usenius
 */

#ifndef UW_UTILITIES_H_
#define UW_UTILITIES_H_

#include "uw_types.h"
#include "uw_can.h"
#include "uw_errors.h"
#include <stdbool.h>


#define GetInt16(hibyte,lobyte)		(((uint16_t) hibyte << 8) + (uint16_t) lobyte)




/// @brief: returns the masked value of 'value'
/// @example: GET_MASKED(0b111000, 0b100001);	// returns 0b100000;
#define GET_MASKED(value, mask)		(value & mask)





int uw_atoi (const char* str);


/// @brief: Initializes a delay.
/// @param delay_ms The desired length of the delay
/// @param p A pointer to variable which will hold the current delay count
void uw_start_delay(unsigned int delay_ms, int* p);

/// @brief: Delay function. Can be used to create different delays in a discrete cyclical step function system.
/// @param delay_ms The length of the delay in ms.
/// @param step_ms The duration between cyclic calls in ms.
/// @param p Pointer to a delay counter variable. Variable's value is increased every step and
/// @pre: uw_start_delay should have been called
/// when it exceeds delay_ms, true is returned.
/// @example:
/// 	static int p;
/// 	uw_start_delay(1500, &p);
/// 	while (true) {
///			if (uw_delay(1, &p)) {
///				...
///			}
/// 	}
bool uw_delay(unsigned int step_ms, int* p);


/// @brief:  function to ease debug console coding for on/off parameters
/// arg:     command line argument, eg args[0]
/// current: current value of the on/off parameter
/// param:   name of the param for console feedback print
/// example:
///    Worklights = Debug_ParamOnOff (args[0], Worklights, "work lights")
///    parses command "work off"
///    prints "turning work lights off"
bool Debug_ParamOnOff (const char* arg, bool current, const char* param);


/// @brief:  prints can message contents in debug console
void Debug_PrintMessage (uw_can_message_st* msg);



/// @brief: Set's the user's application pointer.
/// User can set a pointer to any variable which will be passed to all this library's
/// callback functions. This makes it easier to write object oriented code, since
/// the callback function has the object's instace as a parameter.
void uw_set_application_ptr(void *ptr);



/// @brief: Returns this controller's name as a null-terminated string
char *uw_get_hardware_name();


#ifdef LPC11C14
/// @brief: Defines the interrupts sources on this hardware
typedef enum {
	/******  Cortex-M0 Processor Exceptions Numbers ***************************************************/
	  INT_NMI           = -14,    /*!< 2 Non Maskable Interrupt                           */
	  INT_HARD_FAULT                = -13,    /*!< 3 Cortex-M0 Hard Fault Interrupt                   */
	  INT_SVCALL                   = -5,     /*!< 11 Cortex-M0 SV Call Interrupt                     */
	  INT_PENDSV                   = -2,     /*!< 14 Cortex-M0 Pend SV Interrupt                     */
	  INT_SYSTICK                  = -1,     /*!< 15 Cortex-M0 System Tick Interrupt                 */

	/******  LPC11Cxx or LPC11xx Specific Interrupt Numbers *******************************************************/
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
} uw_int_sources_e;

enum {
	INT_LOWEST_PRIORITY = 3
};

#elif defined(LPC1785)
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

} uw_int_sources_e;

enum {
	INT_LOWEST_PRIORITY = 31
};
#endif

/// @bref: Set's the interrupt sources priority. If the priority is not available on
/// the hardware, an error is returned and logged to stdout.
uw_errors_e uw_set_int_priority(uw_int_sources_e, unsigned int priority);




/**** PROTECTED FUNCTIONS ******/
/* These are meant only for HAL library's internal usage */

/// @brief: Get's the user's application pointer
/// User can set a pointer to any variable which will be passed to all this library's
/// callback functions. This makes it easier to write object oriented code.
void *__uw_get_user_ptr();


#endif /* UW_UTILITIES_H_ */
