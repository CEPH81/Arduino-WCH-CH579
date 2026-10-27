/*
 * Serial_UART0.cpp - Instancia Serial0 (UART0) y su interrupcion.
 * Va en un archivo propio para que solo se enlace si el sketch la usa.
 */
#include "Arduino.h"

UARTClass Serial0(0, PIN_SERIAL0_TX, PIN_SERIAL0_RX);

extern "C" void UART0_IRQHandler(void)
{
    Serial0._irq();
}
