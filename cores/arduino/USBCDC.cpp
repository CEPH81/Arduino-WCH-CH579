/*
 * USBCDC.cpp - SerialUSB: CDC-ACM sobre el USB device del CH579
 *
 * Basado en el manejo de EP0 del ejemplo oficial de WCH
 * (MDK/Templates/USB/Device/VendorDefinedDev/Main.c) y en
 * USB_DeviceInit() del SDK, con las peticiones de clase CDC anadidas.
 *
 * Endpoints:
 *   EP0       control, 64 bytes
 *   EP1 IN    interrupcion (notificaciones CDC; no se usan, siempre NAK)
 *   EP2 OUT   bulk 64 bytes  PC -> placa
 *   EP2 IN    bulk 64 bytes  placa -> PC
 */
#include "Arduino.h"
#include "USBCDC.h"

USBCDC SerialUSB;

#define EP0_SIZE   64
#define EP_SIZE    64

/* VID/PID de pruebas de pid.codes (1209:0001). Sirve para desarrollo
 * privado; para distribuir hay que solicitar un PID propio. */
#define USB_VID    0x1209
#define USB_PID    0x0001

/* Peticiones de la clase CDC */
#define CDC_SET_LINE_CODING         0x20
#define CDC_GET_LINE_CODING         0x21
#define CDC_SET_CONTROL_LINE_STATE  0x22
#define CDC_SEND_BREAK              0x23

/* -------------------------------------------------------- buffers DMA */
static __attribute__((aligned(4))) uint8_t s_ep0buf[EP0_SIZE];
static __attribute__((aligned(4))) uint8_t s_ep1buf[2 * EP_SIZE];   /* OUT + IN */
static __attribute__((aligned(4))) uint8_t s_ep2buf[2 * EP_SIZE];   /* OUT + IN */
#define EP2_OUT_BUF  (s_ep2buf)
#define EP2_IN_BUF   (s_ep2buf + EP_SIZE)

/* Line coding: baudios (LE), stop, paridad, bits */
static uint8_t s_lineCoding[7] = { 0x00, 0xC2, 0x01, 0x00, 0x00, 0x00, 0x08 };  /* 115200 8N1 */

/* ---------------------------------------------------------- descriptores */
static const uint8_t s_devDescr[] = {
    0x12, 0x01,             /* bLength, DEVICE */
    0x00, 0x02,             /* USB 2.00 */
    0x02, 0x00, 0x00,       /* clase CDC (el interfaz define la subclase) */
    EP0_SIZE,
    (uint8_t)USB_VID, (uint8_t)(USB_VID >> 8),
    (uint8_t)USB_PID, (uint8_t)(USB_PID >> 8),
    0x00, 0x01,             /* bcdDevice 1.00 */
    0x01, 0x02, 0x03,       /* iManufacturer, iProduct, iSerialNumber */
    0x01                    /* 1 configuracion */
};

static const uint8_t s_cfgDescr[] = {
    /* Configuracion */
    0x09, 0x02, 67, 0x00, 0x02, 0x01, 0x00, 0x80, 0x32,    /* 2 interfaces, 100 mA */

    /* Interfaz 0: comunicaciones (CDC ACM) */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    0x05, 0x24, 0x00, 0x10, 0x01,                          /* Header 1.10 */
    0x05, 0x24, 0x01, 0x00, 0x01,                          /* Call management */
    0x04, 0x24, 0x02, 0x02,                                /* ACM: line coding + estado */
    0x05, 0x24, 0x06, 0x00, 0x01,                          /* Union: 0 maestro, 1 esclavo */
    0x07, 0x05, 0x81, 0x03, 0x08, 0x00, 0x10,              /* EP1 IN interrupcion, 8 B, 16 ms */

    /* Interfaz 1: datos */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    0x07, 0x05, 0x02, 0x02, EP_SIZE, 0x00, 0x00,           /* EP2 OUT bulk */
    0x07, 0x05, 0x82, 0x02, EP_SIZE, 0x00, 0x00,           /* EP2 IN bulk */
};
static_assert(sizeof(s_cfgDescr) == 67, "longitud del descriptor de configuracion");

