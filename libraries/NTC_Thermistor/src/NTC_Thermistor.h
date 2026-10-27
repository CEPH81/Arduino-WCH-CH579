/*
 * NTC_Thermistor.h - Termistor NTC con la ecuacion Beta.
 * Portado de Hal_CH57xx_NTC (proyectos Keil de la CH579M).
 *
 * Cambio importante respecto al original: el original convertia la lectura
 * con Vcc * raw / 4095, como si el ADC midiera de 0 a 3.3 V. Con la ganancia
 * por defecto el ADC del CH579 mide de 0 a ~2.1 V (raw/2048 * 1.05 V,
 * datasheet tabla 15-2), asi que aquí se usa analogReadMillivolts(), que
 * aplica la formula correcta. Las temperaturas cambian respecto a la
 * version de Keil y deberian ser mas exactas.
 *
 * Limite: el ADC satura por encima de ~2.1 V. Con el divisor del original
 * (3V3 -> NTC -> pin -> R fija -> GND) eso ocurre a unos 38 C con un NTC de
 * 10k/3950. Si necesita medir mas caliente, invierta el divisor
 * (3V3 -> R fija -> pin -> NTC -> GND) y use NTC_LOW_SIDE.
 */
#ifndef NTC_THERMISTOR_H
#define NTC_THERMISTOR_H

#include <Arduino.h>

enum NTC_Topology {
    NTC_HIGH_SIDE,   /* 3V3 -> NTC -> pin -> R fija -> GND   (como el original) */
    NTC_LOW_SIDE     /* 3V3 -> R fija -> pin -> NTC -> GND */
};

class NTC_Thermistor
{
public:
    NTC_Thermistor(pin_size_t pin, float rFixed = 10000.0f, float r0 = 10000.0f,
                   float beta = 3950.0f, float vcc = 3.3f, NTC_Topology topo = NTC_HIGH_SIDE)
        : _pin(pin), _rFixed(rFixed), _r0(r0), _beta(beta), _vccMv(vcc * 1000.0f), _topo(topo) { }

    void  begin(void) { pinMode(_pin, INPUT); }
    float readCelsius(uint8_t samples = 32);   /* promedia 'samples' lecturas; NAN si satura */
    float readResistance(uint8_t samples = 32);

private:
    pin_size_t   _pin;
    float        _rFixed, _r0, _beta, _vccMv;
    NTC_Topology _topo;
};

#endif
