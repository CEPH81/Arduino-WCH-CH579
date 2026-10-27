/*
 * Wire.cpp - I2C maestro por software para WCH CH579
 */
#include "Wire.h"

TwoWire Wire(PIN_WIRE_SDA, PIN_WIRE_SCL);

TwoWire::TwoWire(pin_size_t sda, pin_size_t scl)
    : _sda(sda), _scl(scl), _sdaMask(0), _sclMask(0), _sdaB(false), _sclB(false),
      _half_us(5), _timeout_us(25000), _busHeld(false), _timedOut(false),
      _txAddr(0), _txLen(0), _txActive(false), _rxLen(0), _rxPos(0),
      _onReceive(nullptr), _onRequest(nullptr)
{
}

/* ---------------------------------------------------------- primitivas
 * "Bajo"  : salida con el latch en 0
 * "Suelto": entrada con pull-up (la linea la sube la resistencia)
 */
inline void TwoWire::sdaLow(void)     { if (_sdaB) R32_PB_DIR |= _sdaMask;  else R32_PA_DIR |= _sdaMask; }
inline void TwoWire::sdaRelease(void) { if (_sdaB) R32_PB_DIR &= ~_sdaMask; else R32_PA_DIR &= ~_sdaMask; }
inline void TwoWire::sclLow(void)     { if (_sclB) R32_PB_DIR |= _sclMask;  else R32_PA_DIR |= _sclMask; }
inline bool TwoWire::sdaRead(void)    { return ((_sdaB ? R32_PB_PIN : R32_PA_PIN) & _sdaMask) != 0; }
inline void TwoWire::halfDelay(void)  { delayMicroseconds(_half_us); }

bool TwoWire::sclRelease(void)
{
    if (_sclB) R32_PB_DIR &= ~_sclMask; else R32_PA_DIR &= ~_sclMask;
    /* clock stretching: el esclavo puede mantener SCL en bajo */
    uint32_t t0 = micros();
    while (!(((_sclB ? R32_PB_PIN : R32_PA_PIN) & _sclMask))) {
        if (micros() - t0 > _timeout_us) { _timedOut = true; return false; }
    }
    return true;
}

static void cfg_line(pin_size_t pin, uint32_t *mask, bool *portb)
{
    *mask  = 1UL << g_pinDescription[pin].bit;
    *portb = g_pinDescription[pin].port != 0;
    /* pull-up interno + latch de salida a 0, dejando el pin como entrada */
    if (*portb) { R32_PB_CLR = *mask; GPIOB_ModeCfg(*mask, GPIO_ModeIN_PU); }
    else        { R32_PA_CLR = *mask; GPIOA_ModeCfg(*mask, GPIO_ModeIN_PU); }
}

/* ------------------------------------------------------------ control */
void TwoWire::setPins(pin_size_t sda, pin_size_t scl)
{
    _sda = sda;
    _scl = scl;
}

void TwoWire::begin()
{
    cfg_line(_sda, &_sdaMask, &_sdaB);
    cfg_line(_scl, &_sclMask, &_sclB);
    _busHeld = false;

    /* Si un esclavo quedo a medias reteniendo SDA, 9 pulsos de reloj lo liberan */
    if (!sdaRead()) {
        for (int i = 0; i < 9 && !sdaRead(); i++) {
            sclLow();   halfDelay();
            sclRelease(); halfDelay();
        }
        stop();
    }
}

void TwoWire::begin(uint8_t address)
{
    (void)address;   /* modo esclavo no implementado */
    begin();
}

void TwoWire::end()
{
    if (_busHeld) stop();
    pinMode(_sda, INPUT);
    pinMode(_scl, INPUT);
}

void TwoWire::setClock(uint32_t freq)
{
    if (freq == 0) freq = 100000;
    uint32_t half = 500000UL / freq;    /* medio periodo en us */
    if (half < 1) half = 1;
    _half_us = (uint16_t)half;
}

