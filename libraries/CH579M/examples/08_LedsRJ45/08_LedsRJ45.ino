/*
 * 08_LedsRJ45 - Parpadeo de los LEDs del conector RJ45 (CN1, HR911105A).
 *
 * Adaptado de blink_eth_leds.c (version con la HAL Hal_CH57xx_*).
 *
 * Pines medidos con multimetro en la placa real (el esquema dice PB5/PB6):
 *   LED verde    -> PB4  (LED_ETH_GREEN)   activo en BAJO: LOW = encendido
 *   LED amarillo -> PB7  (LED_ETH_YELLOW)  activo en BAJO: LOW = encendido
 *   LED2 (rojo)  -> PB0  (LED_BUILTIN)     activo en ALTO: HIGH = encendido
 *
 * Secuencia:
 *   1. LED2 y el verde parpadean alternados (cuando uno enciende, el otro apaga)
 *   2. pausa
 *   3. parpadea solo el amarillo
 *   4. pausa
 *
 * Nota: PB4/PB7 tambien son RX0/TX0 de Serial0; no use Serial0 en este sketch.
 * El reloj (cristal de 32 MHz) y el SysTick los configura el core antes de
 * setup(), segun el menu "Reloj del sistema".
 */

/* Invierte el estado de un pin de salida (equivale a GPIO_TogglePin) */
void togglePin(uint8_t pin) {
  digitalWrite(pin, !digitalRead(pin));
}

void setup() {
  /* Estado inicial (igual que el original): LED2 encendido, verde y
   * amarillo apagados. Se escribe el nivel ANTES de pinMode() para que los
   * LEDs activos en bajo no destellen al pasar el pin a salida. */
  digitalWrite(LED_BUILTIN, HIGH);     // PB0 activo en alto -> encendido
  digitalWrite(LED_ETH_GREEN, HIGH);   // activo en bajo -> apagado
  digitalWrite(LED_ETH_YELLOW, HIGH);  // activo en bajo -> apagado

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_ETH_GREEN, OUTPUT);
  pinMode(LED_ETH_YELLOW, OUTPUT);
}

void loop() {
  /* --- LED2 y verde alternados --- */
  for (int i = 0; i < 5; i++) {
    togglePin(LED_BUILTIN);
    togglePin(LED_ETH_GREEN);
    delay(300);
  }
  digitalWrite(LED_BUILTIN, LOW);      // apaga LED2
  digitalWrite(LED_ETH_GREEN, HIGH);   // apaga verde

  delay(500);

  /* --- Solo amarillo --- */
  for (int i = 0; i < 5; i++) {
    togglePin(LED_ETH_YELLOW);
    delay(300);
  }
  digitalWrite(LED_ETH_YELLOW, HIGH);  // apaga amarillo

  delay(500);
}
