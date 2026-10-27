/*
 * USBCDC.h - Puerto serie virtual por el USB nativo del CH579 (SerialUSB)
 *
 * Dispositivo USB 2.0 Full Speed de clase CDC-ACM. En Windows 10/11, Linux y
 * macOS aparece como puerto COM sin instalar drivers. En la placa CH579M V1.6
 * es el segundo micro-USB (PB10 = D-, PB11 = D+), no el del CH340.
 *
 *   SerialUSB.begin();               // el baudrate no importa en USB
 *   while (!SerialUSB) { }           // espera a que se abra el puerto en el PC
 *   SerialUSB.println("Hola por USB");
 *
 * Notas:
 *  - El USB necesita el PLL (480 MHz / 10 = 48 MHz): begin() lo enciende.
 *  - Use el reloj de cristal (HSE o PLL): el RC interno no tiene la
 *    precision que exige el USB.
 *  - Si el PC no tiene el puerto abierto, lo que se escribe se descarta
 *    (no bloquea el programa).
 */
#ifndef CH579_USBCDC_H
#define CH579_USBCDC_H

#include "api/HardwareSerial.h"

#ifndef USBCDC_RX_BUFFER_SIZE
#define USBCDC_RX_BUFFER_SIZE 256
#endif
#ifndef USBCDC_TX_BUFFER_SIZE
#define USBCDC_TX_BUFFER_SIZE 256
#endif

class USBCDC : public arduino::HardwareSerial
{
public:
    void begin(unsigned long baud = 115200) override { (void)baud; start(); }
    void begin(unsigned long baud, uint16_t config) override { (void)config; begin(baud); }
    void end() override;

    int available(void) override;
    int availableForWrite(void) override;
    int peek(void) override;
    int read(void) override;
    void flush(void) override;
    size_t write(uint8_t c) override { return write(&c, 1); }
    size_t write(const uint8_t *buf, size_t len) override;
    using Print::write;

    /* true cuando el PC configuro el dispositivo y abrio el puerto (DTR) */
    operator bool() override { return _configured && _dtr; }

    bool     configured(void) const { return _configured; }
    bool     dtr(void) const { return _dtr; }
    bool     rts(void) const { return _rts; }
    uint32_t baud(void) const;               /* el que eligio el programa del PC */

    /* Tiempo maximo (ms) que write() espera si el buffer esta lleno */
    void setWriteTimeout(uint32_t ms) { _writeTimeout = ms; }

    /* Uso interno (rutina de interrupcion USB) */
    void _irq(void);

private:
    void start(void);
    void handleSetup(void);
    void ep0In(void);
    void ep0Out(void);
    void resetEndpoints(void);
    void txKick(void);                       /* llamar con la IRQ USB bloqueada */
    void rxResume(void);

    volatile bool _configured = false;
    volatile bool _dtr = false, _rts = false;
    bool     _started = false;
    uint32_t _writeTimeout = 50;

    /* EP0 */
    uint8_t        _setupReq = 0;
    uint16_t       _setupLen = 0;
    const uint8_t *_descr = nullptr;
    uint8_t        _address = 0;
    uint8_t        _config = 0;

    /* Datos */
    volatile uint16_t _rxHead = 0, _rxTail = 0;
    volatile bool     _rxPaused = false;
    uint8_t           _rxBuf[USBCDC_RX_BUFFER_SIZE];
    volatile uint16_t _txHead = 0, _txTail = 0;
    volatile bool     _txBusy = false;
    volatile uint8_t  _txLastLen = 0;
    uint8_t           _txBuf[USBCDC_TX_BUFFER_SIZE];
};

extern USBCDC SerialUSB;

#endif
