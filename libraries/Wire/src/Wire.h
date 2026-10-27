/*
 * Wire.h - I2C maestro por software para WCH CH579
 *
 * El CH579 no tiene controlador I2C, asi que el bus se genera por
 * software. Pines por defecto: SDA=PB21, SCL=PB20 (cambiables con
 * Wire.setPins(sda, scl) antes de Wire.begin()).
 *
 * Las lineas se manejan como drenador abierto: en "1" el pin queda como
 * entrada con pull-up interno; ponga pull-ups externos de 4.7 kOhm para
 * 100 kHz o mas.
 *
 * Solo modo maestro: begin(direccion), onReceive() y onRequest() se aceptan
 * por compatibilidad pero el modo esclavo no esta implementado.
 */
#ifndef CH579_WIRE_H
#define CH579_WIRE_H

#include <Arduino.h>
#include "api/HardwareI2C.h"

#ifndef WIRE_BUFFER_LENGTH
#define WIRE_BUFFER_LENGTH 64
#endif
#define BUFFER_LENGTH WIRE_BUFFER_LENGTH

class TwoWire : public arduino::HardwareI2C
{
public:
    TwoWire(pin_size_t sda, pin_size_t scl);

    void setPins(pin_size_t sda, pin_size_t scl);
    void begin() override;
    void begin(uint8_t address) override;
    void end() override;
    void setClock(uint32_t freq) override;
    void setTimeout(uint32_t us) { _timeout_us = us; }

    void beginTransmission(uint8_t address) override;
    uint8_t endTransmission(bool stopBit) override;
    uint8_t endTransmission(void) override { return endTransmission(true); }

    size_t requestFrom(uint8_t address, size_t len, bool stopBit) override;
    size_t requestFrom(uint8_t address, size_t len) override { return requestFrom(address, len, true); }

    size_t write(uint8_t data) override;
    size_t write(const uint8_t *data, size_t len) override;
    using Print::write;

    int available(void) override;
    int read(void) override;
    int peek(void) override;
    void flush(void) override { }

    void onReceive(void (*cb)(int)) override { _onReceive = cb; }
    void onRequest(void (*cb)(void)) override { _onRequest = cb; }

private:
    /* primitivas del bus */
    inline void sdaLow(void);
    inline void sdaRelease(void);
    inline void sclLow(void);
    bool sclRelease(void);          /* espera clock stretching */
    inline bool sdaRead(void);
    inline void halfDelay(void);
    bool start(void);
    void stop(void);
    bool writeByte(uint8_t b);      /* true = ACK */
    uint8_t readByte(bool ack);

    pin_size_t _sda, _scl;
    uint32_t   _sdaMask, _sclMask;
    bool       _sdaB, _sclB;
    uint16_t   _half_us;
    uint32_t   _timeout_us;
    bool       _busHeld;            /* sin STOP tras la ultima transaccion */
    bool       _timedOut;

    uint8_t _txAddr;
    uint8_t _txBuf[WIRE_BUFFER_LENGTH];
    size_t  _txLen;
    bool    _txActive;
    uint8_t _rxBuf[WIRE_BUFFER_LENGTH];
    size_t  _rxLen, _rxPos;

    void (*_onReceive)(int);
    void (*_onRequest)(void);
};

extern TwoWire Wire;

#endif
