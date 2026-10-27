/*
 * Arduino.h - Core Arduino para WCH CH579
 */
#ifndef Arduino_h
#define Arduino_h

/* ------------------------------------------------------------------------
 * SDK de WCH. Se incluye primero y despues se retiran las macros que chocan
 * con la API de Arduino (min/max, NULL de C, etc.).
 * ------------------------------------------------------------------------ */
#include "CH57x_common.h"

#undef min
#undef max
#undef NULL
#ifdef __cplusplus
#define NULL __null
#else
#define NULL ((void *)0)
#endif

#include "api/ArduinoAPI.h"

#if defined(__cplusplus) && !defined(c_plusplus)
using namespace arduino;
#endif

#include "variant.h"

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------- utilidades sobre los pines */
#define digitalPinToPort(p)        (g_pinDescription[(p)].port)
#define digitalPinToBitMask(p)     (1UL << g_pinDescription[(p)].bit)
#define digitalPinToInterrupt(p)   (p)
#define digitalPinHasPWM(p)        ((p) < NUM_DIGITAL_PINS && g_pinDescription[(p)].pwm_channel >= 0)
#define analogInputToDigitalPin(p) (p)
#define portOutputRegister(port)   ((port) ? &R32_PB_OUT : &R32_PA_OUT)
#define portInputRegister(port)    ((port) ? &R32_PB_PIN : &R32_PA_PIN)
#define portModeRegister(port)     ((port) ? &R32_PB_DIR : &R32_PA_DIR)

#define clockCyclesPerMicrosecond() (F_CPU / 1000000L)
#define clockCyclesToMicroseconds(a) ((a) / clockCyclesPerMicrosecond())
#define microsecondsToClockCycles(a) ((a) * clockCyclesPerMicrosecond())

/* Modo extra de pinMode(): salida push-pull de 20 mA (OUTPUT = 5 mA) */
#define OUTPUT_20MA ((PinMode)0x10)

#define interrupts()   __enable_irq()
#define noInterrupts() __disable_irq()

/* ------------------------------------------------------- ADC del CH579
 * El ADC es de 12 bits y su rango depende de la ganancia (PGA), con
 * Vref interna ~1.05 V (datasheet, tabla 15-2):
 *   AR_DEFAULT  (0 dB, x1)    0 V .. 2.1 V   <- por defecto
 *   AR_GAIN_2X  (+6 dB, x2)   0.53 V .. 1.57 V
 *   AR_GAIN_1_2 (-6 dB, x1/2) hasta ~3.15 V (preciso de 1.9 V a 3 V)
 *   AR_GAIN_1_4 (-12 dB, x1/4) hasta VIO33   (preciso de 2.9 V a VIO33)
 */
enum {
    AR_GAIN_1_4 = 0,    /* ADC_PGA_1_4 */
    AR_GAIN_1_2 = 1,    /* ADC_PGA_1_2 */
    AR_DEFAULT  = 2,    /* ADC_PGA_0   */
    AR_GAIN_2X  = 3     /* ADC_PGA_2   */
};

void analogReadResolution(int bits);   /* 1..12, por defecto 10 */
void analogWriteResolution(int bits);  /* 1..16, por defecto 8  */
int  analogReadMillivolts(pin_size_t pin); /* tension en mV segun la ganancia */
void analogCalibrate(void);            /* recalibra el offset (PA5 flotante) */

/* PWM por hardware (PWM4..PWM11): 8 bits, frecuencia comun a todos los
 * canales. Por defecto ~1 kHz. */
void analogWriteFrequency(uint32_t hz);

/* Reloj real del sistema en Hz (lee los registros) */
uint32_t ch579_get_sysclk(void);

#ifdef __cplusplus
} /* extern "C" */

/* Puertos serie: UART0..3 y USB nativo (SerialUSB) */
#include "HardwareSerial.h"
#include "USBCDC.h"
#endif

#endif /* Arduino_h */