/* ------------------------------------------------------------- bus I2C */
bool TwoWire::start(void)
{
    _timedOut = false;
    /* START o START repetido: SDA baja mientras SCL esta alto */
    sdaRelease();
    if (!sclRelease()) return false;
    halfDelay();
    sdaLow();
    halfDelay();
    sclLow();
    return true;
}

void TwoWire::stop(void)
{
    sdaLow();
    halfDelay();
    sclRelease();
    halfDelay();
    sdaRelease();
    halfDelay();
    _busHeld = false;
}

bool TwoWire::writeByte(uint8_t b)
{
    for (uint8_t i = 0; i < 8; i++) {
        if (b & 0x80) sdaRelease(); else sdaLow();
        b <<= 1;
        halfDelay();
        if (!sclRelease()) return false;
        halfDelay();
        sclLow();
    }
    /* noveno pulso: el esclavo responde ACK (SDA en bajo) */
    sdaRelease();
    halfDelay();
    if (!sclRelease()) return false;
    bool ack = !sdaRead();
    halfDelay();
    sclLow();
    return ack;
}

uint8_t TwoWire::readByte(bool ack)
{
    uint8_t b = 0;
    sdaRelease();
    for (uint8_t i = 0; i < 8; i++) {
        halfDelay();
        if (!sclRelease()) return 0xFF;
        b = (uint8_t)((b << 1) | (sdaRead() ? 1 : 0));
        halfDelay();
        sclLow();
    }
    if (ack) sdaLow(); else sdaRelease();
    halfDelay();
    sclRelease();
    halfDelay();
    sclLow();
    sdaRelease();
    return b;
}

/* ---------------------------------------------------------- escritura */
void TwoWire::beginTransmission(uint8_t address)
{
    _txAddr = address;
    _txLen = 0;
    _txActive = true;
}

size_t TwoWire::write(uint8_t data)
{
    if (!_txActive || _txLen >= WIRE_BUFFER_LENGTH) return 0;
    _txBuf[_txLen++] = data;
    return 1;
}

size_t TwoWire::write(const uint8_t *data, size_t len)
{
    size_t n = 0;
    while (n < len && write(data[n])) n++;
    return n;
}

/* Codigos de Arduino: 0 ok, 1 dato demasiado largo, 2 NACK en direccion,
 * 3 NACK en dato, 4 otro error, 5 tiempo agotado */
uint8_t TwoWire::endTransmission(bool stopBit)
{
    if (!_txActive) return 4;
    _txActive = false;

    uint8_t result = 0;
    if (!start()) return 5;
    if (!writeByte((uint8_t)(_txAddr << 1))) {
        result = _timedOut ? 5 : 2;
    } else {
        for (size_t i = 0; i < _txLen; i++) {
            if (!writeByte(_txBuf[i])) { result = _timedOut ? 5 : 3; break; }
        }
    }

    if (stopBit || result != 0) stop();
    else _busHeld = true;
    return result;
}

/* ------------------------------------------------------------- lectura */
size_t TwoWire::requestFrom(uint8_t address, size_t len, bool stopBit)
{
    _rxLen = _rxPos = 0;
    if (len == 0) return 0;
    if (len > WIRE_BUFFER_LENGTH) len = WIRE_BUFFER_LENGTH;

    if (!start()) return 0;
    if (!writeByte((uint8_t)((address << 1) | 1))) {
        stop();
        return 0;
    }
    for (size_t i = 0; i < len; i++) {
        _rxBuf[i] = readByte(i + 1 < len);   /* NACK en el ultimo byte */
        if (_timedOut) { stop(); return 0; }
    }
    _rxLen = len;

    if (stopBit) stop();
    else _busHeld = true;
    return len;
}

int TwoWire::available(void) { return (int)(_rxLen - _rxPos); }

int TwoWire::read(void)
{
    if (_rxPos >= _rxLen) return -1;
    return _rxBuf[_rxPos++];
}

int TwoWire::peek(void)
{
    if (_rxPos >= _rxLen) return -1;
    return _rxBuf[_rxPos];
}
