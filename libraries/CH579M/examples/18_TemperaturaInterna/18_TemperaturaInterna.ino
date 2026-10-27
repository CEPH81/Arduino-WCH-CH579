/*
 * 18_TemperaturaInterna - Sensor de temperatura interno del CH579, por
 * Serial cada segundo. Adaptado del proyecto Keil "ch579_sensor_temp_interno".
 *
 * Usa las funciones del SDK de WCH. No combine este sketch con analogRead():
 * los dos configuran el mismo ADC.
 *
 * CALIBRACION MANUAL: este chip no trae grabado el punto de 25 C en la ROM
 * (D25_vol = 0), asi que ADC_GetCurrentTS() da un offset incorrecto,
 * aunque la pendiente es estable. Para calibrar:
 *   1. Ponga TEMP_OFFSET en 0, cargue y anote "Temp" ya estable.
 *   2. Mida la temperatura ambiente con un termometro.
 *   3. TEMP_OFFSET = temperatura_real - Temp_mostrado, y vuelva a cargar.
 * El valor 137 venia del proyecto Keil; puede variar, recalibrelo.
 */
#define TEMP_OFFSET  137

int16_t roughCalib;

void setup() {
  Serial.begin(115200);

  /* Enciende el sensor y selecciona su canal UNA sola vez: hacerlo en cada
     lectura no lo deja estabilizar */
  ADC_InterTSSampInit();
  /* El datasheet (15.3.2) pide el buffer de entrada, que la funcion del
     SDK no activa */
  R8_ADC_CFG |= RB_ADC_BUF_EN;

  /* Calibracion gruesa del offset del ADC (ejemplo oficial de WCH).
     Requiere PA5 (A1) como entrada flotante y sin nada conectado. */
  pinMode(PA5, INPUT);
  roughCalib = ADC_DataCalib_Rough();
  delay(2);   // calentamiento del sensor
}

void loop() {
  /* Promedia 100 muestras: el ruido del dato crudo se amplifica varios
     grados en la formula si se usa una sola */
  const uint16_t N = 100;
  int32_t sum = 0;
  noInterrupts();              // evita el ruido de la ISR del SysTick
  for (uint16_t i = 0; i < N; i++) {
    sum += (int32_t)ADC_ExcutSingleConver() + roughCalib;
    delayMicroseconds(20);
  }
  interrupts();

  int32_t avg = sum / (int32_t)N;
  if (avg < 0)    avg = 0;
  if (avg > 4095) avg = 4095;

  int temp = ADC_GetCurrentTS((uint16_t)avg) + TEMP_OFFSET;
  Serial.print("Raw: ");
  Serial.print(avg);
  Serial.print("  Temp: ");
  Serial.print(temp);
  Serial.println(" C");
  delay(1000);
}
