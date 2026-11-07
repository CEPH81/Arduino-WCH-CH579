/*
 * BLE_Baliza - Difusor (broadcaster): la placa anuncia su nombre y, en los
 * "datos de fabricante", un contador y la lectura de A0 (PA4), que cambian
 * cada segundo. Nadie se conecta: cualquiera cerca puede leerlos.
 *
 * Para verlo: app "nRF Connect" (Android/iOS) > SCAN > "CH579 Baliza".
 * En "Manufacturer data" vera  0xFFFF  y luego  CC CC AA AA :
 * contador (2 bytes) y A0 (2 bytes), el byte bajo primero.
 * O cargue BLE_Escaner en otra placa CH579.
 *
 * Herramientas > Reloj del sistema: Cristal externo 32 MHz (HSE).
 */
#include <BLE_CH579.h>

uint16_t contador = 0;

void anunciar() {
  uint16_t a0 = analogRead(A0);
  uint8_t datos[4] = { (uint8_t)contador, (uint8_t)(contador >> 8),
                       (uint8_t)a0, (uint8_t)(a0 >> 8) };
  BLEAdvertisingData adv;
  adv.flags(0x04);                       /* solo BLE, sin modo descubrible */
  adv.name("CH579 Baliza");
  adv.manufacturer(0xFFFF, datos, 4);    /* 0xFFFF: id para pruebas */
  BLE.setAdvertisingData(adv);
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== CH579M: BLE_Baliza ===");

  BLE.setAdvertisingInterval(200);       /* ms: mas largo = menos consumo */
  anunciar();
  if (!BLE.beginBroadcaster()) {
    Serial.println("No arranco el BLE (revise el reloj: cristal 32 MHz)");
    while (1);
  }
  Serial.print("Anunciando como \"CH579 Baliza\". MAC: ");
  BLE.printAddress(Serial);
  Serial.println();
}

void loop() {
  static uint32_t t = 0;
  BLE.poll();
  if (millis() - t >= 1000) {
    t = millis();
    contador++;
    anunciar();                          /* cambia los datos sin parar el anuncio */
    Serial.print("contador=");
    Serial.print(contador);
    Serial.print("  A0=");
    Serial.println(analogRead(A0));
  }
}
