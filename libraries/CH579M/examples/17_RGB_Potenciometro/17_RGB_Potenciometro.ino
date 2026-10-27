/*
 * 17_RGB_Potenciometro - Rueda de colores: un potenciometro en A0 (PA4)
 * recorre los matices (hue) de un LED RGB con 3 canales PWM por hardware.
 * Adaptado del proyecto Keil "ch579_pwm_rgb_pot".
 *
 *   Rojo  -> PWM4 -> PA6
 *   Verde -> PWM5 -> PA7
 *   Azul  -> PWM7 -> PB1
 * Potenciometro: extremos a 3V3/GND, cursor a PA4 (el ADC llega hasta ~2.1 V).
 */
const uint8_t PIN_R = PA6, PIN_G = PA7, PIN_B = PB1;

/* Matiz (0-359) a RGB de 8 bits con S = V = 255 */
void hsv2rgb(uint16_t h, uint8_t &r, uint8_t &g, uint8_t &b) {
  uint8_t region    = (uint8_t)(h / 60);
  uint8_t remainder = (uint8_t)(((h - (region * 60)) * 255) / 60);
  uint8_t v = 255, p = 0;
  uint8_t q = (uint8_t)((v * (255 - remainder)) / 255);
  uint8_t t = (uint8_t)((v * remainder) / 255);
  switch (region) {
    case 0:  r = v; g = t; b = p; break;
    case 1:  r = q; g = v; b = p; break;
    case 2:  r = p; g = v; b = t; break;
    case 3:  r = p; g = q; b = v; break;
    case 4:  r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}

void setup() {
  analogReadResolution(12);
  /* Mismo reloj de PWM que el original: Fsys/4/256 = 31.25 kHz */
  analogWriteFrequency(31250);
}

void loop() {
  uint16_t raw = analogRead(A0);                          // 0..4095 (calibrado)
  uint16_t hue = (uint16_t)(((uint32_t)raw * 360u) / 4095u);
  if (hue >= 360) hue = 359;

  uint8_t r, g, b;
  hsv2rgb(hue, r, g, b);
  analogWrite(PIN_R, r);
  analogWrite(PIN_G, g);
  analogWrite(PIN_B, b);
  delay(20);
}
