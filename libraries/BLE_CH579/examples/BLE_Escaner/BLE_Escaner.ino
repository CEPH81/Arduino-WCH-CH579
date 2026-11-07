/*
 * BLE_Escaner - Observador (observer): lista los dispositivos Bluetooth LE
 * cercanos con su MAC, intensidad (RSSI) y nombre, si lo anuncian.
 *
 * Cada dispositivo se muestra una vez por tanda de 3 s (la pila filtra los
 * repetidos). Escriba en el Monitor Serie:
 *   p   pausa / sigue
 *   d   muestra tambien los datos crudos del anuncio
 *
 * Herramientas > Reloj del sistema: Cristal externo 32 MHz (HSE).
 */
#include <BLE_CH579.h>

bool crudo = false;
bool pausa = false;

void visto(const BLEDevice &d) {
  char nombre[32];
  d.printAddress(Serial);
  Serial.print("  ");
  Serial.print(d.rssi());
  Serial.print(" dBm  ");
  Serial.print(d.isScanResponse() ? "resp " : d.connectable() ? "conec" : "anun ");
  if (d.name(nombre, sizeof(nombre))) {
    Serial.print("  \"");
    Serial.print(nombre);
    Serial.print('"');
  }
  if (d.hasService(BLE_UART_SERVICE)) Serial.print("  [UART BLE]");
  Serial.println();
  if (crudo) {
    Serial.print("    ");
    for (uint8_t i = 0; i < d.length(); i++) {
      if (d.data()[i] < 16) Serial.print('0');
      Serial.print(d.data()[i], HEX);
      Serial.print(' ');
    }
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("=== CH579M: BLE_Escaner ===");
  if (!BLE.beginObserver()) {
    Serial.println("No arranco el BLE (revise el reloj: cristal 32 MHz)");
    while (1);
  }
  BLE.onDevice(visto);
  BLE.scan(true);       /* activo: pide la respuesta de escaneo (ahi suele ir el nombre) */
  Serial.println("Escaneando... (p = pausa, d = datos crudos)");
}

void loop() {
  BLE.poll();
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'd') crudo = !crudo;
    if (c == 'p') {
      pausa = !pausa;
      if (pausa) { BLE.stopScan(); Serial.println("-- pausa --"); }
      else BLE.scan(true);
    }
  }
}
