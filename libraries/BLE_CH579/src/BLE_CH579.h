/*
 * BLE_CH579.h - Bluetooth LE 4.2 del WCH CH579 con estilo Arduino.
 *
 * Usa la pila oficial de WCH (CH57xBLE.LIB). Un rol por programa:
 *   BLE.beginBroadcaster()      difusor: solo anuncia (baliza, iBeacon)
 *   BLE.beginObserver()         observador: solo escucha anuncios (escaner)
 *   BLE.beginPeripheral(nombre) periferico: el movil se conecta (UART BLE)
 *   BLE.beginCentral()          central: la placa se conecta a otro periferico
 *
 * Periferico y central intercambian datos con el "Nordic UART Service"
 * (NUS); BLE se usa como Serial: available(), read(), print()...
 * Llame a BLE.poll() en loop(); delay() tambien atiende la pila.
 *
 * Requisitos: Reloj del sistema = Cristal externo 32 MHz (HSE) y antena
 * (la pista o el conector de la placa). Usa el RTC, unos 9 KB de RAM y los
 * ultimos 512 bytes de la DataFlash: con BLE use EEPROM solo en 0..1535.
 */
#ifndef BLE_CH579_H
#define BLE_CH579_H

#include <Arduino.h>
#include "ble_core.h"

/* Tipos de campo de anuncio mas usados */
#define BLE_AD_FLAGS          0x01
#define BLE_AD_UUID16         0x03
#define BLE_AD_UUID128        0x07
#define BLE_AD_NAME_SHORT     0x08
#define BLE_AD_NAME           0x09
#define BLE_AD_TX_POWER       0x0A
#define BLE_AD_SERVICE_DATA   0x16
#define BLE_AD_MANUFACTURER   0xFF

/* Un dispositivo visto al escanear (valido solo dentro del callback) */
class BLEDevice {
public:
    BLEDevice(const ble_report_t *r) : _r(r) {}
    const uint8_t *address() const { return _r->addr; }   /* menos significativo primero */
    uint8_t addressType() const    { return _r->addrType; }
    int     rssi() const           { return _r->rssi; }
    bool    connectable() const    { return _r->evtType == 0 || _r->evtType == 1; }
    bool    isScanResponse() const { return _r->evtType == 4; }
    uint8_t length() const         { return _r->len; }
    const uint8_t *data() const    { return _r->data; }
    /* "84:C2:E4:03:02:02" (buf de 18) */
    char   *addressString(char *buf) const;
    void    printAddress(Print &p) const;
    /* Busca un campo; devuelve sus datos y su largo en *len, o NULL */
    const uint8_t *field(uint8_t type, uint8_t *len) const;
    /* Copia el nombre (completo o corto) a buf; false si no trae */
    bool    name(char *buf, uint8_t size) const;
    bool    hasService(uint16_t uuid16) const;
    bool    hasService(const uint8_t uuid128[16]) const;
    bool    matchName(const char *n) const;
private:
    const ble_report_t *_r;
};

/* Arma los datos de un anuncio (31 bytes como maximo) */
class BLEAdvertisingData {
public:
    BLEAdvertisingData() : _n(0) {}
    void clear() { _n = 0; }
    bool add(uint8_t type, const void *d, uint8_t len);
    bool flags(uint8_t f = 0x06) { return add(BLE_AD_FLAGS, &f, 1); }  /* general + solo BLE */
    bool name(const char *n);
    bool service(uint16_t uuid16);
    bool service(const uint8_t uuid128[16]) { return add(BLE_AD_UUID128, uuid128, 16); }
    bool manufacturer(uint16_t company, const void *d, uint8_t len);
    bool serviceData(uint16_t uuid16, const void *d, uint8_t len);
    bool txPower(int8_t dbm) { return add(BLE_AD_TX_POWER, &dbm, 1); }
    const uint8_t *data() const { return _b; }
    uint8_t length() const { return _n; }
    uint8_t free() const { return 31 - _n; }
private:
    uint8_t _b[31];
    uint8_t _n;
};

class BLEClass : public Stream {
public:
    /* --- antes de begin (opcional) */
    void setTxPower(int8_t dbm) { ble_set_tx_power(dbm); }         /* -20..3 dBm */
    void setAdvertisingInterval(uint16_t ms) { ble_set_adv_interval(ms); }

    /* --- iniciar en un rol: true si la pila arranco */
    bool beginBroadcaster() { return ble_begin_broadcaster() == 0; }
    bool beginObserver()    { return ble_begin_observer() == 0; }
    bool beginPeripheral(const char *name) { return ble_begin_peripheral(name) == 0; }
    bool beginCentral()     { return ble_begin_central() == 0; }
    void poll() { ble_poll(); }

    const uint8_t *address() { return ble_address(); }
    void printAddress(Print &p);

    /* --- anuncios (difusor y periferico) */
    bool setAdvertisingData(const uint8_t *d, uint8_t n) { return ble_set_adv_data(d, n) == 0; }
    bool setAdvertisingData(const BLEAdvertisingData &a) { return ble_set_adv_data(a.data(), a.length()) == 0; }
    bool setScanResponseData(const uint8_t *d, uint8_t n) { return ble_set_scan_rsp(d, n) == 0; }
    bool setScanResponseData(const BLEAdvertisingData &a) { return ble_set_scan_rsp(a.data(), a.length()) == 0; }
    bool advertise(bool on = true) { return ble_advertise(on) == 0; }

    /* --- escaneo (observador y central); el callback recibe cada anuncio */
    void onDevice(void (*cb)(const BLEDevice &d));
    bool scan(bool active = true) { return ble_scan(active) == 0; }
    void stopScan() { ble_stop_scan(); }

    /* --- conexion (periferico y central) */
    void onConnect(void (*cb)(void))    { _con = cb; ble_set_conn_cbs(_con, _dis); }
    void onDisconnect(void (*cb)(void)) { _dis = cb; ble_set_conn_cbs(_con, _dis); }
    bool connect(const BLEDevice &d) { return ble_connect(d.address(), d.addressType()) == 0; }
    bool connect(const uint8_t addr[6], uint8_t type = 0) { return ble_connect(addr, type) == 0; }
    void disconnect() { ble_disconnect(); }
    bool connected() { return ble_connected(); }
    operator bool() { return ble_connected(); }

    /* --- datos (Stream) */
    int available() override { return ble_available(); }
    int peek() override { return ble_peek(); }
    int read() override { return ble_read(); }
    void flush() override { ble_flush(); }
    int availableForWrite() override { return ble_tx_free(); }
    size_t write(uint8_t c) override { return ble_write(&c, 1); }
    size_t write(const uint8_t *b, size_t n) override { return ble_write(b, n); }
    using Print::write;

private:
    void (*_con)(void) = nullptr;
    void (*_dis)(void) = nullptr;
};

extern BLEClass BLE;

/* UUID del servicio UART (Nordic UART Service), por si lo quiere anunciar */
extern const uint8_t BLE_UART_SERVICE[16];

#endif
