/*
 * SPI.h - SPI maestro por hardware para WCH CH579
 *
 *   SPI  -> SPI0: SCK=PA13 MOSI=PA14 MISO=PA15 (SS=PA12, manejado por el sketch)
 *   SPI1 -> SPI1: SCK=PA0  MOSI=PA1  MISO=PA2
 *
 * Limitacion del hardware: solo existen los modos SPI 0 y 3. Si se pide el
 * modo 1 se usa el 0, y si se pide el 2 se usa el 3 (misma polaridad de
 * reloj, distinta fase): compruebe que su dispositivo lo admite.
 */
#ifndef CH579_SPI_H
#define CH579_SPI_H

#include <Arduino.h>
#include "api/HardwareSPI.h"

/* La API de Arduino define arduino::SPIClass (= HardwareSPI); las librerias
 * que reciben un "SPIClass &" aceptan SPI y SPI1 sin cambios. */
class CH579SPI : public arduino::HardwareSPI
{
public:
    CH579SPI(uint8_t spi, pin_size_t sck, pin_size_t mosi, pin_size_t miso, int ss);

    void begin() override;
    void end() override;

    void beginTransaction(arduino::SPISettings settings) override;
    void endTransaction(void) override;

    uint8_t  transfer(uint8_t data) override;
    uint16_t transfer16(uint16_t data) override;
    void     transfer(void *buf, size_t count) override;
    void     transfer(const void *txbuf, void *rxbuf, size_t count);

    void usingInterrupt(int) override { }
    void notUsingInterrupt(int) override { }
    void attachInterrupt() override { }
    void detachInterrupt() override { }

    /* Compatibilidad con la API clasica de AVR */
    void setBitOrder(BitOrder order);
    void setDataMode(uint8_t mode);
    void setClockDivider(uint8_t div);   /* divisor directo de Fsys (2..255) */

private:
    void apply(uint32_t clock, BitOrder order, uint8_t mode);

    volatile uint8_t *_base;
    pin_size_t _sck, _mosi, _miso;
    int        _ss;
    bool       _lsbfirst;
};

extern CH579SPI SPI;
extern CH579SPI SPI1;

#endif
