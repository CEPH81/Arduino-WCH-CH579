/*
 * 09_PotenciometroADC - Lee un potenciometro en A0 (PA4) y envia el valor
 * crudo de 12 bits (0-4095) por Serial cada 500 ms.
 * Adaptado del proyecto Keil "ch579_ADC_UART1".
 *
 * Potenciometro: extremos a 3V3 y GND, cursor a PA4.
 * Con la ganancia por defecto el ADC mide de 0 a ~2.1 V: por encima la
 * lectura se queda en 4095. Para recorrer 0-3.3 V ponga una resistencia
 * en serie con el extremo de 3V3 (p. ej. 6.8k con un pot de 10k).
 * El core aplica la calibracion de offset del ADC (deje PA5 libre).
 */
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
}

void loop() {
  uint16_t val = analogRead(A0);
  Serial.print("ADC: ");
  Serial.println(val);
  delay(500);
}
