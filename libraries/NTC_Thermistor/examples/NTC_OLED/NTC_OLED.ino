/*
 * NTC_OLED - Termometro con NTC de 10k y barra de temperatura en una OLED
 * SSD1306. Adaptado del proyecto Keil "ch579_SSD1306_NTC".
 *
 * Conexiones:
 *   Divisor: 3V3 -> NTC -> PA4 (A0) -> R fija 10k -> GND
 *   OLED   : SDA = PA1, SCL = PA2
 *
 * Beta = 3950 es un valor tipico: revise el datasheet de su NTC.
 * Con este divisor el ADC satura hacia los 38 C (ver NTC_Thermistor.h);
 * entonces se muestra "SATURADO".
 */
#include <Wire.h>
#include <OLED_SSD1306.h>
#include <NTC_Thermistor.h>

#define TEMP_MIN_C  10.0f    // rango de la barra
#define TEMP_MAX_C  50.0f

OLED_SSD1306 oled(0x3C);
NTC_Thermistor ntc(A0, 10000.0f, 10000.0f, 3950.0f, 3.3f, NTC_HIGH_SIDE);

void setup() {
  ntc.begin();
  Wire.setPins(PA1, PA2);    // SDA, SCL
  Wire.begin();
  Wire.setClock(400000);
  oled.begin();
  oled.clear();
}

void loop() {
  float temp = ntc.readCelsius();

  char msg[24];
  if (isnan(temp)) {
    snprintf(msg, sizeof(msg), "TEMP: SATURADO ");
  } else {
    int t10 = (int)(temp * 10.0f + (temp >= 0 ? 0.5f : -0.5f));
    snprintf(msg, sizeof(msg), "TEMP: %d.%d C   ", t10 / 10, abs(t10 % 10));
  }
  oled.setCursor(0, 0);
  oled.print(msg);

  uint16_t barLen;
  if (isnan(temp) || temp >= TEMP_MAX_C) barLen = SSD1306_WIDTH;
  else if (temp <= TEMP_MIN_C)           barLen = 0;
  else barLen = (uint16_t)(((temp - TEMP_MIN_C) * SSD1306_WIDTH) / (TEMP_MAX_C - TEMP_MIN_C));
  oled.drawBarGraph(3, 2, barLen);

  delay(300);
}
