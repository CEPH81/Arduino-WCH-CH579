/*
 * OLED_SSD1306.cpp - Portado de Hal_CH57xx_SSD1306.c
 *
 * Cambio respecto al original: los datos se envian en bloques de hasta
 * 32 bytes (Wire tiene un buffer de 64), en lugar de 129 bytes en una sola
 * transaccion. En modo de direccionamiento horizontal la columna avanza
 * sola, asi que el resultado en pantalla es el mismo.
 */
#include "OLED_SSD1306.h"
#include "OLED_Font5x7.h"

#define OLED_CHUNK 32

void OLED_SSD1306::cmd(uint8_t c)
{
    _wire->beginTransmission(_addr);
    _wire->write((uint8_t)0x00);     /* byte de control: comando */
    _wire->write(c);
    _wire->endTransmission();
}

void OLED_SSD1306::drawColumns(const uint8_t *data, size_t len)
{
    while (len > 0) {
        size_t n = len > OLED_CHUNK ? OLED_CHUNK : len;
        _wire->beginTransmission(_addr);
        _wire->write((uint8_t)0x40); /* byte de control: datos */
        _wire->write(data, n);
        _wire->endTransmission();
        data += n;
        len -= n;
    }
}

void OLED_SSD1306::begin(void)
{
    static const uint8_t init_seq[] = {
        0xAE,             /* display off */
        0xD5, 0x80,       /* divisor de reloj */
        0xA8, 0x3F,       /* multiplex: 64 */
        0xD3, 0x00,       /* offset: 0 */
        0x40,             /* linea de inicio: 0 */
        0x8D, 0x14,       /* charge pump: activado */
        0x20, 0x00,       /* direccionamiento horizontal */
        0xA1,             /* remapeo de segmentos */
        0xC8,             /* barrido COM remapeado */
        0xDA, 0x12,       /* configuracion de pines COM */
        0x81, 0xCF,       /* contraste */
        0xD9, 0xF1,       /* precarga */
        0xDB, 0x40,       /* VCOMH */
        0xA4,             /* mostrar desde la RAM */
        0xA6,             /* normal (no invertido) */
        0xAF              /* display on */
    };
    for (size_t i = 0; i < sizeof(init_seq); i++) cmd(init_seq[i]);
}

void OLED_SSD1306::setCursor(uint8_t page, uint8_t col)
{
    _page = page & 7;
    cmd((uint8_t)(0xB0 + _page));
    cmd((uint8_t)(0x00 + (col & 0x0F)));
    cmd((uint8_t)(0x10 + (col >> 4)));
}

void OLED_SSD1306::clear(void)
{
    uint8_t zeros[SSD1306_WIDTH];
    memset(zeros, 0, sizeof(zeros));
    for (uint8_t p = 0; p < SSD1306_PAGES; p++) {
        setCursor(p, 0);
        drawColumns(zeros, SSD1306_WIDTH);
    }
    setCursor(0, 0);
}

size_t OLED_SSD1306::write(uint8_t c)
{
    if (c == '\r') return 1;
    if (c == '\n') { setCursor((uint8_t)(_page + 1), 0); return 1; }

    const uint8_t *glyph;
    Font5x7_GetGlyph((char)c, &glyph);
    uint8_t buf[6];
    memcpy(buf, glyph, 5);
    buf[5] = 0x00;                    /* columna de separacion */
    drawColumns(buf, 6);
    return 1;
}

void OLED_SSD1306::setContrast(uint8_t value) { cmd(0x81); cmd(value); }
void OLED_SSD1306::display(bool on)           { cmd(on ? 0xAF : 0xAE); }
void OLED_SSD1306::invertDisplay(bool inv)    { cmd(inv ? 0xA7 : 0xA6); }

void OLED_SSD1306::drawBarGraph(uint8_t pageStart, uint8_t pageHeight, uint16_t length)
{
    uint8_t row[SSD1306_WIDTH];
    if (length > SSD1306_WIDTH) length = SSD1306_WIDTH;
    for (uint16_t x = 0; x < SSD1306_WIDTH; x++) row[x] = (x < length) ? 0xFF : 0x00;
    for (uint8_t p = 0; p < pageHeight; p++) {
        setCursor((uint8_t)(pageStart + p), 0);
        drawColumns(row, SSD1306_WIDTH);
    }
}
