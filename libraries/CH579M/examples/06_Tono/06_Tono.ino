/*
 * 06_Tono - Melodia con un buzzer pasivo.
 *
 * Conecte el buzzer entre PB1 y GND. tone() usa el temporizador TMR3 y
 * funciona en cualquier pin (un tono a la vez).
 */
const uint8_t PIN_BUZZER = PB1;
const unsigned int notas[] = { 262, 294, 330, 349, 392, 440, 494, 523 };  // Do..Do

void setup() { }

void loop() {
  for (unsigned int i = 0; i < sizeof(notas) / sizeof(notas[0]); i++) {
    tone(PIN_BUZZER, notas[i], 200);
    delay(250);
  }
  noTone(PIN_BUZZER);
  delay(1500);
}
