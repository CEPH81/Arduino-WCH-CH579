/*
 * SPI_Loopback - Prueba del SPI0 sin dispositivos externos.
 *
 * Una con un cable MOSI (PA14) y MISO (PA15). Cada byte enviado debe volver
 * igual. Resultado por Serial (CH340, 115200 baudios).
 */
#include <SPI.h>

void setup() {
  Serial.begin(115200);
  SPI.begin();
}

void loop() {
  uint8_t errores = 0;
  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  for (int i = 0; i < 256; i++) {
    uint8_t r = SPI.transfer((uint8_t)i);
    if (r != (uint8_t)i) errores++;
  }
  SPI.endTransaction();

  Serial.print("SPI loopback: ");
  Serial.println(errores == 0 ? "OK (256/256)" : "FALLO - revise el puente PA14-PA15");
  delay(1000);
}
