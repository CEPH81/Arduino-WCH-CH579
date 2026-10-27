/*
 * 07_Printf - printf() de C por Serial y funciones del SDK de WCH.
 *
 * En este core, printf() escribe por Serial (tras Serial.begin()).
 * Tambien se pueden llamar las funciones del SDK de WCH (CH57x_*.h)
 * directamente desde el sketch.
 */
void setup() {
  Serial.begin(115200);
  delay(100);
  printf("Chip ID: 0x%02X\r\n", R8_CHIP_ID);
  printf("Reloj del sistema: %lu Hz\r\n", (unsigned long)GetSysClock());
}

void loop() {
  printf("millis = %lu\r\n", millis());
  delay(1000);
}