static const uint8_t s_langDescr[] = { 0x04, 0x03, 0x09, 0x04 };
static const uint8_t s_manuDescr[] = { 0x08, 0x03, 'W', 0, 'C', 0, 'H', 0 };
static const uint8_t s_prodDescr[] = {
    0x22, 0x03, 'C', 0, 'H', 0, '5', 0, '7', 0, '9', 0, 'M', 0, ' ', 0, 'S', 0,
    'e', 0, 'r', 0, 'i', 0, 'a', 0, 'l', 0, 'U', 0, 'S', 0, 'B', 0
};
/* Numero de serie a partir del ID unico del chip: Windows asigna siempre
 * el mismo COM a la misma placa. 12 digitos hexadecimales. */
static uint8_t s_serialDescr[2 + 12 * 2];

static void build_serial_descr(void)
{
    uint8_t id[8];
    GetUniqueID(id);
    static const char hex[] = "0123456789ABCDEF";
    s_serialDescr[0] = sizeof(s_serialDescr);
    s_serialDescr[1] = 0x03;
    for (int i = 0; i < 6; i++) {
        s_serialDescr[2 + i * 4]     = (uint8_t)hex[id[5 - i] >> 4];
        s_serialDescr[2 + i * 4 + 1] = 0;
        s_serialDescr[2 + i * 4 + 2] = (uint8_t)hex[id[5 - i] & 0x0F];
        s_serialDescr[2 + i * 4 + 3] = 0;
    }
}

/* --------------------------------------------------- secciones criticas */
static inline uint32_t usb_lock(void)
{
    uint32_t en = NVIC->ISER[0] & (1UL << USB_IRQn);
    NVIC_DisableIRQ(USB_IRQn);
    return en;
}
static inline void usb_unlock(uint32_t en)
{
    if (en) NVIC_EnableIRQ(USB_IRQn);
}

/* ================================================================ init */
void USBCDC::resetEndpoints(void)
{
    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    R8_UEP1_CTRL = UEP_R_RES_NAK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP1_T_LEN = 0;
    R8_UEP2_T_LEN = 0;
    _txBusy = false;
    _txLastLen = 0;
    _rxPaused = false;
}

void USBCDC::start(void)
{
    if (_started) return;

    /* El USB usa los 48 MHz del PLL (480 MHz / 10) */
    PWR_UnitModCfg(ENABLE, UNIT_SYS_PLL);
    delay(5);

    build_serial_descr();
    _rxHead = _rxTail = _txHead = _txTail = 0;
    _configured = _dtr = _rts = false;

    R8_USB_CTRL   = 0x00;                       /* modo dispositivo */
    R8_UEP4_1_MOD = RB_UEP1_RX_EN | RB_UEP1_TX_EN;
    R8_UEP2_3_MOD = RB_UEP2_RX_EN | RB_UEP2_TX_EN;
    R16_UEP0_DMA  = (uint16_t)(uint32_t)s_ep0buf;
    R16_UEP1_DMA  = (uint16_t)(uint32_t)s_ep1buf;
    R16_UEP2_DMA  = (uint16_t)(uint32_t)s_ep2buf;
    resetEndpoints();
    R8_USB_DEV_AD = 0x00;

    /* Dispositivo + pull-up en D+, NAK automatico mientras haya un
     * evento sin atender, DMA */
    R8_USB_CTRL   = RB_UC_DEV_PU_EN | RB_UC_INT_BUSY | RB_UC_DMA_EN;
    R8_USB_INT_FG = 0xFF;
    R16_PIN_ANALOG_IE |= RB_PIN_USB_IE;         /* pines PB10/PB11 en modo USB */
    R8_UDEV_CTRL  = RB_UD_PD_DIS | RB_UD_PORT_EN;
    R8_USB_INT_EN = RB_UIE_SUSPEND | RB_UIE_BUS_RST | RB_UIE_TRANSFER;

    NVIC_EnableIRQ(USB_IRQn);
    _started = true;
}

void USBCDC::end()
{
    if (!_started) return;
    NVIC_DisableIRQ(USB_IRQn);
    R8_USB_INT_EN = 0;
    R8_UDEV_CTRL  = 0;                          /* puerto fisico apagado */
    R8_USB_CTRL   = RB_UC_RESET_SIE | RB_UC_CLR_ALL;   /* sin pull-up: el PC ve una desconexion */
    _configured = _dtr = _rts = false;
    _started = false;
}

