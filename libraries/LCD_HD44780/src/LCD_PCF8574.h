/*
 * LCD_PCF8574.h - LCD HD44780 con modulo I2C PCF8574 (backpack), via Wire.
 *
 *   LCD_PCF8574 lcd(0x26);        // direccion del PCF8574 (0x27, 0x3F...)
 *   Wire.setPins(PA1, PA2);   // SDA, SCL (opcional)
 *   Wire.begin();
 *   lcd.begin();
 *   lcd.print("Hola");
 */
#ifndef LCD_PCF8574_H
#define LCD_PCF8574_H

#include "LCD_HD44780.h"
#include <Wire.h>

class LCD_PCF8574 : public HD44780
{
public:
    explicit LCD_PCF8574(uint8_t address, TwoWire &wire = Wire) : _addr(address), _wire(&wire) { }
    void begin(void) { init(); }

protected:
    void writeRaw(uint8_t data) override
    {
        _wire->beginTransmission(_addr);
        _wire->write(data);
        _wire->endTransmission();
    }

private:
    uint8_t  _addr;
    TwoWire *_wire;
};

#endif
