/*
 * 02_SerialEcho - Devuelve todo lo que recibe por Serial.
 *
 * Serial es el UART1 (PA9/PA8), conectado al CH340N de la placa.
 * Abra el Monitor Serie a 115200 baudios y escriba algo.
 */
void setup() {
  Serial.begin(115200);
  Serial.println("CH579M listo. Escriba algo:");
}

void loop() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    Serial.write(c);
  }
}
