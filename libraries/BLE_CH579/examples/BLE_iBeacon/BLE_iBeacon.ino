/*
 * BLE_iBeacon - Baliza con formato iBeacon (o Eddystone-URL, ver abajo).
 *
 * Un iBeacon anuncia un UUID de 16 bytes, un "major" y un "minor" y la
 * potencia medida a 1 m; con ella el movil calcula la distancia aproximada.
 * Pruebelo con apps como "Beacon Scanner" o "nRF Connect".
 *
 * Con EDDYSTONE en 1 anuncia una direccion web (Eddystone-URL) en su lugar.
 *
 * Herramientas > Reloj del sistema: Cristal externo 32 MHz (HSE).
 */
#include <BLE_CH579.h>

#define EDDYSTONE  0                     /* 1 = Eddystone-URL */

/* Cambie el UUID por uno suyo (p. ej. de uuidgenerator.net) */
const uint8_t UUID[16] = { 0xE2, 0xC5, 0x6D, 0xB5, 0xDF, 0xFB, 0x48, 0xD2,
                           0xB0, 0x60, 0xD0, 0xF5, 0xA7, 0x10, 0x96, 0xE0 };
const uint16_t MAJOR = 1;
const uint16_t MINOR = 579;
const int8_t   POTENCIA_1M = -59;        /* dBm medidos a 1 m (ajustelo) */

void setup() {
  Serial.begin(115200);
  Serial.println("=== CH579M: BLE_iBeacon ===");

  BLEAdvertisingData adv;
  adv.flags(0x06);
#if EDDYSTONE
  /* Eddystone-URL: https://www.wch.cn  (0x03 = "https://", 0x07 = ".cn") */
  const uint8_t uuid16[2] = { 0xAA, 0xFE };
  adv.add(BLE_AD_UUID16, uuid16, 2);
  const uint8_t url[] = { 0x10, (uint8_t)POTENCIA_1M, 0x03, 'w', 'w', 'w', '.', 'w', 'c', 'h', 0x07 };
  adv.serviceData(0xFEAA, url, sizeof(url));
#else
  uint8_t ib[23];
  ib[0] = 0x02; ib[1] = 0x15;            /* tipo iBeacon, 21 bytes */
  memcpy(ib + 2, UUID, 16);
  ib[18] = MAJOR >> 8; ib[19] = MAJOR;   /* major y minor van al reves (big endian) */
  ib[20] = MINOR >> 8; ib[21] = MINOR;
  ib[22] = (uint8_t)POTENCIA_1M;
  adv.manufacturer(0x004C, ib, sizeof(ib));
#endif
  BLE.setAdvertisingData(adv);
  BLE.setAdvertisingInterval(100);
  BLE.setTxPower(0);

  if (!BLE.beginBroadcaster()) {
    Serial.println("No arranco el BLE (revise el reloj: cristal 32 MHz)");
    while (1);
  }
  Serial.print("Baliza activa. MAC: ");
  BLE.printAddress(Serial);
  Serial.println();
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  BLE.poll();
  digitalWrite(LED_BUILTIN, (millis() % 2000) < 50);   /* destello: sigue viva */
}
