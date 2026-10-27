/*
 * LCD_HD44780.cpp - Portado de lcd_init()/lcd_send() de los ejemplos en Keil.
 */
#include "LCD_HD44780.h"

#define LCD_RS      0x01
#define LCD_ENABLE  0x04

void HD44780::pulseEnable(uint8_t data)
{
    writeRaw(data | LCD_ENABLE);
    delayMicroseconds(1);
    writeRaw(data & ~LCD_ENABLE);
    delayMicroseconds(50);
}

/* nibble ya viene en los 4 bits altos (0xF0) */
void HD44780::write4bits(uint8_t nibble, bool rs)
{
    pulseEnable((uint8_t)((nibble & 0xF0) | _backlight | (rs ? LCD_RS : 0)));
}

void HD44780::send(uint8_t value, bool rs)
{
    write4bits(value & 0xF0, rs);
    write4bits((uint8_t)(value << 4), rs);
}

void HD44780::init(void)
{
    delay(50);                          /* encendido de la LCD */
    /* Reset estandar para forzar el modo 4 bits */
    write4bits(0x30, false); delay(5);
    write4bits(0x30, false); delayMicroseconds(150);
    write4bits(0x30, false); delayMicroseconds(150);
    write4bits(0x20, false);            /* modo 4 bits */

    command(0x28);                      /* 4 bits, 2 lineas, 5x8 */
    command(_control);                  /* display ON */
    clear();
    command(0x06);                      /* el cursor avanza, sin desplazar */
}

void HD44780::clear(void)
{
    command(0x01);
    delay(2);                           /* el borrado necesita mas tiempo */
}

void HD44780::home(void)
{
    command(0x02);
    delay(2);
}

void HD44780::setCursor(uint8_t col, uint8_t row)
{
    static const uint8_t row_offset[] = { 0x00, 0x40, 0x14, 0x54 };
    if (row > 3) row = 3;
    command((uint8_t)(0x80 | (col + row_offset[row])));
}

void HD44780::backlight(bool on)
{
    _backlight = on ? 0x08 : 0x00;
    writeRaw(_backlight);
}

void HD44780::display(bool on)
{
    _control = on ? (_control | 0x04) : (_control & ~0x04);
    command(_control);
}
