/*
 * LCD_HD44780.h - Logica comun de una LCD de caracteres HD44780 en modo 4 bits.
 *
 * Las clases LCD_PCF8574 (PCF8574) y LCD_HC164 solo implementan writeRaw(),
 * que pone un byte en las salidas del expansor con este mapeo de bits:
 *   bit0 = RS   bit1 = RW   bit2 = E   bit3 = luz de fondo   bit4..7 = D4..D7
 */
#ifndef LCD_HD44780_BASE_H
#define LCD_HD44780_BASE_H

#include <Arduino.h>

class HD44780 : public Print
{
public:
    void init(void);                        /* secuencia de arranque HD44780 */
    void clear(void);
    void home(void);
    void setCursor(uint8_t col, uint8_t row);
    void backlight(bool on);
    void display(bool on);
    void command(uint8_t cmd) { send(cmd, false); }

    size_t write(uint8_t c) override { send(c, true); return 1; }
    using Print::write;

protected:
    virtual void writeRaw(uint8_t data) = 0;

private:
    void pulseEnable(uint8_t data);
    void write4bits(uint8_t nibble, bool rs);
    void send(uint8_t value, bool rs);

    uint8_t _backlight = 0x08;
    uint8_t _control   = 0x0C;   /* display ON, cursor OFF, blink OFF */
};

#endif
