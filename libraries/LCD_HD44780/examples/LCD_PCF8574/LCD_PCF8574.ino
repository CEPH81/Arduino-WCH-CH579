/*
 * LCD_PCF8574 - LCD 16x2 con modulo I2C PCF8574.
 * Adaptado del proyecto Keil "ch579_LCD".
 *
 * Conexiones: SDA = PA1, SCL = PA2 (pull-up de 4.7k recomendadas).
 * Direccion del PCF8574: 0x26 (use el ejemplo I2C_Scanner si no la sabe).
 */
#include <Wire.h>
#include <LCD_PCF8574.h>

LCD_PCF8574 lcd(0x26);

void setup() {
  Wire.setPins(PA1, PA2);   // SDA, SCL
  Wire.begin();
  lcd.begin();

  lcd.setCursor(0, 0);
  lcd.print("Hola CH579M");
  lcd.setCursor(0, 1);
  lcd.print("Val: ");
  lcd.print(23.456, 2);     // -> "23.46"
  lcd.print(" cnt=");
  lcd.print(42);
}

void loop() {
  // el mensaje queda fijo en pantalla
}
