/*
 * BLE_UART - Periferico (peripheral): puente entre el Monitor Serie y el
 * movil por Bluetooth LE. Lo que escriba en uno aparece en el otro.
 *
 * En el movil use una de estas apps (gratis):
 *   - "Serial Bluetooth Terminal" (Android): Devices > Bluetooth LE > SCAN
 *     > "CH579-UART". Si pregunta, elija "Nordic UART".
 *   - "nRF Connect": conectar > "Nordic UART Service": active las
 *     notificaciones de TX (flechas) y escriba en RX.
 *   - "nRF Toolbox" > UART.
 * No hace falta emparejar desde los ajustes de Bluetooth del movil.
 *
 * Herramientas > Reloj del sistema: Cristal externo 32 MHz (HSE).
 */
#include <BLE_CH579.h>

void conectado()    { Serial.println("\n[BLE] movil conectado"); digitalWrite(LED_BUILTIN, HIGH); }
void desconectado() { Serial.println("\n[BLE] movil desconectado"); digitalWrite(LED_BUILTIN, LOW); }

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("=== CH579M: BLE_UART ===");

  BLE.onConnect(conectado);
  BLE.onDisconnect(desconectado);
  if (!BLE.beginPeripheral("CH579-UART")) {
    Serial.println("No arranco el BLE (revise el reloj: cristal 32 MHz)");
    while (1);
  }
  Serial.print("Anunciando \"CH579-UART\". MAC: ");
  BLE.printAddress(Serial);
  Serial.println("\nConectese desde el movil.");
}

void loop() {
  BLE.poll();
  while (BLE.available()) Serial.write(BLE.read());      /* movil -> monitor */
  while (Serial.available()) BLE.write(Serial.read());   /* monitor -> movil */
}
