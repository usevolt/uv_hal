//*****************************************************************************
// LPC407x_8x Microcontroller Startup code for use with LPCXpresso IDE
//
// Version : 150706
//*****************************************************************************
//
// Copyright(C) NXP Semiconductors, 2014-2015, 2020
// All rights reserved.
//
// NXP Confidential. This software is owned or controlled by NXP and may only be
// used strictly in accordance with the applicable license terms.
//
// By expressly accepting such terms or by downloading, installing, activating
// and/or otherwise using the software, you are agreeing that you have read, and
// that you agree to comply with and are bound by, such license terms.
// 
// If you do not agree to be bound by the applicable license terms, then you may not
// retain, install, activate or otherwise use the software.
//*****************************************************************************
#include <stdint.h>


const uint32_t OscRateIn = 12000000;
const uint32_t RTCOscRateIn = 32768;


#if defined (__cplusplus)
#ifdef __REDLIB__
#error Redlib does not support C++
#else
//*****************************************************************************
//
// The entry point for the C++ library startup
//
//*****************************************************************************
extern "C" {
    extern void __libc_init_array(void);
}
#endif
#endif

#define WEAK __attribute__ ((weak))
#define ALIAS(f) __attribute__ ((weak, alias (#f)))

//*****************************************************************************
#if defined (__cplusplus)
extern "C" {
#endif

// Declaration of external SystemInit function
extern void Chip_SystemInit(void);

//*****************************************************************************
//
// Forward declaration of the default handlers. These are aliased.
// When the application defines a handler (with the same name), this will 
// automatically take precedence over these weak definitions
//
//*****************************************************************************
     void ResetISR(void);
WEAK void NMI_Handler(void);
WEAK void HardFault_Handler(void);
WEAK void MemManage_Handler(void);
WEAK void BusFault_Handler(void);
WEAK void UsageFault_Handler(void);
WEAK void SVC_Handler(void);
WEAK void DebugMon_Handler(void);
WEAK void PendSV_Handler(void);
WEAK void SysTick_Handler(void);
WEAK void IntDefaultHandler(void);

//*****************************************************************************
//
// Forward declaration of the specific IRQ handlers. These are aliased
// to the IntDefaultHandler, which is a 'forever' loop. When the application
// defines a handler (with the same name), this will automatically take 
// precedence over these weak definitions
//
//*****************************************************************************
void WDT_IRQHandler(void) ALIAS(IntDefaultHandler);
void TIMER0_IRQHandler(void) ALIAS(IntDefaultHandler);
void TIMER1_IRQHandler(void) ALIAS(IntDefaultHandler);
void TIMER2_IRQHandler(void) ALIAS(IntDefaultHandler);
void TIMER3_IRQHandler(void) ALIAS(IntDefaultHandler);
void UART0_IRQHandler(void) ALIAS(IntDefaultHandler);
void UART1_IRQHandler(void) ALIAS(IntDefaultHandler);
void UART2_IRQHandler(void) ALIAS(IntDefaultHandler);
void UART3_IRQHandler(void) ALIAS(IntDefaultHandler);
void PWM1_IRQHandler(void) ALIAS(IntDefaultHandler);
void I2C0_IRQHandler(void) ALIAS(IntDefaultHandler);
void I2C1_IRQHandler(void) ALIAS(IntDefaultHandler);
void I2C2_IRQHandler(void) ALIAS(IntDefaultHandler);
void SPI_IRQHandler(void) ALIAS(IntDefaultHandler);
void SSP0_IRQHandler(void) ALIAS(IntDefaultHandler);
void SSP1_IRQHandler(void) ALIAS(IntDefaultHandler);
void PLL0_IRQHandler(void) ALIAS(IntDefaultHandler);
void RTC_IRQHandler(void) ALIAS(IntDefaultHandler);
void EINT0_IRQHandler(void) ALIAS(IntDefaultHandler);
void EINT1_IRQHandler(void) ALIAS(IntDefaultHandler);
void EINT2_IRQHandler(void) ALIAS(IntDefaultHandler);
void EINT3_IRQHandler(void) ALIAS(IntDefaultHandler);
void ADC_IRQHandler(void) ALIAS(IntDefaultHandler);
void BOD_IRQHandler(void) ALIAS(IntDefaultHandler);
void USB_IRQHandler(void) ALIAS(IntDefaultHandler);
void CAN_IRQHandler(void) ALIAS(IntDefaultHandler);
void DMA_IRQHandler(void) ALIAS(IntDefaultHandler);
void I2S_IRQHandler(void) ALIAS(IntDefaultHandler);
#if defined (__USE_LPCOPEN)
void ETH_IRQHandler(void) ALIAS(IntDefaultHandler);
#else
void ENET_IRQHandler(void) ALIAS(IntDefaultHandler);
#endif
void RIT_IRQHandler(void) ALIAS(IntDefaultHandler);
void MCPWM_IRQHandler(void) ALIAS(IntDefaultHandler);
void QEI_IRQHandler(void) ALIAS(IntDefaultHandler);
void PLL1_IRQHandler(void) ALIAS(IntDefaultHandler);
void USBActivity_IRQHandler(void) ALIAS(IntDefaultHandler);
void CANActivity_IRQHandler(void) ALIAS(IntDefaultHandler);
#if defined (__USE_LPCOPEN)
void SDIO_IRQHandler(void) ALIAS(IntDefaultHandler);
#else
void MCI_IRQHandler(void) ALIAS(IntDefaultHandler);
#endif
void UART4_IRQHandler(void) ALIAS(IntDefaultHandler);
void SSP2_IRQHandler(void) ALIAS(IntDefaultHandler);
void LCD_IRQHandler(void) ALIAS(IntDefaultHandler);
void GPIO_IRQHandler(void) ALIAS(IntDefaultHandler);
void PWM0_IRQHandler(void) ALIAS(IntDefaultHandler);
void EEPROM_IRQHandler(void) ALIAS(IntDefaultHandler);

//*****************************************************************************
//
// The entry point for the application.
// __main() is the entry point for Redlib based applications
// main() is the entry point for Newlib based applications
//
//*****************************************************************************
#if defined (__REDLIB__)
extern void __main(void);
#endif
extern int main(void);
//*****************************************************************************
//
// External declaration for the pointer to the stack top from the Linker Script
//
//*****************************************************************************
extern void _vStackTop(void);

//*****************************************************************************
//
// External declaration for LPC MCU vector table checksum from  Linker Script
//
//*****************************************************************************
WEAK extern void __valid_user_code_checksum();

//*****************************************************************************
#if defined (__cplusplus)
} // extern "C"
#endif
//*****************************************************************************
//
// The vector table.
// This relies on the linker script to place at correct location in memory.
//
//*****************************************************************************
extern void (* const g_pfnVectors[])(void);
__attribute__ ((used,section(".isr_vector")))
void (* const g_pfnVectors[])(void) = {
    // Core Level - CM4
    &_vStackTop,                        // The initial stack pointer
    ResetISR,                           // The reset handler
    NMI_Handler,                        // The NMI handler
    HardFault_Handler,                  // The hard fault handler
    MemManage_Handler,                  // The MPU fault handler
    BusFault_Handler,                   // The bus fault handler
    UsageFault_Handler,                 // The usage fault handler
    __valid_user_code_checksum,         // LPC MCU Checksum
    0,                                  // Reserved
    0,                                  // Reserved
    0,                                  // Reserved
    SVC_Handler,                        // SVCall handler
    DebugMon_Handler,                   // Debug monitor handler
    0,                                  // Reserved
    PendSV_Handler,                     // The PendSV handler
    SysTick_Handler,                    // The SysTick handler

    // Chip Level - LPC40xx
    WDT_IRQHandler,                     // 16, 0x40 - WDT
    TIMER0_IRQHandler,                  // 17, 0x44 - TIMER0
    TIMER1_IRQHandler,                  // 18, 0x48 - TIMER1
    TIMER2_IRQHandler,                  // 19, 0x4c - TIMER2
    TIMER3_IRQHandler,                  // 20, 0x50 - TIMER3
    UART0_IRQHandler,                   // 21, 0x54 - UART0
    UART1_IRQHandler,                   // 22, 0x58 - UART1
    UART2_IRQHandler,                   // 23, 0x5c - UART2
    UART3_IRQHandler,                   // 24, 0x60 - UART3
    PWM1_IRQHandler,                    // 25, 0x64 - PWM1
    I2C0_IRQHandler,                    // 26, 0x68 - I2C0
    I2C1_IRQHandler,                    // 27, 0x6c - I2C1
    I2C2_IRQHandler,                    // 28, 0x70 - I2C2
    IntDefaultHandler,                  // 29, Not used
    SSP0_IRQHandler,                    // 30, 0x78 - SSP0
    SSP1_IRQHandler,                    // 31, 0x7c - SSP1
    PLL0_IRQHandler,                    // 32, 0x80 - PLL0 (Main PLL)
    RTC_IRQHandler,                     // 33, 0x84 - RTC
    EINT0_IRQHandler,                   // 34, 0x88 - EINT0
    EINT1_IRQHandler,                   // 35, 0x8c - EINT1
    EINT2_IRQHandler,                   // 36, 0x90 - EINT2
    EINT3_IRQHandler,                   // 37, 0x94 - EINT3
    ADC_IRQHandler,                     // 38, 0x98 - ADC
    BOD_IRQHandler,                     // 39, 0x9c - BOD
    USB_IRQHandler,                     // 40, 0xA0 - USB
    CAN_IRQHandler,                     // 41, 0xa4 - CAN
    DMA_IRQHandler,                     // 42, 0xa8 - GP DMA
    I2S_IRQHandler,                     // 43, 0xac - I2S
#if defined (__USE_LPCOPEN)
    ETH_IRQHandler,                     // 44, 0xb0 - Ethernet
    SDIO_IRQHandler,                    // 45, 0xb4 - SD/MMC card I/F
#else
    ENET_IRQHandler,                    // 44, 0xb0 - Ethernet
    MCI_IRQHandler,                     // 45, 0xb4 - SD/MMC card I/F
#endif                               
    MCPWM_IRQHandler,                   // 46, 0xb8 - Motor Control PWM
    QEI_IRQHandler,                     // 47, 0xbc - Quadrature Encoder
    PLL1_IRQHandler,                    // 48, 0xc0 - PLL1 (USB PLL)
    USBActivity_IRQHandler,             // 49, 0xc4 - USB Activity interrupt to wakeup
    CANActivity_IRQHandler,             // 50, 0xc8 - CAN Activity interrupt to wakeup
    UART4_IRQHandler,                   // 51, 0xcc - UART4
    SSP2_IRQHandler,                    // 52, 0xd0 - SSP2
    LCD_IRQHandler,                     // 53, 0xd4 - LCD
    GPIO_IRQHandler,                    // 54, 0xd8 - GPIO
    PWM0_IRQHandler,                    // 55, 0xdc - PWM0
    EEPROM_IRQHandler,                  // 56, 0xe0 - EEPROM

};

