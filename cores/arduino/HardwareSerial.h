/*
 * HardwareSerial.h - UART0..UART3 del CH579
 *
 *   Serial  = UART1  TX=PA9  RX=PA8   (CH340N de la placa)
 *   Serial0 = UART0  TX=PB7  RX=PB4
 *   Serial2 = UART2  TX=PA7  RX=PA6
 *   Serial3 = UART3  TX=PA5  RX=PA4
 *
 * Recepcion por interrupcion con buffer circular; transmision usando la
 * FIFO de 8 bytes del UART (bloquea solo si la FIFO esta llena).
 */
#ifndef CH579_HARDWARE_SERIAL_H
#define CH579_HARDWARE_SERIAL_H

#include "api/HardwareSerial.h"

#ifndef SERIAL_RX_BUFFER_SIZE
#define SERIAL_RX_BUFFER_SIZE 256
#endif

class UARTClass : public arduino::HardwareSerial
{
public:
    UARTClass(uint8_t uart, pin_size_t tx, pin_size_t rx);

    void begin(unsigned long baud) override { begin(baud, SERIAL_8N1); }
    void begin(unsigned long baud, uint16_t config) override;
    void end() override;
    int available(void) override;
    int availableForWrite(void) override;
    int peek(void) override;
    int read(void) override;
    void flush(void) override;
    size_t write(uint8_t c) override;
    using Print::write;
    operator bool() override { return true; }

    /* Uso interno: rutina de interrupcion */
    void _irq(void);

private:
    volatile uint8_t *_base;
    uint8_t     _uart;
    pin_size_t  _tx, _rx;
    bool        _begun;
    /* Buffer circular de un productor (IRQ) y un consumidor (loop):
     * _head solo lo escribe la interrupcion y _tail solo el programa. */
    volatile uint16_t _head, _tail;
    uint8_t _rxbuf[SERIAL_RX_BUFFER_SIZE];
};

extern UARTClass Serial;    /* UART1 */
extern UARTClass Serial0;   /* UART0 */
extern UARTClass Serial2;   /* UART2 */
extern UARTClass Serial3;   /* UART3 */

#endif
