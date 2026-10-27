/*
 * wiring.c - Reloj del sistema, SysTick, millis(), micros() y retardos
 */
#include "Arduino.h"

static volatile uint32_t s_millis = 0;
static uint32_t s_ticks_per_us = F_CPU / 1000000UL;

void SysTick_Handler(void)
{
    s_millis++;
}

uint32_t ch579_get_sysclk(void)
{
    return GetSysClock();
}

/* --------------------------------------------------------------- reloj
 * Al reset el CH579 corre con el RC interno de 32 MHz (HSI). Segun el menu
 * "Reloj del sistema" se pasa al cristal de 32 MHz (HSE) o al PLL.
 * Secuencia validada en la placa CH579M: encender HSE, ajustar carga y
 * corriente del cristal, esperar a que estabilice y conmutar.
 */
static void ch579_clock_init(void)
{
#if defined(CH579_CLOCK_HSE_32MHZ) || defined(CH579_CLOCK_PLL_40MHZ)
    PWR_UnitModCfg(ENABLE, UNIT_SYS_HSE);
    HSECFG_Capacitance(HSECap_18p);
    HSECFG_Current(HSE_RCur_100);
    mDelaymS(5);
  #if defined(CH579_CLOCK_PLL_40MHZ)
    PWR_UnitModCfg(ENABLE, UNIT_SYS_PLL);
    mDelayuS(100);
    SetSysClock(CLK_SOURCE_PLL_40MHz);
  #else
    SetSysClock(CLK_SOURCE_HSE_32MHz);
  #endif
#else
    SetSysClock(CLK_SOURCE_HSI_32MHz);
#endif
    mDelayuS(10);
}

void init(void)
{
    ch579_clock_init();

    uint32_t f = GetSysClock();
    s_ticks_per_us = f / 1000000UL;

    /* SysTick a 1 kHz para millis() */
    SysTick_Config(f / 1000UL);
    NVIC_SetPriority(SysTick_IRQn, 0);

    __enable_irq();
}

/* ------------------------------------------------------------- tiempos */
unsigned long millis(void)
{
    return s_millis;
}

unsigned long micros(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();

    uint32_t ms   = s_millis;
    uint32_t val  = SysTick->VAL;
    uint32_t load = SysTick->LOAD;

    /* Si el SysTick dio la vuelta pero su interrupcion aun no se atendio */
    if ((SCB->ICSR & SCB_ICSR_PENDSTSET_Msk) && val > (load >> 1)) {
        ms++;
    }

    if (!primask) __enable_irq();

    return ms * 1000UL + (load - val) / s_ticks_per_us;
}

void delay(unsigned long ms)
{
    if (ms == 0) return;
    uint32_t start = micros();
    while (ms > 0) {
        yield();
        while (ms > 0 && (micros() - start) >= 1000UL) {
            ms--;
            start += 1000UL;
        }
    }
}

/* Espera activa contando ciclos del SysTick: funciona aun con las
 * interrupciones deshabilitadas. */
void delayMicroseconds(unsigned int us)
{
    if (us == 0) return;
    uint32_t reload    = SysTick->LOAD + 1UL;
    uint32_t remaining = (uint32_t)us * s_ticks_per_us;
    uint32_t prev      = SysTick->VAL;

    while (1) {
        uint32_t now = SysTick->VAL;
        uint32_t elapsed = (prev >= now) ? (prev - now) : (prev + reload - now);
        if (elapsed >= remaining) break;
        remaining -= elapsed;
        prev = now;
    }
}
