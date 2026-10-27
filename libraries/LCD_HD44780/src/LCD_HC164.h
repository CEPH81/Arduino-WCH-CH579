/*
 * LCD_HC164.h - LCD HD44780 a traves de un registro de desplazamiento
 * 74HC164 (2 hilos: DATA y CLK). Portado de Hal_CH57xx_HC164.
 *
 * Mapeo asumido de Q0..Q7 (el mismo que el PCF8574):
 *   Q0=RS  Q1=RW  Q2=E  Q3=luz  Q4..Q7=D4..D7
 */
#ifndef LCD_HC164_H
#define LCD_HC164_H

#include "LCD_HD44780.h"

class LCD_HC164 : public HD44780
{
public:
    LCD_HC164(pin_size_t dataPin, pin_size_t clkPin) : _data(dataPin), _clk(clkPin) { }

    void begin(void)
    {
        digitalWrite(_data, LOW);
        digitalWrite(_clk, LOW);
        pinMode(_data, OUTPUT);
        pinMode(_clk, OUTPUT);
        init();
    }

protected:
    /* MSB primero: el primer bit termina en Q7 y el ultimo en Q0 */
    void writeRaw(uint8_t data) override
    {
        for (int8_t i = 7; i >= 0; i--) {
            digitalWrite(_data, (data & (1 << i)) ? HIGH : LOW);
            delayMicroseconds(2);
            digitalWrite(_clk, HIGH);       /* flanco de subida: el 164 toma el bit */
            delayMicroseconds(2);
            digitalWrite(_clk, LOW);
            delayMicroseconds(2);
        }
    }

private:
    pin_size_t _data, _clk;
};

#endif
