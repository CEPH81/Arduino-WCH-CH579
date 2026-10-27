/*
 * Tone.cpp - tone() / noTone() con el temporizador TMR3
 * Genera una onda cuadrada al 50 % en cualquier pin (un tono a la vez).
 */
#include "Arduino.h"
#include "ch579_pins.h"

static volatile int8_t   s_tone_pin    = -1;
static volatile uint32_t s_tone_mask   = 0;
static volatile uint8_t  s_tone_portb  = 0;
static volatile int32_t  s_tone_toggles = 0;   /* <0 = infinito */

static void tone_stop(void)
{
    TMR3_ITCfg(DISABLE, TMR0_3_IT_CYC_END);
    TMR3_Disable();
    NVIC_DisableIRQ(TMR3_IRQn);
    if (s_tone_pin >= 0) {
        if (s_tone_portb) R32_PB_CLR = s_tone_mask; else R32_PA_CLR = s_tone_mask;
    }
    s_tone_pin = -1;
}

void tone(uint8_t pin, unsigned int frequency, unsigned long duration)
{
    if (!ch579_pin_valid(pin)) return;
    if (frequency == 0) { noTone(pin); return; }

    tone_stop();
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);

    s_tone_mask  = ch579_pin_mask(pin);
    s_tone_portb = (uint8_t)ch579_pin_portb(pin);
    s_tone_toggles = duration ? (int32_t)((2UL * frequency * duration) / 1000UL) : -1;
    if (s_tone_toggles == 0) s_tone_toggles = 1;
    s_tone_pin = (int8_t)pin;

    uint32_t half = GetSysClock() / (2UL * frequency);
    if (half < 2) half = 2;
    if (half > 0x3FFFFFF) half = 0x3FFFFFF;     /* contador de 26 bits */

    TMR3_TimerInit(half);
    TMR3_ClearITFlag(TMR0_3_IT_CYC_END);
    TMR3_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
    NVIC_EnableIRQ(TMR3_IRQn);
}

void noTone(uint8_t pin)
{
    if (s_tone_pin < 0) return;
    if (pin != (uint8_t)s_tone_pin) return;
    tone_stop();
}

extern "C" void TMR3_IRQHandler(void)
{
    TMR3_ClearITFlag(TMR0_3_IT_CYC_END);
    if (s_tone_pin < 0) return;

    if (s_tone_portb) R32_PB_OUT ^= s_tone_mask;
    else              R32_PA_OUT ^= s_tone_mask;

    if (s_tone_toggles > 0 && --s_tone_toggles == 0) {
        tone_stop();
    }
}
