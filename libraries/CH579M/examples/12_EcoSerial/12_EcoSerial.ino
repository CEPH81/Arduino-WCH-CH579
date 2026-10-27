/*
 * 12_EcoSerial - Eco de lo que llega por Serial, normalizando el Enter.
 * Adaptado del proyecto Keil "ch579_UART1_CH340N".
 *
 * La terminal puede enviar "\r", "\n" o "\r\n" al pulsar Enter: siempre se
 * responde con "\r\n" y se ignora el '\n' que llega justo despues de '\r'.
 */
uint8_t prevByte = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  while (Serial.available()) {
    uint8_t c = (uint8_t)Serial.read();
    if (c == '\r' || c == '\n') {
      if (!(c == '\n' && prevByte == '\r')) {
        Serial.print("\r\n");
      }
    } else {
      Serial.write(c);
    }
    prevByte = c;
  }
}
