/*
 * wiring_digital.c - pinMode(), digitalWrite(), digitalRead()
 */
#include "Arduino.h"
#include "ch579_pins.h"

/* Pines configurados como OUTPUT_OPENDRAIN (emulado: el CH579 no tiene
 * salida de drenador abierto). Bit n = pin n. */
static volatile uint64_t s_opendrain = 0;

int ch579_pin_is_opendrain(pin_size_t pin)
{
    return (s_opendrain >> pin) & 1U;
}

static void gpio_mode(pin_size_t pin, GPIOModeTypeDef mode)
{
    uint32_t m = ch579_pin_mask(pin);
    if (ch579_pin_portb(pin)) GPIOB_ModeCfg(m, mode);
    else                      GPIOA_ModeCfg(m, mode);
}

static inline void gpio_write(pin_size_t pin, int high)
{
    uint32_t m = ch579_pin_mask(pin);
    if (high) {
        uint32_t primask = __get_PRIMASK();
        __disable_irq();
        if (ch579_pin_portb(pin)) R32_PB_OUT |= m;
        else                      R32_PA_OUT |= m;
        if (!primask) __enable_irq();
    } else {
        if (ch579_pin_portb(pin)) R32_PB_CLR = m;
        else                      R32_PA_CLR = m;
    }
}

void pinMode(pin_size_t pin, PinMode mode)
{
    if (!ch579_pin_valid(pin)) return;

    ch579_pwm_detach(pin);

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    s_opendrain &= ~(1ULL << pin);
    if (mode == OUTPUT_OPENDRAIN) s_opendrain |= (1ULL << pin);
    if (!primask) __enable_irq();

    switch ((int)mode) {
    case INPUT:            gpio_mode(pin, GPIO_ModeIN_Floating); break;
    case INPUT_PULLUP:     gpio_mode(pin, GPIO_ModeIN_PU);       break;
    case INPUT_PULLDOWN:   gpio_mode(pin, GPIO_ModeIN_PD);       break;
    case OUTPUT:           gpio_mode(pin, GPIO_ModeOut_PP_5mA);  break;
    case (int)OUTPUT_20MA: gpio_mode(pin, GPIO_ModeOut_PP_20mA); break;
    case OUTPUT_OPENDRAIN:
        /* Liberado (alto) = entrada flotante; LOW = salida a 0 */
        gpio_write(pin, 0);
        gpio_mode(pin, GPIO_ModeIN_Floating);
        break;
    default: break;
    }
}

void digitalWrite(pin_size_t pin, PinStatus val)
{
    if (!ch579_pin_valid(pin)) return;

    ch579_pwm_detach(pin);

    if (ch579_pin_is_opendrain(pin)) {
        if (val == LOW) {
            gpio_write(pin, 0);
            gpio_mode(pin, GPIO_ModeOut_PP_5mA);
        } else {
            gpio_mode(pin, GPIO_ModeIN_Floating);
        }
        return;
    }
    gpio_write(pin, val != LOW);
}

PinStatus digitalRead(pin_size_t pin)
{
    if (!ch579_pin_valid(pin)) return LOW;
    uint32_t m = ch579_pin_mask(pin);
    uint32_t v = ch579_pin_portb(pin) ? R32_PB_PIN : R32_PA_PIN;
    return (v & m) ? HIGH : LOW;
}
