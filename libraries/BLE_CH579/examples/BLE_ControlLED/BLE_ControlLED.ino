/*
 * BLE_ControlLED - Periferico: controle la placa desde el movil con
 * ordenes de texto por el servicio UART de Bluetooth LE.
 *
 * Ordenes (una por linea; la app debe enviar fin de linea):
 *   on / off      enciende o apaga el LED (PB0)
 *   blink N       parpadea N veces
 *   a0            responde la lectura de A0 (PA4) y su voltaje
 *   auto          envia A0 cada segundo (otra vez "auto" para parar)
 *   info          tiempo encendido y MAC
 *   ?             ayuda
 *
 * App: "Serial Bluetooth Terminal" (Android) o "nRF Toolbox" > UART,
 * dispositivo "CH579-LED". Ver BLE_UART para conectarse.
 *
 * Herramientas > Reloj del sistema: Cristal externo 32 MHz (HSE).
 */
#include <BLE_CH579.h>

char linea[32];
uint8_t largo = 0;
bool autoA0 = false;

void ayuda() {
  BLE.println("Ordenes: on, off, blink N, a0, auto, info, ?");
}

void enviarA0() {
  int v = analogRead(A0);
  BLE.print("A0=");
  BLE.print(v);
  BLE.print("  ");
  BLE.print(v * 3300L / 1023);           /* resolucion por omision: 10 bits */
  BLE.println(" mV");
}

void orden(char *s) {
  Serial.print("orden: ");
  Serial.println(s);
  if (!strcmp(s, "on"))        { digitalWrite(LED_BUILTIN, HIGH); BLE.println("LED encendido"); }
  else if (!strcmp(s, "off"))  { digitalWrite(LED_BUILTIN, LOW);  BLE.println("LED apagado"); }
  else if (!strncmp(s, "blink", 5)) {
    int n = atoi(s + 5);
    if (n <= 0) n = 3;
    for (int i = 0; i < n; i++) {        /* delay() sigue atendiendo el BLE */
      digitalWrite(LED_BUILTIN, HIGH); delay(150);
      digitalWrite(LED_BUILTIN, LOW);  delay(150);
    }
    BLE.print("parpadeo x");
    BLE.println(n);
  }
  else if (!strcmp(s, "a0"))   enviarA0();
  else if (!strcmp(s, "auto")) { autoA0 = !autoA0; BLE.println(autoA0 ? "auto: si" : "auto: no"); }
  else if (!strcmp(s, "info")) {
    BLE.print("encendido ");
    BLE.print(millis() / 1000);
    BLE.print(" s, MAC ");
    BLE.printAddress(BLE);
    BLE.println();
  }
  else ayuda();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("=== CH579M: BLE_ControlLED ===");
  BLE.onConnect(ayuda);                  /* al conectarse le muestra las ordenes */
  if (!BLE.beginPeripheral("CH579-LED")) {
    Serial.println("No arranco el BLE (revise el reloj: cristal 32 MHz)");
    while (1);
  }
  Serial.println("Esperando al movil (\"CH579-LED\")...");
}

void loop() {
  static uint32_t t = 0;
  BLE.poll();
  while (BLE.available()) {
    char c = BLE.read();
    if (c == '\r' || c == '\n') {
      if (largo) { linea[largo] = 0; orden(linea); largo = 0; }
    } else if (largo < sizeof(linea) - 1) {
      linea[largo++] = tolower(c);
    }
  }
  if (autoA0 && BLE.connected() && millis() - t >= 1000) {
    t = millis();
    enviarA0();
  }
  if (!BLE.connected()) autoA0 = false;
}
