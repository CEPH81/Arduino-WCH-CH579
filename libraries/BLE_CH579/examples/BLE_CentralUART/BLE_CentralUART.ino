/*
 * BLE_CentralUART - Central: la placa busca otra placa CH579 con el ejemplo
 * BLE_UART (o BLE_ControlLED, o cualquier equipo con el servicio UART de
 * Nordic), se conecta sola y hace de puente con el Monitor Serie.
 *
 * Necesita DOS placas:
 *   placa 1: BLE_UART       placa 2: este ejemplo
 * Lo que escriba en el monitor de una aparece en el de la otra. Si se
 * pierde la conexion, vuelve a buscar.
 *
 * NOMBRE: el nombre que busca; vacio ("") = el primero con servicio UART.
 *
 * Herramientas > Reloj del sistema: Cristal externo 32 MHz (HSE).
 */
#include <BLE_CH579.h>

const char NOMBRE[] = "CH579-UART";

bool buscando = false;

void visto(const BLEDevice &d) {
  bool sirve = NOMBRE[0] ? d.matchName(NOMBRE) : d.hasService(BLE_UART_SERVICE);
  if (!sirve || !buscando) return;
  buscando = false;
  Serial.print("Encontrado ");
  d.printAddress(Serial);
  Serial.print(" (");
  Serial.print(d.rssi());
  Serial.println(" dBm), conectando...");
  BLE.connect(d);
}

void conectado() {
  Serial.println("[BLE] conectado: escriba para enviar");
  digitalWrite(LED_BUILTIN, HIGH);
}

void desconectado() {
  Serial.println("[BLE] desconectado");
  digitalWrite(LED_BUILTIN, LOW);
}

bool buscar() {
  Serial.print("Buscando \"");
  Serial.print(NOMBRE[0] ? NOMBRE : "(servicio UART)");
  Serial.println("\"...");
  buscando = BLE.scan(true);
  return buscando;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("=== CH579M: BLE_CentralUART ===");
  BLE.onDevice(visto);
  BLE.onConnect(conectado);
  BLE.onDisconnect(desconectado);
  if (!BLE.beginCentral()) {
    Serial.println("No arranco el BLE (revise el reloj: cristal 32 MHz)");
    while (1);
  }
  buscar();
}

void loop() {
  static uint32_t t = 0;
  BLE.poll();
  if (BLE.connected()) {
    while (BLE.available()) Serial.write(BLE.read());
    while (Serial.available()) BLE.write(Serial.read());
    t = millis();
  } else if (!buscando && millis() - t > 6000) {
    /* sin conexion: vuelve a buscar; si un intento sigue colgado, lo cancela */
    if (!buscar()) BLE.disconnect();
    t = millis();
  }
}
