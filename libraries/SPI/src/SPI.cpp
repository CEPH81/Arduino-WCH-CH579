/*
 * SPI.cpp - SPI maestro por hardware para WCH CH579
 * Basado en la secuencia de SPI0_MasterDefInit()/SPI0_MasterRecvByte() del SDK.
 */
#include "SPI.h"

/* SPI_CTRL_MOD, SPI_CTRL_CFG, SPI_CLOCK_DIV, SPI_BUFFER y SPI_INT_FLAG son los
 * desplazamientos de registro comunes a SPI0 y SPI1 definidos en CH579SFR.h */

CH579SPI SPI (0, PIN_SPI_SCK,  PIN_SPI_MOSI,  PIN_SPI_MISO,  PIN_SPI_SS);
CH579SPI SPI1(1, PIN_SPI1_SCK, PIN_SPI1_MOSI, PIN_SPI1_MISO, -1);

CH579SPI::CH579SPI(uint8_t spi, pin_size_t sck, pin_size_t mosi, pin_size_t miso, int ss)
    : _base(spi ? BA_SPI1 : BA_SPI0), _sck(sck), _mosi(mosi), _miso(miso), _ss(ss),
      _lsbfirst(false)
{
}

void CH579SPI::begin()
{
    if (_ss >= 0) {
        digitalWrite((pin_size_t)_ss, HIGH);
        pinMode((pin_size_t)_ss, OUTPUT);
    }
    digitalWrite(_sck, LOW);
    pinMode(_sck, OUTPUT);
    pinMode(_mosi, OUTPUT);
    pinMode(_miso, INPUT);

    _base[SPI_CTRL_MOD] = RB_SPI_ALL_CLEAR;
    _base[SPI_CTRL_MOD] = RB_SPI_MOSI_OE | RB_SPI_SCK_OE;      /* maestro, 3 hilos */
    _base[SPI_CTRL_CFG] |= RB_SPI_AUTO_IF;
    _base[SPI_CTRL_CFG] &= ~RB_SPI_DMA_ENABLE;
    apply(4000000, MSBFIRST, SPI_MODE0);
}

void CH579SPI::end()
{
    _base[SPI_CTRL_MOD] = RB_SPI_ALL_CLEAR;
    pinMode(_sck, INPUT);
    pinMode(_mosi, INPUT);
}

void CH579SPI::apply(uint32_t clock, BitOrder order, uint8_t mode)
{
    uint32_t f = GetSysClock();
    uint32_t div = clock ? (f + clock - 1) / clock : 255;   /* redondea hacia abajo la frecuencia */
    if (div < 2)   div = 2;
    if (div > 255) div = 255;

    if (div == 2) _base[SPI_CTRL_CFG] |= RB_SPI_MST_DLY_EN;   /* como SPI0_CLKCfg() */
    else          _base[SPI_CTRL_CFG] &= ~RB_SPI_MST_DLY_EN;
    _base[SPI_CLOCK_DIV] = (uint8_t)div;

    setBitOrder(order);
    setDataMode(mode);
}

void CH579SPI::beginTransaction(arduino::SPISettings s)
{
    apply(s.getClockFreq(), s.getBitOrder(), (uint8_t)s.getDataMode());
}

void CH579SPI::endTransaction(void) { }

void CH579SPI::setBitOrder(BitOrder order)
{
    _lsbfirst = (order == LSBFIRST);
    if (_lsbfirst) _base[SPI_CTRL_CFG] |= RB_SPI_BIT_ORDER;
    else           _base[SPI_CTRL_CFG] &= ~RB_SPI_BIT_ORDER;
}

void CH579SPI::setDataMode(uint8_t mode)
{
    /* CPOL = 1 (modos 2 y 3) -> modo 3 del hardware; CPOL = 0 -> modo 0 */
    if (mode == SPI_MODE2 || mode == SPI_MODE3) _base[SPI_CTRL_MOD] |= RB_SPI_MST_SCK_MOD;
    else                                         _base[SPI_CTRL_MOD] &= ~RB_SPI_MST_SCK_MOD;
}

void CH579SPI::setClockDivider(uint8_t div)
{
    if (div < 2) div = 2;
    if (div == 2) _base[SPI_CTRL_CFG] |= RB_SPI_MST_DLY_EN;
    else          _base[SPI_CTRL_CFG] &= ~RB_SPI_MST_DLY_EN;
    _base[SPI_CLOCK_DIV] = div;
}

uint8_t CH579SPI::transfer(uint8_t data)
{
    _base[SPI_CTRL_MOD] &= ~RB_SPI_FIFO_DIR;       /* sentido: salida (full duplex) */
    _base[SPI_BUFFER] = data;
    while (!(_base[SPI_INT_FLAG] & RB_SPI_FREE)) { }
    return _base[SPI_BUFFER];
}

uint16_t CH579SPI::transfer16(uint16_t data)
{
    uint8_t hi, lo;
    if (_lsbfirst) {
        lo = transfer((uint8_t)data);
        hi = transfer((uint8_t)(data >> 8));
    } else {
        hi = transfer((uint8_t)(data >> 8));
        lo = transfer((uint8_t)data);
    }
    return (uint16_t)((hi << 8) | lo);
}

void CH579SPI::transfer(void *buf, size_t count)
{
    uint8_t *p = (uint8_t *)buf;
    while (count--) { *p = transfer(*p); p++; }
}

void CH579SPI::transfer(const void *txbuf, void *rxbuf, size_t count)
{
    const uint8_t *tx = (const uint8_t *)txbuf;
    uint8_t *rx = (uint8_t *)rxbuf;
    for (size_t i = 0; i < count; i++) {
        uint8_t v = transfer(tx ? tx[i] : 0xFF);
        if (rx) rx[i] = v;
    }
}
