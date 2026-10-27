/*
 * wiring_shift.c - shiftOut() / shiftIn()
 */
#include "Arduino.h"

void shiftOut(pin_size_t dataPin, pin_size_t clockPin, BitOrder bitOrder, uint8_t val)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (bitOrder == LSBFIRST) digitalWrite(dataPin, (val & (1 << i)) ? HIGH : LOW);
        else                      digitalWrite(dataPin, (val & (1 << (7 - i))) ? HIGH : LOW);
        digitalWrite(clockPin, HIGH);
        digitalWrite(clockPin, LOW);
    }
}

uint8_t shiftIn(pin_size_t dataPin, pin_size_t clockPin, BitOrder bitOrder)
{
    uint8_t value = 0;
    for (uint8_t i = 0; i < 8; i++) {
        digitalWrite(clockPin, HIGH);
        if (bitOrder == LSBFIRST) value |= (digitalRead(dataPin) ? 1 : 0) << i;
        else                      value |= (digitalRead(dataPin) ? 1 : 0) << (7 - i);
        digitalWrite(clockPin, LOW);
    }
    return value;
}
