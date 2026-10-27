/*
 * 13_PWM_TMR0 - PWM de alta resolucion con el temporizador TMR0 en PA3.
 * Adaptado del proyecto Keil "ch579_PWM".
 *
 * A diferencia de analogWrite() (8 bits, PWM4..PWM11), TMR0 genera un PWM
 * de hasta 26 bits. Aqui: 1 kHz y un barrido de 0 % a 100 % y vuelta.
 * Pruebe con un LED (y su resistencia) o el osciloscopio en PA3.
 *
 * Se usan directamente las funciones del SDK de WCH (CH57x_timer.h);
 * el core no usa TMR0, asi que esta libre.
 */
const uint32_t PWM_FREQ_HZ = 1000;
uint32_t periodo;   // en ciclos de reloj

void setup() {
  periodo = GetSysClock() / PWM_FREQ_HZ;      // 32000 a 32 MHz

  pinMode(PA3, OUTPUT);                       // PA3 = TMR0 / PWM0
  TMR0_PWMCycleCfg(periodo);                  // periodo total
  TMR0_PWMInit(High_Level, PWM_Times_1);      // reposo bajo, pulso activo en alto
}

void loop() {
  for (uint32_t duty = 0; duty <= 100; duty++) {
    TMR0_PWMActDataWidth((periodo * duty) / 100);
    delay(10);
  }
  for (uint32_t duty = 100; duty > 0; duty--) {
    TMR0_PWMActDataWidth((periodo * duty) / 100);
    delay(10);
  }
}