//*****************************************************************************
// Functions to carry out the initialization of RW and BSS data sections. These
// are written as separate functions rather than being inlined within the
// ResetISR() function in order to cope with MCUs with multiple banks of
// memory.
//*****************************************************************************
__attribute__ ((section(".after_vectors")))
void data_init(unsigned int romstart, unsigned int start, unsigned int len) {
    unsigned int *pulDest = (unsigned int*) start;
    unsigned int *pulSrc = (unsigned int*) romstart;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = *pulSrc++;
}

__attribute__ ((section(".after_vectors")))
void bss_init(unsigned int start, unsigned int len) {
    unsigned int *pulDest = (unsigned int*) start;
    unsigned int loop;
    for (loop = 0; loop < len; loop = loop + 4)
        *pulDest++ = 0;
}

//*****************************************************************************
// The following symbols are constructs generated by the linker, indicating
// the location of various points in the "Global Section Table". This table is
// created by the linker via the Code Red managed linker script mechanism. It
// contains the load address, execution address and length of each RW data
// section and the execution and length of each BSS (zero initialized) section.
//*****************************************************************************
extern unsigned int __data_section_table;
extern unsigned int __data_section_table_end;
extern unsigned int __bss_section_table;
extern unsigned int __bss_section_table_end;

//*****************************************************************************
// Reset entry point for your code.
// Sets up a simple runtime environment and initializes the C/C++
// library.
//*****************************************************************************
__attribute__ ((section(".after_vectors")))
void
ResetISR(void) {

    //
    // Copy the data sections from flash to SRAM.
    //
    unsigned int LoadAddr, ExeAddr, SectionLen;
    unsigned int *SectionTableAddr;

    // Load base address of Global Section Table
    SectionTableAddr = &__data_section_table;

    // Copy the data sections from flash to SRAM.
    while (SectionTableAddr < &__data_section_table_end) {
        LoadAddr = *SectionTableAddr++;
        ExeAddr = *SectionTableAddr++;
        SectionLen = *SectionTableAddr++;
        data_init(LoadAddr, ExeAddr, SectionLen);
    }
    // At this point, SectionTableAddr = &__bss_section_table;
    // Zero fill the bss segment
    while (SectionTableAddr < &__bss_section_table_end) {
        ExeAddr = *SectionTableAddr++;
        SectionLen = *SectionTableAddr++;
        bss_init(ExeAddr, SectionLen);
    }

#if defined (__VFP_FP__) && !defined (__SOFTFP__)
/*
 * Code to enable the Cortex-M4 FPU only included
 * if appropriate build options have been selected.
 * Code taken from Section 7.1, Cortex-M4 TRM (DDI0439C)
 */  
    // Read CPACR (located at address 0xE000ED88)
    // Set bits 20-23 to enable CP10 and CP11 coprocessors
    // Write back the modified value to the CPACR
    asm volatile ("LDR.W R0, =0xE000ED88\n\t"
                  "LDR R1, [R0]\n\t"
                  "ORR R1, R1, #(0xF << 20)\n\t"
                  "STR R1, [R0]");  
#endif // (__VFP_FP__) && !(__SOFTFP__)

    // Check to see if we are running the code from a non-zero
    // address (eg RAM, external flash), in which case we need
    // to modify the VTOR register to tell the CPU that the
    // vector table is located at a non-0x0 address.

    // Note that we do not use the CMSIS register access mechanism,
    // as there is no guarantee that the project has been configured
    // to use CMSIS.
    unsigned int * pSCB_VTOR = (unsigned int *) 0xE000ED08;
    if ((unsigned int *)g_pfnVectors!=(unsigned int *) 0x00000000) {
        // CMSIS : SCB->VTOR = <address of vector table>
        *pSCB_VTOR = (unsigned int)g_pfnVectors;
    }

    Chip_SystemInit();

#if defined (__cplusplus)
    //
    // Call C++ library initialisation
    //
    __libc_init_array();
#endif

#if defined (__REDLIB__)
    // Call the Redlib library, which in turn calls main()
    __main() ;
#else
    main();
#endif

    //
    // main() shouldn't return, but if it does, we'll just enter an infinite loop 
    //
    while (1) {
        ;
    }
}

