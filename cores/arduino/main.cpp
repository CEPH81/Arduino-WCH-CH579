/*
 * main.cpp - Punto de entrada de los sketches
 */
#include "Arduino.h"

/* Las variantes pueden redefinirla para inicializar hardware propio */
void initVariant(void) __attribute__((weak));
void initVariant(void) { }

int main(void)
{
    init();
    initVariant();

    setup();

    for (;;) {
        loop();
        if (arduino::serialEventRun) {
            arduino::serialEventRun();
        }
    }
    return 0;
}
