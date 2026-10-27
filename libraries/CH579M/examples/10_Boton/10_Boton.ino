/*
 * 10_Boton - Enciende LED2 mientras se mantiene pulsado un boton.
 * Adaptado del proyecto Keil "ch579_BUTTON".
 *
 * Boton entre PA0 y GND (usa el pull-up interno).
 */
const uint8_t PIN_BOTON = PA0;

void setup() {
  pinMode(PIN_BOTON, INPUT_PULLUP);
  digitalWrite(LED_BUILTIN, LOW);      // apagado al inicio
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  if (digitalRead(PIN_BOTON) == LOW) {           // presionado (a GND)
    delay(20);                                   // antirrebote simple
    if (digitalRead(PIN_BOTON) == LOW) {
      digitalWrite(LED_BUILTIN, HIGH);
      return;
    }
  }
  digitalWrite(LED_BUILTIN, LOW);
}
