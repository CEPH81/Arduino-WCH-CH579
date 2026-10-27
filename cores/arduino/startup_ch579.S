/*
 * startup_ch579.S - Arranque GCC para WCH CH579 (Cortex-M0)
 *
 * Tabla de vectores tomada de startup_CH579.s del pack de Keil de WCH
 * (WCH57x_DFP 1.1.0), reescrita en sintaxis GNU.
 */
    .syntax unified
    .cpu cortex-m0
    .thumb

/* ---------------------------------------------------------------- vectores */
    .section .isr_vector, "a", %progbits
    .align 2
    .global __Vectors
    .type   __Vectors, %object
__Vectors:
    .word   _estack                 /* Tope de la pila            */
    .word   Reset_Handler           /* Reset                      */
    .word   NMI_Handler             /* NMI                        */
    .word   HardFault_Handler       /* Hard Fault                 */
    .word   0, 0, 0, 0, 0, 0, 0     /* Reservados                 */
    .word   SVC_Handler             /* SVCall                     */
    .word   0, 0                    /* Reservados                 */
    .word   PendSV_Handler          /* PendSV                     */
    .word   SysTick_Handler         /* SysTick                    */
    /* Interrupciones externas (IRQn 0..19) */
    .word   TMR0_IRQHandler         /*  0: TMR0                   */
    .word   GPIO_IRQHandler         /*  1: GPIO A/B               */
    .word   SLAVE_IRQHandler        /*  2: Puerto paralelo esclavo */
    .word   SPI0_IRQHandler         /*  3: SPI0                   */
    .word   BB_IRQHandler           /*  4: BLE baseband           */
    .word   LLE_IRQHandler          /*  5: BLE LLE                */
    .word   USB_IRQHandler          /*  6: USB                    */
    .word   ETH_IRQHandler          /*  7: Ethernet               */
    .word   TMR1_IRQHandler         /*  8: TMR1                   */
    .word   TMR2_IRQHandler         /*  9: TMR2                   */
    .word   UART0_IRQHandler        /* 10: UART0                  */
    .word   UART1_IRQHandler        /* 11: UART1                  */
    .word   RTC_IRQHandler          /* 12: RTC                    */
    .word   ADC_IRQHandler          /* 13: ADC / TouchKey         */
    .word   SPI1_IRQHandler         /* 14: SPI1                   */
    .word   LED_IRQHandler          /* 15: LED                    */
    .word   TMR3_IRQHandler         /* 16: TMR3                   */
    .word   UART2_IRQHandler        /* 17: UART2                  */
    .word   UART3_IRQHandler        /* 18: UART3                  */
    .word   WDT_IRQHandler          /* 19: Watchdog / bateria     */
    .size   __Vectors, . - __Vectors

/* ------------------------------------------------------------------- reset */
    .text
    .thumb_func
    .align 2
    .global Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    /* Copiar .data de Flash a RAM */
    ldr     r1, =_sidata
    ldr     r2, =_sdata
    ldr     r3, =_edata
1:  cmp     r2, r3
    bhs     2f
    ldr     r0, [r1]
    str     r0, [r2]
    adds    r1, r1, #4
    adds    r2, r2, #4
    b       1b
    /* Poner .bss a cero */
2:  ldr     r2, =_sbss
    ldr     r3, =_ebss
    movs    r0, #0
3:  cmp     r2, r3
    bhs     4f
    str     r0, [r2]
    adds    r2, r2, #4
    b       3b
    /* Constructores C++ y main() */
4:  bl      __libc_init_array
    bl      main
5:  b       5b
    .size   Reset_Handler, . - Reset_Handler

/* ----------------------------------------------------- manejador por defecto */
    .thumb_func
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b       Default_Handler
    .size   Default_Handler, . - Default_Handler

    .macro  weak_handler name
    .weak   \name
    .thumb_set \name, Default_Handler
    .endm

    weak_handler NMI_Handler
    weak_handler HardFault_Handler
    weak_handler SVC_Handler
    weak_handler PendSV_Handler
    weak_handler SysTick_Handler
    weak_handler TMR0_IRQHandler
    weak_handler GPIO_IRQHandler
    weak_handler SLAVE_IRQHandler
    weak_handler SPI0_IRQHandler
    weak_handler BB_IRQHandler
    weak_handler LLE_IRQHandler
    weak_handler USB_IRQHandler
    weak_handler ETH_IRQHandler
    weak_handler TMR1_IRQHandler
    weak_handler TMR2_IRQHandler
    weak_handler UART0_IRQHandler
    weak_handler UART1_IRQHandler
    weak_handler RTC_IRQHandler
    weak_handler ADC_IRQHandler
    weak_handler SPI1_IRQHandler
    weak_handler LED_IRQHandler
    weak_handler TMR3_IRQHandler
    weak_handler UART2_IRQHandler
    weak_handler UART3_IRQHandler
    weak_handler WDT_IRQHandler

    .end
