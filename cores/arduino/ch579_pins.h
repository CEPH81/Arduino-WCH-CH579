/*
 * ch579_pins.h - Acceso interno a los puertos GPIO (uso del core)
 */
#ifndef CH579_PINS_H
#define CH579_PINS_H

#include "Arduino.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline int ch579_pin_valid(pin_size_t p) { return p < NUM_DIGITAL_PINS; }
static inline uint32_t ch579_pin_mask(pin_size_t p) { return 1UL << g_pinDescription[p].bit; }
static inline int ch579_pin_portb(pin_size_t p) { return g_pinDescription[p].port; }

/* Desactiva la salida PWM del pin, si la tenia (wiring_analog.c) */
void ch579_pwm_detach(pin_size_t pin);
/* Cambia el sentido de un pin en modo OUTPUT_OPENDRAIN (wiring_digital.c) */
int  ch579_pin_is_opendrain(pin_size_t pin);

#ifdef __cplusplus
}
#endif

#endif