uint32_t USBCDC::baud(void) const
{
    return (uint32_t)s_lineCoding[0] | ((uint32_t)s_lineCoding[1] << 8) |
           ((uint32_t)s_lineCoding[2] << 16) | ((uint32_t)s_lineCoding[3] << 24);
}

/* ============================================================== datos */
int USBCDC::available(void)
{
    return (uint16_t)(_rxHead - _rxTail) & (USBCDC_RX_BUFFER_SIZE - 1);
}

int USBCDC::peek(void)
{
    if (_rxHead == _rxTail) return -1;
    return _rxBuf[_rxTail];
}

int USBCDC::read(void)
{
    if (_rxHead == _rxTail) return -1;
    uint8_t c = _rxBuf[_rxTail];
    _rxTail = (uint16_t)((_rxTail + 1) & (USBCDC_RX_BUFFER_SIZE - 1));
    if (_rxPaused) rxResume();
    return c;
}

/* Vuelve a aceptar datos del PC cuando cabe un paquete completo */
void USBCDC::rxResume(void)
{
    uint32_t k = usb_lock();
    int freeSpace = USBCDC_RX_BUFFER_SIZE - 1 - available();
    if (_rxPaused && freeSpace >= EP_SIZE) {
        _rxPaused = false;
        R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_ACK;
    }
    usb_unlock(k);
}

int USBCDC::availableForWrite(void)
{
    uint16_t used = (uint16_t)(_txHead - _txTail) & (USBCDC_TX_BUFFER_SIZE - 1);
    return USBCDC_TX_BUFFER_SIZE - 1 - used;
}

/* Envia el siguiente paquete por EP2 IN si el endpoint esta libre */
void USBCDC::txKick(void)
{
    if (_txBusy || !_configured) return;
    uint16_t n = (uint16_t)(_txHead - _txTail) & (USBCDC_TX_BUFFER_SIZE - 1);
    if (n == 0) {
        if (_txLastLen == EP_SIZE) {               /* cierra la transferencia con un ZLP */
            _txLastLen = 0;
            R8_UEP2_T_LEN = 0;
            R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
            _txBusy = true;
        }
        return;
    }
    if (n > EP_SIZE) n = EP_SIZE;
    for (uint16_t i = 0; i < n; i++) {
        EP2_IN_BUF[i] = _txBuf[_txTail];
        _txTail = (uint16_t)((_txTail + 1) & (USBCDC_TX_BUFFER_SIZE - 1));
    }
    _txLastLen = (uint8_t)n;
    R8_UEP2_T_LEN = (uint8_t)n;
    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_ACK;
    _txBusy = true;
}

size_t USBCDC::write(const uint8_t *buf, size_t len)
{
    if (!_configured || !_dtr) return 0;           /* nadie escucha: se descarta */

    size_t done = 0;
    uint32_t t0 = millis();
    while (done < len) {
        int room = availableForWrite();
        if (room == 0) {
            if (millis() - t0 > _writeTimeout || !_configured) break;
            uint32_t k = usb_lock(); txKick(); usb_unlock(k);
            continue;
        }
        uint32_t k = usb_lock();
        while (room-- > 0 && done < len) {
            _txBuf[_txHead] = buf[done++];
            _txHead = (uint16_t)((_txHead + 1) & (USBCDC_TX_BUFFER_SIZE - 1));
        }
        txKick();
        usb_unlock(k);
        t0 = millis();
    }
    return done;
}

void USBCDC::flush(void)
{
    uint32_t t0 = millis();
    while (_configured && _dtr && (_txBusy || _txHead != _txTail)) {
        if (millis() - t0 > _writeTimeout) break;
    }
}

