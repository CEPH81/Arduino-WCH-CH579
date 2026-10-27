/*
 * HCSR04_OLED - Medidor de distancia con HC-SR04 y barra de proximidad
 * en una OLED SSD1306. Adaptado del proyecto Keil "ch579_SSD1306_HC-SR04".
 *
 * Conexiones:
 *   Trig -> PB2
 *   Echo -> PA3 (divisor 1k/2k si el modulo es de 5 V; HC-SR04P a 3.3 V va directo)
 *   OLED -> SDA = PA1, SCL = PA2
 */
#include <Wire.h>
#include <OLED_SSD1306.h>
#include <HCSR04_CH579.h>

#define DIST_MIN_CM   2u     // rango util de la barra
#define DIST_MAX_CM   100u

OLED_SSD1306 oled(0x3C);
HCSR04 sensor(PB2, PA3);     // Trig, Echo

void setup() {
  sensor.begin();
  Wire.setPins(PA1, PA2);    // SDA, SCL
  Wire.begin();
  Wire.setClock(400000);
  oled.begin();
  oled.clear();
}

void loop() {
  uint16_t dist = sensor.readDistanceCm();

  /* Texto (pagina 0) */
  char msg[20];
  if (dist == HCSR04_NO_ECHO || dist > 400) {
    snprintf(msg, sizeof(msg), "FUERA DE RANGO ");
  } else {
    snprintf(msg, sizeof(msg), "DIST: %3u CM   ", dist);
  }
  oled.setCursor(0, 0);
  oled.print(msg);

  /* Barra de proximidad (paginas 3-4): mas cerca = mas larga */
  uint16_t barLen;
  if (dist == HCSR04_NO_ECHO || dist >= DIST_MAX_CM) {
    barLen = 0;
  } else if (dist <= DIST_MIN_CM) {
    barLen = SSD1306_WIDTH;
  } else {
    barLen = (uint16_t)(((uint32_t)(DIST_MAX_CM - dist) * SSD1306_WIDTH) / (DIST_MAX_CM - DIST_MIN_CM));
  }
  oled.drawBarGraph(3, 2, barLen);

  delay(150);   // el HC-SR04 pide >= 60 ms entre disparos
}
