/*
 * Serial_UART1.cpp - Instancia Serial (UART1) y su interrupcion.
 * Va en un archivo propio para que solo se enlace si el sketch la usa.
 */
#include "Arduino.h"

UARTClass Serial(1, PIN_SERIAL_TX, PIN_SERIAL_RX);

extern "C" void UART1_IRQHandler(void)
{
    Serial._irq();
}

/* serialEvent(): se llama despues de cada loop() si hay datos en Serial */
void serialEvent(void) __attribute__((weak));

void arduino::serialEventRun(void)
{
    if (serialEvent && Serial.available()) serialEvent();
}

/* printf() de C -> Serial (solo si el sketch usa Serial) */
extern "C" int _write(int fd, const char *buf, int len)
{
    if (fd == 1 || fd == 2) {
        Serial.write((const uint8_t *)buf, (size_t)len);
    }
    return len;
}