/* ===================================================== interrupcion USB */
void USBCDC::handleSetup(void)
{
    USB_SETUP_REQ *req = (USB_SETUP_REQ *)s_ep0buf;
    uint8_t  type  = req->bRequestType;
    uint16_t len   = 0;
    bool     error = false;

    R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
    if (R8_USB_RX_LEN != sizeof(USB_SETUP_REQ)) { error = true; goto done; }

    _setupReq = req->bRequest;
    _setupLen = req->wLength;

    if ((type & USB_REQ_TYP_MASK) == USB_REQ_TYP_CLASS) {
        switch (_setupReq) {
        case CDC_GET_LINE_CODING:
            _descr = s_lineCoding;
            len = sizeof(s_lineCoding);
            break;
        case CDC_SET_LINE_CODING:            /* los 7 bytes llegan en la fase OUT */
            break;
        case CDC_SET_CONTROL_LINE_STATE:
            _dtr = (req->wValue & 0x01) != 0;
            _rts = (req->wValue & 0x02) != 0;
            break;
        case CDC_SEND_BREAK:
            break;
        default:
            error = true;
            break;
        }
        if (!error && (type & 0x80)) {
            if (_setupLen > len) _setupLen = len;
            uint16_t n = _setupLen > EP0_SIZE ? EP0_SIZE : _setupLen;
            memcpy(s_ep0buf, _descr, n);
            _descr += n;
        }
        goto done;
    }

    if ((type & USB_REQ_TYP_MASK) != USB_REQ_TYP_STANDARD) { error = true; goto done; }

    switch (_setupReq) {
    case USB_GET_DESCRIPTOR:
        switch (req->wValue >> 8) {
        case USB_DESCR_TYP_DEVICE: _descr = s_devDescr; len = sizeof(s_devDescr); break;
        case USB_DESCR_TYP_CONFIG: _descr = s_cfgDescr; len = sizeof(s_cfgDescr); break;
        case USB_DESCR_TYP_STRING:
            switch (req->wValue & 0xFF) {
            case 0: _descr = s_langDescr;   len = sizeof(s_langDescr);   break;
            case 1: _descr = s_manuDescr;   len = sizeof(s_manuDescr);   break;
            case 2: _descr = s_prodDescr;   len = sizeof(s_prodDescr);   break;
            case 3: _descr = s_serialDescr; len = sizeof(s_serialDescr); break;
            default: error = true; break;
            }
            break;
        default:
            error = true;                     /* incluye DEVICE_QUALIFIER: solo Full Speed */
            break;
        }
        if (!error) {
            if (_setupLen > len) _setupLen = len;
            uint16_t n = _setupLen > EP0_SIZE ? EP0_SIZE : _setupLen;
            memcpy(s_ep0buf, _descr, n);
            _descr += n;
        }
        break;

    case USB_SET_ADDRESS:
        _address = (uint8_t)(req->wValue & 0x7F);   /* se aplica tras la fase de estado */
        break;

    case USB_GET_CONFIGURATION:
        s_ep0buf[0] = _config;
        if (_setupLen > 1) _setupLen = 1;
        break;

    case USB_SET_CONFIGURATION:
        _config = (uint8_t)(req->wValue & 0xFF);
        resetEndpoints();
        _configured = (_config != 0);
        if (!_configured) _dtr = _rts = false;
        break;

    case USB_GET_INTERFACE:
        s_ep0buf[0] = 0;
        if (_setupLen > 1) _setupLen = 1;
        break;

    case USB_SET_INTERFACE:
        break;

    case USB_GET_STATUS:
        s_ep0buf[0] = 0;
        s_ep0buf[1] = 0;
        if (_setupLen > 2) _setupLen = 2;
        break;

    case USB_CLEAR_FEATURE:
        if ((type & USB_REQ_RECIP_MASK) == USB_REQ_RECIP_ENDP) {
            switch (req->wIndex & 0xFF) {
            case 0x82: R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK;
                       _txBusy = false; break;
            case 0x02: R8_UEP2_CTRL = (R8_UEP2_CTRL & ~(RB_UEP_R_TOG | MASK_UEP_R_RES)) | UEP_R_RES_ACK;
                       _rxPaused = false; break;
            case 0x81: R8_UEP1_CTRL = (R8_UEP1_CTRL & ~(RB_UEP_T_TOG | MASK_UEP_T_RES)) | UEP_T_RES_NAK; break;
            default:   error = true; break;
            }
        } else if ((type & USB_REQ_RECIP_MASK) != USB_REQ_RECIP_DEVICE) {
            error = true;
        }
        break;

    case USB_SET_FEATURE:
        break;                                /* remote wakeup: se acepta y se ignora */

    default:
        error = true;
        break;
    }

done:
    if (error) {
        R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;
        return;
    }
    if (type & 0x80) {                        /* el dispositivo envia datos */
        len = _setupLen > EP0_SIZE ? EP0_SIZE : _setupLen;
        _setupLen -= len;
    } else {
        len = 0;                              /* estado: paquete vacio (DATA1) */
    }
    R8_UEP0_T_LEN = (uint8_t)len;
    R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;
}

