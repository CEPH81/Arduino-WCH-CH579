/*
 * 11_HolaSerial - Envia un saludo por Serial (UART1 -> CH340) cada segundo.
 * Adaptado del proyecto Keil "ch579_UART1".
 */
void setup() {
  Serial.begin(115200);
}

void loop() {
  Serial.print("Hola desde CH579M - UART1\r\n");
  delay(1000);
}
