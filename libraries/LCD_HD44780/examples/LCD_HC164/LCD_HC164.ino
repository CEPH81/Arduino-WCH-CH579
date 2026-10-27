/*
 * LCD_HC164 - LCD HD44780 controlada por un 74HC164 (2 hilos).
 * Adaptado del proyecto Keil "ch579_LCD_HC164AG".
 *
 * Pines: DATA = PA12, CLK = PB8.
 * Mapeo asumido del 164: Q0=RS Q1=RW Q2=E Q3=luz Q4..Q7=D4..D7.
 * Si sale basura en pantalla, el adaptador usa otro orden de bits.
 */
#include <LCD_HC164.h>

LCD_HC164 lcd(PA12, PB8);   // DATA, CLK

void setup() {
  lcd.begin();
  lcd.setCursor(0, 0);
  lcd.print("Hola CH579M");
  lcd.setCursor(0, 1);
  lcd.print("LCD via HC164!");
}

void loop() {
}
