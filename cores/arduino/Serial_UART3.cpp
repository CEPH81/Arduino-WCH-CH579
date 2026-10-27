/*
 * Serial_UART3.cpp - Instancia Serial3 (UART3) y su interrupcion.
 * Va en un archivo propio para que solo se enlace si el sketch la usa.
 */
#include "Arduino.h"

UARTClass Serial3(3, PIN_SERIAL3_TX, PIN_SERIAL3_RX);

extern "C" void UART3_IRQHandler(void)
{
    Serial3._irq();
}
