/*
 * 05_Interrupcion - Interrupcion externa en un pin.
 *
 * Conecte un pulsador entre PA3 y GND. Cada pulsacion cambia el LED.
 * Pines con interrupcion: PA0..PA15 y PB0..PB15.
 * Modos: LOW, HIGH, RISING, FALLING y CHANGE.
 */
const uint8_t PIN_BOTON = PA3;
volatile uint32_t pulsaciones = 0;
volatile uint32_t ultimo = 0;

void alPulsar() {
  uint32_t ahora = millis();
  if (ahora - ultimo > 50) {        // antirrebote de 50 ms
    pulsaciones++;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
  ultimo = ahora;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(PIN_BOTON, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_BOTON), alPulsar, FALLING);
}

void loop() {
  static uint32_t antes = 0;
  if (pulsaciones != antes) {
    antes = pulsaciones;
    Serial.print("Pulsaciones: ");
    Serial.println(antes);
  }
}
