/*
 * BLE_CH579.cpp - Clases BLE, BLEDevice y BLEAdvertisingData.
 */
#include "BLE_CH579.h"
#include <string.h>

BLEClass BLE;

const uint8_t BLE_UART_SERVICE[16] = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0,
                                       0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E };

/* delay() llama a yield(): asi la pila se atiende tambien mientras espera */
extern "C" void yield(void) { ble_poll(); }

/* ---------------------------------------------------------------- BLE */
static void print_addr(Print &p, const uint8_t *a)
{
    for (int i = 5; i >= 0; i--) {
        if (a[i] < 16) p.write('0');
        p.print(a[i], HEX);
        if (i) p.write(':');
    }
}

void BLEClass::printAddress(Print &p) { print_addr(p, ble_address()); }

static void (*s_dev_cb)(const BLEDevice &d);
static void dev_tramp(const ble_report_t *r)
{
    BLEDevice d(r);
    s_dev_cb(d);
}

void BLEClass::onDevice(void (*cb)(const BLEDevice &d))
{
    s_dev_cb = cb;
    ble_set_report_cb(cb ? dev_tramp : nullptr);
}

/* ---------------------------------------------------------- BLEDevice */
char *BLEDevice::addressString(char *buf) const
{
    static const char hx[] = "0123456789ABCDEF";
    char *p = buf;
    for (int i = 5; i >= 0; i--) {
        *p++ = hx[_r->addr[i] >> 4];
        *p++ = hx[_r->addr[i] & 15];
        *p++ = i ? ':' : 0;
    }
    return buf;
}

void BLEDevice::printAddress(Print &p) const { print_addr(p, _r->addr); }

const uint8_t *BLEDevice::field(uint8_t type, uint8_t *len) const
{
    const uint8_t *p = _r->data, *e = p + _r->len;
    while (p + 1 < e && p[0]) {
        uint8_t n = p[0];
        if (p + 1 + n > e) break;
        if (p[1] == type) {
            if (len) *len = n - 1;
            return p + 2;
        }
        p += n + 1;
    }
    return nullptr;
}

bool BLEDevice::name(char *buf, uint8_t size) const
{
    uint8_t n;
    const uint8_t *d = field(BLE_AD_NAME, &n);
    if (!d) d = field(BLE_AD_NAME_SHORT, &n);
    if (!d || !size) return false;
    if (n > size - 1) n = size - 1;
    memcpy(buf, d, n);
    buf[n] = 0;
    return true;
}

bool BLEDevice::matchName(const char *s) const
{
    uint8_t n;
    const uint8_t *d = field(BLE_AD_NAME, &n);
    if (!d) d = field(BLE_AD_NAME_SHORT, &n);
    return d && strlen(s) == n && !memcmp(d, s, n);
}

bool BLEDevice::hasService(uint16_t u) const
{
    for (uint8_t t = 0x02; t <= 0x03; t++) {          /* lista incompleta o completa */
        uint8_t n;
        const uint8_t *d = field(t, &n);
        for (uint8_t i = 0; d && i + 1 < n; i += 2)
            if ((d[i] | (d[i + 1] << 8)) == u) return true;
    }
    return false;
}

bool BLEDevice::hasService(const uint8_t u[16]) const
{
    for (uint8_t t = 0x06; t <= 0x07; t++) {
        uint8_t n;
        const uint8_t *d = field(t, &n);
        for (uint8_t i = 0; d && i + 15 < n; i += 16)
            if (!memcmp(d + i, u, 16)) return true;
    }
    return false;
}

/* -------------------------------------------------- BLEAdvertisingData */
bool BLEAdvertisingData::add(uint8_t type, const void *d, uint8_t len)
{
    if (_n + 2 + len > 31) return false;
    _b[_n++] = len + 1;
    _b[_n++] = type;
    memcpy(_b + _n, d, len);
    _n += len;
    return true;
}

bool BLEAdvertisingData::name(const char *s)
{
    uint8_t n = strlen(s);
    if (_n + 2 + n <= 31) return add(BLE_AD_NAME, s, n);
    if (_n + 3 > 31) return false;
    return add(BLE_AD_NAME_SHORT, s, 31 - 2 - _n);     /* nombre recortado */
}

bool BLEAdvertisingData::service(uint16_t u)
{
    uint8_t d[2] = { (uint8_t)u, (uint8_t)(u >> 8) };
    return add(BLE_AD_UUID16, d, 2);
}

bool BLEAdvertisingData::manufacturer(uint16_t c, const void *d, uint8_t len)
{
    if (_n + 4 + len > 31) return false;
    _b[_n++] = len + 3;
    _b[_n++] = BLE_AD_MANUFACTURER;
    _b[_n++] = (uint8_t)c;
    _b[_n++] = (uint8_t)(c >> 8);
    memcpy(_b + _n, d, len);
    _n += len;
    return true;
}

bool BLEAdvertisingData::serviceData(uint16_t u, const void *d, uint8_t len)
{
    if (_n + 4 + len > 31) return false;
    _b[_n++] = len + 3;
    _b[_n++] = BLE_AD_SERVICE_DATA;
    _b[_n++] = (uint8_t)u;
    _b[_n++] = (uint8_t)(u >> 8);
    memcpy(_b + _n, d, len);
    _n += len;
    return true;
}
