/*
 * EEPROM_Contador - Cuenta los reinicios de la placa.
 *
 * El contador se guarda en la DataFlash y sobrevive al apagado.
 * Pulse RST varias veces y observe el Monitor Serie (115200 baudios).
 */
#include <EEPROM.h>

struct Datos {
  uint32_t firma;
  uint32_t arranques;
};
const uint32_t FIRMA = 0xC579C579;

void setup() {
  Serial.begin(115200);
  Datos d;
  EEPROM.get(0, d);
  if (d.firma != FIRMA) {          // primera vez: inicializa
    d.firma = FIRMA;
    d.arranques = 0;
  }
  d.arranques++;
  EEPROM.put(0, d);

  Serial.print("Arranques registrados: ");
  Serial.println(d.arranques);
}

void loop() { }
