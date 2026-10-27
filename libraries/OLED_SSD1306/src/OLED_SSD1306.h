/*
 * OLED_SSD1306.h - OLED SSD1306 128x64 por I2C (Wire).
 * Portado de Hal_CH57xx_SSD1306 (proyectos Keil de la CH579M).
 *
 * Trabaja por "paginas" (bandas de 8 px): 8 paginas x 128 columnas.
 * Con la fuente 5x7 caben 21 caracteres por linea y 8 lineas.
 * La fuente solo tiene mayusculas (las minusculas se muestran en mayuscula),
 * digitos y . , : ! ? - / = +
 *
 *   OLED_SSD1306 oled;        // direccion 0x3C (o 0x3D)
 *   Wire.setPins(PA1, PA2);   // SDA, SCL
 *   Wire.begin();
 *   oled.begin();
 *   oled.clear();
 *   oled.setCursor(0, 0);     // pagina, columna
 *   oled.print("HOLA");
 */
#ifndef OLED_SSD1306_H
#define OLED_SSD1306_H

#include <Arduino.h>
#include <Wire.h>

#define SSD1306_WIDTH   128
#define SSD1306_PAGES   8

class OLED_SSD1306 : public Print
{
public:
    explicit OLED_SSD1306(uint8_t address = 0x3C, TwoWire &wire = Wire)
        : _addr(address), _wire(&wire) { }

    void begin(void);
    void clear(void);
    void setCursor(uint8_t page, uint8_t col);
    void setContrast(uint8_t value);
    void display(bool on);                 /* false = pantalla apagada (bajo consumo) */
    void invertDisplay(bool invert);

    /* Barra horizontal de "length" columnas (0..128) rellenas, de
     * "pageHeight" paginas de alto, empezando en la pagina pageStart. */
    void drawBarGraph(uint8_t pageStart, uint8_t pageHeight, uint16_t length);

    /* Envia columnas en bruto en la posicion actual (1 byte = 8 px verticales) */
    void drawColumns(const uint8_t *data, size_t len);

    size_t write(uint8_t c) override;      /* '\n' pasa a la pagina siguiente */
    using Print::write;

private:
    void cmd(uint8_t c);

    uint8_t  _addr;
    TwoWire *_wire;
    uint8_t  _page = 0;
};

#endif
