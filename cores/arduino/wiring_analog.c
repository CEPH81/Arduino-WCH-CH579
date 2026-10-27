/*
 * wiring_analog.c - analogRead() con el ADC de 12 bits y analogWrite() con
 * los canales PWM4..PWM11 del CH579.
 */
#include "Arduino.h"
#include "ch579_pins.h"

/* ================================================================= ADC */

static uint8_t  s_adc_ready      = 0;
static uint8_t  s_adc_gain       = AR_DEFAULT;   /* ADC_PGA_0: 0..2.1 V */
static uint8_t  s_adc_resolution = 10;
static int8_t   s_adc_last_ch    = -1;
static int16_t  s_adc_offset     = 0;     /* calibracion gruesa */

static void adc_setup(void)
{
    ADC_ExtSingleChSampInit(SampleFreq_3_2, (ADC_SignalPGATypeDef)s_adc_gain);

    /* Calibracion gruesa del offset (como en el ejemplo oficial de WCH).
     * WCH pide PA5 (AIN1) como entrada flotante durante la medida: se guarda
     * su configuracion, se calibra y se restaura. */
    uint32_t m = 1UL << 5;
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    uint32_t dir = R32_PA_DIR & m, pu = R32_PA_PU & m, pd = R32_PA_PD_DRV & m;
    GPIOA_ModeCfg(m, GPIO_ModeIN_Floating);
    s_adc_offset = ADC_DataCalib_Rough();
    R32_PA_PD_DRV = (R32_PA_PD_DRV & ~m) | pd;
    R32_PA_PU     = (R32_PA_PU & ~m) | pu;
    R32_PA_DIR    = (R32_PA_DIR & ~m) | dir;
    if (!primask) __enable_irq();

    s_adc_last_ch = -1;
    s_adc_ready = 1;
}

void analogCalibrate(void)
{
    adc_setup();
}

void analogReference(uint8_t mode)
{
    if (mode > AR_GAIN_2X) mode = AR_DEFAULT;
    s_adc_gain = mode;
    s_adc_ready = 0;          /* se reconfigura en la siguiente lectura */
}

void analogReadResolution(int bits)
{
    if (bits < 1)  bits = 1;
    if (bits > 12) bits = 12;
    s_adc_resolution = (uint8_t)bits;
}

/* Lectura de 12 bits ya corregida con el offset de calibracion */
static int adc_read_raw12(pin_size_t pin)
{
    if (!ch579_pin_valid(pin)) return -1;
    int8_t ch = g_pinDescription[pin].adc_channel;
    if (ch < 0) return -1;

    if (!s_adc_ready) adc_setup();

    ch579_pwm_detach(pin);
    GPIOA_ModeCfg(ch579_pin_mask(pin), GPIO_ModeIN_Floating);

    ADC_ChannelCfg(ch);
    if (ch != s_adc_last_ch) {
        (void)ADC_ExcutSingleConver();   /* descarta la primera tras cambiar de canal */
        s_adc_last_ch = ch;
    }
    int v = (int)ADC_ExcutSingleConver() + s_adc_offset;
    if (v < 0)    v = 0;
    if (v > 4095) v = 4095;
    return v;
}

int analogRead(pin_size_t pin)
{
    int v = adc_read_raw12(pin);
    if (v < 0) return 0;
    return v >> (12 - s_adc_resolution);
}

/* Formulas de la tabla 15-2 del datasheet, Vref interna = 1050 mV */
int analogReadMillivolts(pin_size_t pin)
{
    int32_t raw = adc_read_raw12(pin);
    if (raw < 0) return 0;
    int32_t mv;
    switch (s_adc_gain) {
    case AR_GAIN_1_4: mv = raw * 1050 / 512  - 3150; break;
    case AR_GAIN_1_2: mv = raw * 1050 / 1024 - 1050; break;
    case AR_GAIN_2X:  mv = raw * 1050 / 4096 + 525;  break;
    default:          mv = raw * 1050 / 2048;        break;
    }
    return mv < 0 ? 0 : (int)mv;
}

/* ================================================================= PWM */

static uint8_t  s_pwm_ready      = 0;
static uint8_t  s_pwm_active     = 0;    /* bit n = canal PWM(4+n) activo */
static uint8_t  s_pwm_resolution = 8;
static uint32_t s_pwm_freq       = 1000; /* Hz */

static void pwm_apply_clock(void)
{
    /* Fpwm = Fsys / CLOCK_DIV / 256 */
    uint32_t div = GetSysClock() / (256UL * s_pwm_freq);
    if (div < 1)   div = 1;
    if (div > 255) div = 255;
    R8_PWM_CLOCK_DIV = (uint8_t)div;
}

static void pwm_setup(void)
{
    pwm_apply_clock();
    PWMX_CycleCfg(PWMX_Cycle_256);
    s_pwm_ready = 1;
}

void analogWriteFrequency(uint32_t hz)
{
    if (hz == 0) hz = 1;
    s_pwm_freq = hz;
    if (s_pwm_ready) pwm_apply_clock();
}

void analogWriteResolution(int bits)
{
    if (bits < 1)  bits = 1;
    if (bits > 16) bits = 16;
    s_pwm_resolution = (uint8_t)bits;
}

void ch579_pwm_detach(pin_size_t pin)
{
    if (!ch579_pin_valid(pin)) return;
    int8_t ch = g_pinDescription[pin].pwm_channel;
    if (ch < 0) return;
    uint8_t bit = (uint8_t)(1U << (ch - 4));
    if (s_pwm_active & bit) {
        R8_PWM_OUT_EN &= ~bit;
        s_pwm_active &= ~bit;
    }
}

void analogWrite(pin_size_t pin, int value)
{
    if (!ch579_pin_valid(pin)) return;

    uint32_t maxval = (1UL << s_pwm_resolution) - 1UL;
    if (value < 0) value = 0;
    if ((uint32_t)value > maxval) value = (int)maxval;

    int8_t ch = g_pinDescription[pin].pwm_channel;
    if (ch < 0) {
        /* Sin PWM por hardware: comportamiento clasico de Arduino */
        pinMode(pin, OUTPUT);
        digitalWrite(pin, ((uint32_t)value * 2UL > maxval) ? HIGH : LOW);
        return;
    }

    /* Escala a 8 bits (0..255) */
    uint32_t duty = ((uint32_t)value * 255UL + maxval / 2UL) / maxval;

    if (duty == 0 || duty >= 255) {
        /* 0 % y 100 % exactos como salida digital */
        pinMode(pin, OUTPUT);
        digitalWrite(pin, duty ? HIGH : LOW);
        return;
    }

    if (!s_pwm_ready) pwm_setup();

    uint8_t bit = (uint8_t)(1U << (ch - 4));
    if (!(s_pwm_active & bit)) {
        if (ch579_pin_portb(pin)) GPIOB_ModeCfg(ch579_pin_mask(pin), GPIO_ModeOut_PP_5mA);
        else                      GPIOA_ModeCfg(ch579_pin_mask(pin), GPIO_ModeOut_PP_5mA);
        PWMX_ACTOUT(bit, (uint8_t)duty, High_Level, ENABLE);
        s_pwm_active |= bit;
    } else {
        *((volatile uint8_t *)(&R8_PWM4_DATA) + (ch - 4)) = (uint8_t)duty;
    }
}
