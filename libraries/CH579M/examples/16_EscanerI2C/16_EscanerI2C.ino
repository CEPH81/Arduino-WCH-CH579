/*
 * 16_EscanerI2C - Busca dispositivos I2C cada 3 s y los muestra por Serial.
 * Adaptado del proyecto Keil "ch579_i2c".
 *
 * Pines: SDA = PA1, SCL = PA2, con pull-up externas de ~4.7k.
 * Wire.begin() libera el bus si un esclavo quedo reteniendo SDA
 * (9 pulsos de reloj + STOP), igual que I2C_Bus_Recover() del original.
 */
#include <Wire.h>

void setup() {
  Serial.begin(115200);
  Wire.setPins(PA1, PA2);      // SDA, SCL
  Wire.begin();
}

void loop() {
  Serial.print("\r\n--- Escaneo I2C ---\r\n");
  uint8_t encontrados = 0;
  char msg[48];

  for (uint8_t addr = 1; addr < 0x7F; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {            // ACK = hay dispositivo
      snprintf(msg, sizeof(msg), "Dispositivo encontrado en 0x%02X\r\n", addr);
      Serial.print(msg);
      encontrados++;
    }
  }
  if (encontrados == 0) {
    Serial.print("Ningun dispositivo respondio.\r\n");
  }
  delay(3000);
}
