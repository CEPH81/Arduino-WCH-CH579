/*
 * wiring_pulse.c - pulseIn() / pulseInLong()
 * Mide con micros(), por lo que necesita las interrupciones activas.
 */
#include "Arduino.h"

unsigned long pulseInLong(pin_size_t pin, uint8_t state, unsigned long timeout)
{
    PinStatus want = state ? HIGH : LOW;
    uint32_t start = micros();

    /* espera a que termine un pulso que ya estuviera en curso */
    while (digitalRead(pin) == want) {
        if (micros() - start >= timeout) return 0;
    }
    /* espera al inicio del pulso */
    while (digitalRead(pin) != want) {
        if (micros() - start >= timeout) return 0;
    }
    uint32_t pulse_start = micros();
    /* espera al final del pulso */
    while (digitalRead(pin) == want) {
        if (micros() - start >= timeout) return 0;
    }
    return micros() - pulse_start;
}

unsigned long pulseIn(pin_size_t pin, uint8_t state, unsigned long timeout)
{
    return pulseInLong(pin, state, timeout);
}
