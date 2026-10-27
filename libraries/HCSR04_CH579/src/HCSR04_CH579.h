/*
 * HCSR04_CH579.h - Sensor ultrasonico HC-SR04.
 * Portado de Hal_CH57xx_HCSR04 (proyectos Keil de la CH579M).
 *
 * Diferencia con el original: el ancho del pulso Echo se mide con micros()
 * (SysTick) en lugar de dejar TMR0 corriendo libre, asi que TMR0 queda
 * disponible para otros usos. Se mantiene el "blanking" de 300 us tras el
 * disparo, que evito lecturas falsas en la placa.
 *
 *   HCSR04 sensor(PB2, PA3);       // Trig, Echo
 *   sensor.begin();
 *   uint16_t cm = sensor.readDistanceCm();   // HCSR04_NO_ECHO si no hubo eco
 *
 * Echo de un modulo a 5 V: use un divisor 1k/2k (el CH579 trabaja a 3.3 V).
 */
#ifndef HCSR04_CH579_H
#define HCSR04_CH579_H

#include <Arduino.h>

#define HCSR04_NO_ECHO  0xFFFFu

class HCSR04
{
public:
    HCSR04(pin_size_t trigPin, pin_size_t echoPin) : _trig(trigPin), _echo(echoPin) { }

    void begin(void);

    /* Distancia en cm, o HCSR04_NO_ECHO si no hubo eco (fuera de rango,
     * sensor desconectado o cableado erroneo). */
    uint16_t readDistanceCm(void);

    /* Ancho del pulso Echo en microsegundos, o 0 si hubo timeout.
     * stage (opcional): 0 = timeout esperando la subida, 1 = esperando la
     * bajada, 2 = medicion correcta. */
    uint32_t readEchoUs(uint8_t *stage = nullptr);

private:
    pin_size_t _trig, _echo;
};

#endif