//*****************************************************************************
// Default exception handlers. Override the ones here by defining your own
// handler routines in your application code.
//*****************************************************************************
__attribute__ ((section(".after_vectors")))
void NMI_Handler(void)
{ while(1) {}
}

__attribute__ ((section(".after_vectors")))
void HardFault_Handler(void)
{ while(1) {}
}

__attribute__ ((section(".after_vectors")))
void MemManage_Handler(void)
{ while(1) {}
}

__attribute__ ((section(".after_vectors")))
void BusFault_Handler(void)
{ while(1) {}
}

__attribute__ ((section(".after_vectors")))
void UsageFault_Handler(void)
{ while(1) {}
}

__attribute__ ((section(".after_vectors")))
void SVC_Handler(void)
{ while(1) {}
}

__attribute__ ((section(".after_vectors")))
void DebugMon_Handler(void)
{ while(1) {}
}

__attribute__ ((section(".after_vectors")))
void PendSV_Handler(void)
{ while(1) {}
}

__attribute__ ((section(".after_vectors")))
void SysTick_Handler(void)
{ while(1) {}
}

//*****************************************************************************
//
// Processor ends up here if an unexpected interrupt occurs or a specific
// handler is not present in the application code.
//
//*****************************************************************************
__attribute__ ((section(".after_vectors")))
void IntDefaultHandler(void)
{ while(1) {}
}
