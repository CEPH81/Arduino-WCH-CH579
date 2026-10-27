/*
 * 04_FadeLED - Brillo del LED de la placa con PWM por hardware.
 *
 * PB0 (LED2) es el canal PWM6. Pines con PWM: PA6 PA7 PB0 PB1 PB2 PB3 PB14
 * (PB23 tambien, pero es el pin de RESET).
 */
int brillo = 0;
int paso = 5;

void setup() {
  analogWriteFrequency(1000);   // 1 kHz (comun a todos los canales)
}

void loop() {
  analogWrite(LED_BUILTIN, brillo);
  brillo += paso;
  if (brillo <= 0 || brillo >= 255) paso = -paso;
  delay(20);
}
