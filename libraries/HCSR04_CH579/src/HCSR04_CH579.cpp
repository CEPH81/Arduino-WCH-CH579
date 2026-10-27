#include "HCSR04_CH579.h"

#define HCSR04_TRIG_US        10u      /* pulso de disparo (datasheet: >= 10 us) */
#define HCSR04_BLANKING_US    300u     /* el modulo termina su rafaga de 40 kHz */
#define HCSR04_RISE_TIMEOUT   30000u   /* us esperando que Echo suba */
#define HCSR04_FALL_TIMEOUT   30000u   /* us de pulso maximo (~5 m) */

void HCSR04::begin(void)
{
    digitalWrite(_trig, LOW);
    pinMode(_trig, OUTPUT);
    pinMode(_echo, INPUT_PULLUP);
}

uint32_t HCSR04::readEchoUs(uint8_t *stage)
{
    /* Pulso de disparo */
    digitalWrite(_trig, HIGH);
    delayMicroseconds(HCSR04_TRIG_US);
    digitalWrite(_trig, LOW);

    delayMicroseconds(HCSR04_BLANKING_US);

    uint32_t t0 = micros();
    while (digitalRead(_echo) == LOW) {
        if (micros() - t0 > HCSR04_RISE_TIMEOUT) { if (stage) *stage = 0; return 0; }
    }
    uint32_t start = micros();
    while (digitalRead(_echo) == HIGH) {
        if (micros() - start > HCSR04_FALL_TIMEOUT) { if (stage) *stage = 1; return 0; }
    }
    uint32_t width = micros() - start;
    if (stage) *stage = 2;
    return width;
}

uint16_t HCSR04::readDistanceCm(void)
{
    uint8_t st;
    uint32_t us = readEchoUs(&st);
    if (st != 2) return HCSR04_NO_ECHO;
    return (uint16_t)(us / 58u);
}
