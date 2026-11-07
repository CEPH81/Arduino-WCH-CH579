/*
 * ble_core.h - Capa en C sobre la pila Bluetooth LE de WCH (CH57xBLE.LIB).
 * La usa BLE_CH579.h; en un sketch use la clase BLE.
 */
#ifndef BLE_CORE_H
#define BLE_CORE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Un anuncio (o respuesta de escaneo) recibido */
typedef struct {
    uint8_t addr[6];        /* direccion, byte menos significativo primero */
    uint8_t addrType;       /* 0 publica, 1 aleatoria */
    uint8_t evtType;        /* 0 conectable, 2 escaneable, 3 no conectable, 4 respuesta de escaneo */
    int8_t  rssi;           /* dBm */
    uint8_t len;
    const uint8_t *data;    /* campos de anuncio [largo][tipo][datos]... */
} ble_report_t;

typedef void (*ble_report_cb_t)(const ble_report_t *r);
typedef void (*ble_void_cb_t)(void);

/* Roles: se elige uno al iniciar. Devuelven 0 si la pila arranco. */
int  ble_begin_broadcaster(void);
int  ble_begin_observer(void);
int  ble_begin_peripheral(const char *name);
int  ble_begin_central(void);

void ble_poll(void);
uint8_t ble_role(void);
const uint8_t *ble_address(void);      /* MAC propia, menos significativo primero */

/* Antes de begin */
void ble_set_tx_power(int8_t dbm);
void ble_set_adv_interval(uint16_t ms);

/* Anuncios (difusor y periferico) */
int  ble_set_adv_data(const uint8_t *d, uint8_t n);
int  ble_set_scan_rsp(const uint8_t *d, uint8_t n);
int  ble_advertise(uint8_t on);

/* Escaneo (observador y central) */
void ble_set_report_cb(ble_report_cb_t cb);
int  ble_scan(uint8_t active);
void ble_stop_scan(void);

/* Conexion (periferico y central): datos como un puerto serie */
void ble_set_conn_cbs(ble_void_cb_t on_conn, ble_void_cb_t on_disc);
uint8_t ble_connected(void);
int  ble_connect(const uint8_t addr[6], uint8_t addrType);   /* central */
void ble_disconnect(void);
int  ble_available(void);
int  ble_peek(void);
int  ble_read(void);
uint16_t ble_write(const uint8_t *d, uint16_t n);
int  ble_tx_free(void);
void ble_flush(void);

#ifdef __cplusplus
}
#endif
#endif
