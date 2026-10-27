/*
 * 03_AnalogRead - Lee un potenciometro en A0 (PA4).
 *
 * El ADC del CH579 es de 12 bits y, con la ganancia por defecto
 * (AR_DEFAULT, x1), mide de 0 a ~2.1 V. Para 0-3.3 V use un divisor
 * resistivo o analogReference(AR_GAIN_1_4) (preciso desde ~2.9 V).
 *
 * Importante: la calibracion del ADC usa PA5 (A1); dejelo libre.
 */
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);       // 0..4095
  analogReference(AR_DEFAULT);    // 0..2.1 V
}

void loop() {
  int crudo = analogRead(A0);
  int mv = analogReadMillivolts(A0);
  Serial.print("A0 = ");
  Serial.print(crudo);
  Serial.print("  (");
  Serial.print(mv);
  Serial.println(" mV)");
  delay(250);
}
