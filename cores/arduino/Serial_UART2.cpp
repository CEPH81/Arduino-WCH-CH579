/*
 * Serial_UART2.cpp - Instancia Serial2 (UART2) y su interrupcion.
 * Va en un archivo propio para que solo se enlace si el sketch la usa.
 */
#include "Arduino.h"

UARTClass Serial2(2, PIN_SERIAL2_TX, PIN_SERIAL2_RX);

extern "C" void UART2_IRQHandler(void)
{
    Serial2._irq();
}