void USBCDC::ep0In(void)
{
    switch (_setupReq) {
    case USB_GET_DESCRIPTOR:
    case CDC_GET_LINE_CODING: {
        uint16_t n = _setupLen > EP0_SIZE ? EP0_SIZE : _setupLen;
        if (n == 0 && _setupReq == CDC_GET_LINE_CODING) {
            R8_UEP0_T_LEN = 0;
            R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
            break;
        }
        memcpy(s_ep0buf, _descr, n);
        _setupLen -= n;
        _descr += n;
        R8_UEP0_T_LEN = (uint8_t)n;
        R8_UEP0_CTRL ^= RB_UEP_T_TOG;
        break;
    }
    case USB_SET_ADDRESS:
        R8_USB_DEV_AD = (R8_USB_DEV_AD & RB_UDA_GP_BIT) | _address;
        R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        break;
    default:                                  /* fin de la fase de estado */
        R8_UEP0_T_LEN = 0;
        R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
        break;
    }
}

void USBCDC::ep0Out(void)
{
    if (_setupReq == CDC_SET_LINE_CODING) {
        uint8_t n = R8_USB_RX_LEN;
        if (n > sizeof(s_lineCoding)) n = sizeof(s_lineCoding);
        memcpy(s_lineCoding, s_ep0buf, n);
        _setupReq = 0xFF;
    }
}

void USBCDC::_irq(void)
{
    uint8_t flags = R8_USB_INT_FG;

    if (flags & RB_UIF_TRANSFER) {
        uint8_t st = R8_USB_INT_ST;
        switch (st & (MASK_UIS_TOKEN | MASK_UIS_ENDP)) {
        case UIS_TOKEN_SETUP:
            handleSetup();
            break;
        case UIS_TOKEN_IN:
            ep0In();
            break;
        case UIS_TOKEN_OUT:
            ep0Out();
            break;

        case UIS_TOKEN_OUT | 2:
            if (st & RB_UIS_TOG_OK) {
                uint8_t n = R8_USB_RX_LEN;
                for (uint8_t i = 0; i < n; i++) {
                    uint16_t next = (uint16_t)((_rxHead + 1) & (USBCDC_RX_BUFFER_SIZE - 1));
                    if (next == _rxTail) break;          /* no deberia pasar: se pausa antes */
                    _rxBuf[_rxHead] = EP2_OUT_BUF[i];
                    _rxHead = next;
                }
                /* Si no cabe otro paquete, NAK hasta que read() libere sitio */
                if (USBCDC_RX_BUFFER_SIZE - 1 - available() < EP_SIZE) {
                    _rxPaused = true;
                    R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_R_RES) | UEP_R_RES_NAK;
                }
            }
            break;
        case UIS_TOKEN_IN | 2:
            R8_UEP2_CTRL = (R8_UEP2_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            _txBusy = false;
            txKick();
            break;
        case UIS_TOKEN_IN | 1:
            R8_UEP1_CTRL = (R8_UEP1_CTRL & ~MASK_UEP_T_RES) | UEP_T_RES_NAK;
            break;
        default:
            break;
        }
        R8_USB_INT_FG = RB_UIF_TRANSFER;
    } else if (flags & RB_UIF_BUS_RST) {
        R8_USB_DEV_AD = 0;
        _address = 0;
        _config = 0;
        _configured = _dtr = _rts = false;
        resetEndpoints();
        R8_USB_INT_FG = RB_UIF_BUS_RST;
    } else if (flags & RB_UIF_SUSPEND) {
        R8_USB_INT_FG = RB_UIF_SUSPEND;
    } else {
        R8_USB_INT_FG = flags;
    }
}

extern "C" void USB_IRQHandler(void)
{
    SerialUSB._irq();
}
