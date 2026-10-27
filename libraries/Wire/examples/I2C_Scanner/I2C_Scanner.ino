/*
 * I2C_Scanner - Busca dispositivos en el bus I2C.
 *
 * Pines por defecto: SDA=PB21, SCL=PB20. Para usar otros, descomente
 * Wire.setPins(). Resultado por Serial (115200 baudios).
 */
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  // Wire.setPins(PB21, PB20);   // (SDA, SCL)
  Wire.begin();
  Wire.setClock(100000);
}

void loop() {
  int encontrados = 0;
  Serial.println("Buscando dispositivos I2C...");
  for (uint8_t addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.print("  Dispositivo en 0x");
      if (addr < 16) Serial.print('0');
      Serial.println(addr, HEX);
      encontrados++;
    }
  }
  Serial.print("Total: ");
  Serial.println(encontrados);
  Serial.println();
  delay(3000);
}
