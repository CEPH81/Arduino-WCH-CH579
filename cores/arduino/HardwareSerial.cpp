/*
 * HardwareSerial.cpp - Implementacion comun de UART0..UART3
 */
#include "Arduino.h"
#include "ch579_pins.h"

static_assert((SERIAL_RX_BUFFER_SIZE & (SERIAL_RX_BUFFER_SIZE - 1)) == 0,
              "SERIAL_RX_BUFFER_SIZE debe ser potencia de 2");

static volatile uint8_t *const s_uart_base[4] = { BA_UART0, BA_UART1, BA_UART2, BA_UART3 };
static const IRQn_Type s_uart_irq[4] = { UART0_IRQn, UART1_IRQn, UART2_IRQn, UART3_IRQn };

#define REG(off)  (_base[(off)])

UARTClass::UARTClass(uint8_t uart, pin_size_t tx, pin_size_t rx)
    : _base(s_uart_base[uart & 3]), _uart(uart & 3), _tx(tx), _rx(rx),
      _begun(false), _head(0), _tail(0)
{
}

void UARTClass::begin(unsigned long baud, uint16_t config)
{
    if (baud == 0) baud = 9600;
    NVIC_DisableIRQ(s_uart_irq[_uart]);

    /* Pines: TX en alto como salida, RX como entrada con pull-up */
    pinMode(_rx, INPUT_PULLUP);
    digitalWrite(_tx, HIGH);
    pinMode(_tx, OUTPUT);

    /* Divisor: Fbaud = Fsys / 8 / DL  (con DIV = 1) */
    uint32_t x = 10UL * GetSysClock() / 8UL / baud;
    x = (x + 5UL) / 10UL;
    if (x == 0) x = 1;
    if (x > 0xFFFF) x = 0xFFFF;
    *(volatile uint16_t *)&REG(UART_DLL) = (uint16_t)x;
    REG(UART_DIV) = 1;

    /* FIFO habilitada con disparo a 1 byte: cada caracter recibido genera
     * la interrupcion al instante (con 4 bytes, los caracteres sueltos
     * esperaban al timeout de la FIFO). Ambas FIFO limpias. */
    REG(UART_FCR) = (0 << 6) | RB_FCR_TX_FIFO_CLR | RB_FCR_RX_FIFO_CLR | RB_FCR_FIFO_EN;

    /* Formato de trama */
    uint8_t lcr;
    switch (config & SERIAL_DATA_MASK) {
    case SERIAL_DATA_5: lcr = 0; break;
    case SERIAL_DATA_6: lcr = 1; break;
    case SERIAL_DATA_7: lcr = 2; break;
    default:            lcr = 3; break;
    }
    if ((config & SERIAL_STOP_BIT_MASK) == SERIAL_STOP_BIT_2) lcr |= RB_LCR_STOP_BIT;
    switch (config & SERIAL_PARITY_MASK) {
    case SERIAL_PARITY_ODD:   lcr |= RB_LCR_PAR_EN | (0 << 4); break;
    case SERIAL_PARITY_EVEN:  lcr |= RB_LCR_PAR_EN | (1 << 4); break;
    case SERIAL_PARITY_MARK:  lcr |= RB_LCR_PAR_EN | (2 << 4); break;
    case SERIAL_PARITY_SPACE: lcr |= RB_LCR_PAR_EN | (3 << 4); break;
    default: break;
    }
    REG(UART_LCR) = lcr;

    _head = _tail = 0;

    /* Habilita el pin TX y las interrupciones de recepcion */
    REG(UART_IER) = RB_IER_TXD_EN | RB_IER_RECV_RDY | RB_IER_LINE_STAT;
    REG(UART_MCR) |= RB_MCR_INT_OE;

    NVIC_EnableIRQ(s_uart_irq[_uart]);
    _begun = true;
}

void UARTClass::end()
{
    if (!_begun) return;
    flush();
    NVIC_DisableIRQ(s_uart_irq[_uart]);
    REG(UART_IER) = 0;
    REG(UART_MCR) &= ~RB_MCR_INT_OE;
    pinMode(_tx, INPUT);
    pinMode(_rx, INPUT);
    _head = _tail = 0;
    _begun = false;
}

int UARTClass::available(void)
{
    return (uint16_t)(_head - _tail) & (SERIAL_RX_BUFFER_SIZE - 1);
}

int UARTClass::availableForWrite(void)
{
    if (!_begun) return 0;
    return UART_FIFO_SIZE - REG(UART_TFC);
}

int UARTClass::peek(void)
{
    if (_head == _tail) return -1;
    return _rxbuf[_tail];
}

int UARTClass::read(void)
{
    if (_head == _tail) return -1;
    uint8_t c = _rxbuf[_tail];
    _tail = (uint16_t)((_tail + 1) & (SERIAL_RX_BUFFER_SIZE - 1));
    return c;
}

void UARTClass::flush(void)
{
    if (!_begun) return;
    while (!(REG(UART_LSR) & RB_LSR_TX_ALL_EMP)) { }
}

size_t UARTClass::write(uint8_t c)
{
    if (!_begun) return 0;
    while (REG(UART_TFC) >= UART_FIFO_SIZE) { }
    REG(UART_THR) = c;
    return 1;
}

void UARTClass::_irq(void)
{
    for (;;) {
        uint8_t iir = REG(UART_IIR) & RB_IIR_INT_MASK;
        switch (iir) {
        case UART_II_RECV_RDY:
        case UART_II_RECV_TOUT:
            while (REG(UART_LSR) & RB_LSR_DATA_RDY) {
                uint8_t c = REG(UART_RBR);
                uint16_t next = (uint16_t)((_head + 1) & (SERIAL_RX_BUFFER_SIZE - 1));
                if (next != _tail) {          /* si esta lleno, se descarta */
                    _rxbuf[_head] = c;
                    _head = next;
                }
            }
            break;
        case UART_II_LINE_STAT:
            (void)REG(UART_LSR);              /* leer LSR limpia el error */
            break;
        case UART_II_THR_EMPTY:
            break;
        case UART_II_MODEM_CHG:
            (void)REG(UART_MSR);
            break;
        default:                              /* UART_II_NO_INTER */
            return;
        }
    }
}
