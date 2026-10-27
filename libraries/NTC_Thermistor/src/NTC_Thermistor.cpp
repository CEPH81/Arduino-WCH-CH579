#include "NTC_Thermistor.h"
#include <math.h>

#define NTC_T0_KELVIN     298.15f   /* 25 C */
#define NTC_ADC_LIMIT_MV  2080      /* por encima, el ADC (ganancia x1) esta saturado */

float NTC_Thermistor::readResistance(uint8_t samples)
{
    if (samples == 0) samples = 1;
    int32_t sum = 0;
    for (uint8_t i = 0; i < samples; i++) sum += analogReadMillivolts(_pin);
    float mv = (float)sum / samples;

    if (mv <= 0.0f || mv >= NTC_ADC_LIMIT_MV || mv >= _vccMv) return NAN;

    /* Divisor: v = Vcc * Rbajo / (Rarriba + Rbajo) */
    if (_topo == NTC_HIGH_SIDE) return _rFixed * (_vccMv / mv - 1.0f);
    return _rFixed * mv / (_vccMv - mv);
}

float NTC_Thermistor::readCelsius(uint8_t samples)
{
    float r = readResistance(samples);
    if (isnan(r) || r <= 0.0f) return NAN;
    float inv_t = (1.0f / NTC_T0_KELVIN) + (1.0f / _beta) * logf(r / _r0);
    return (1.0f / inv_t) - 273.15f;
}
