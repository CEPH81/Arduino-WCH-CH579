/*
 * WInterrupts.c - attachInterrupt() / detachInterrupt()
 *
 * El CH579 tiene interrupcion en PA0..PA15 y PB0..PB15, todas compartiendo
 * una sola IRQ (GPIO_IRQn). PB16..PB23 no generan interrupcion.
 * El hardware solo detecta un flanco o un nivel; CHANGE se emula
 * invirtiendo el flanco activo en cada interrupcion.
 */
#include "Arduino.h"
#include "ch579_pins.h"

typedef struct {
    voidFuncPtrParam cb;
    void            *param;
    uint8_t          change;   /* 1 = modo CHANGE emulado */
} irq_slot_t;

static irq_slot_t s_slots[32];   /* 0..15 = PA0..PA15, 16..31 = PB0..PB15 */

static void call_plain(void *param)
{
    voidFuncPtr f = (voidFuncPtr)param;
    if (f) f();
}

static inline int slot_of(pin_size_t pin)
{
    uint8_t bit = g_pinDescription[pin].bit;
    if (bit > 15) return -1;
    return g_pinDescription[pin].port * 16 + bit;
}

/* Elige el flanco opuesto al nivel actual (para CHANGE) */
static inline void set_edge_for_level(int portb, uint32_t mask)
{
    uint32_t level = portb ? (R32_PB_PIN & mask) : (R32_PA_PIN & mask);
    if (level) {    /* ahora en alto -> esperar flanco de bajada */
        if (portb) R32_PB_CLR = mask; else R32_PA_CLR = mask;
    } else {        /* ahora en bajo -> esperar flanco de subida */
        if (portb) R32_PB_OUT |= mask; else R32_PA_OUT |= mask;
    }
}

void attachInterruptParam(pin_size_t pin, voidFuncPtrParam cb, PinStatus mode, void *param)
{
    if (!ch579_pin_valid(pin) || cb == NULL) return;
    int s = slot_of(pin);
    if (s < 0) return;

    int portb = ch579_pin_portb(pin);
    uint32_t mask = ch579_pin_mask(pin);
    GPIOITModeTpDef it;
    uint8_t change = 0;

    switch (mode) {
    case LOW:     it = GPIO_ITMode_LowLevel;  break;
    case HIGH:    it = GPIO_ITMode_HighLevel; break;
    case FALLING: it = GPIO_ITMode_FallEdge;  break;
    case RISING:  it = GPIO_ITMode_RiseEdge;  break;
    case CHANGE:
        change = 1;
        it = digitalRead(pin) ? GPIO_ITMode_FallEdge : GPIO_ITMode_RiseEdge;
        break;
    default: return;
    }

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    s_slots[s].cb     = cb;
    s_slots[s].param  = param;
    s_slots[s].change = change;
    if (portb) GPIOB_ITModeCfg(mask, it);
    else       GPIOA_ITModeCfg(mask, it);
    if (!primask) __enable_irq();

    NVIC_EnableIRQ(GPIO_IRQn);
}

void attachInterrupt(pin_size_t pin, voidFuncPtr cb, PinStatus mode)
{
    attachInterruptParam(pin, call_plain, mode, (void *)cb);
}

void detachInterrupt(pin_size_t pin)
{
    if (!ch579_pin_valid(pin)) return;
    int s = slot_of(pin);
    if (s < 0) return;
    uint16_t mask = (uint16_t)ch579_pin_mask(pin);

    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    if (ch579_pin_portb(pin)) { R16_PB_INT_EN &= ~mask; R16_PB_INT_IF = mask; }
    else                      { R16_PA_INT_EN &= ~mask; R16_PA_INT_IF = mask; }
    s_slots[s].cb = NULL;
    if (!primask) __enable_irq();
}

static void dispatch(int portb, uint16_t flags)
{
    while (flags) {
        int bit = __builtin_ctz(flags);
        flags &= (uint16_t)(flags - 1);
        irq_slot_t *sl = &s_slots[portb * 16 + bit];
        if (sl->change) set_edge_for_level(portb, 1UL << bit);
        if (sl->cb) sl->cb(sl->param);
    }
}

void GPIO_IRQHandler(void)
{
    uint16_t fa = R16_PA_INT_IF & R16_PA_INT_EN;
    uint16_t fb = R16_PB_INT_IF & R16_PB_INT_EN;
    R16_PA_INT_IF = fa;
    R16_PB_INT_IF = fb;
    dispatch(0, fa);
    dispatch(1, fb);
}
