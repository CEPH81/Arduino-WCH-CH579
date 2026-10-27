/*
 * 14_Timer_TMR0 - Interrupcion periodica de TMR0 cada 500 ms que alterna
 * LED2. Todo ocurre en la interrupcion; loop() queda vacio.
 * Adaptado del proyecto Keil "ch579_TIMER".
 *
 * Se usan las funciones del SDK (TMR0_TimerInit, TMR0_ITCfg...). La rutina
 * de interrupcion se declara con extern "C" para que sustituya a la vacia
 * del core. TMR0_TimerInit() ya limpia y arranca el contador, asi que no
 * hace falta el arreglo de RB_TMR_ALL_CLEAR del proyecto original.
 */
extern "C" void TMR0_IRQHandler(void) {
  if (TMR0_GetITFlag(TMR0_3_IT_CYC_END)) {
    TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);

  TMR0_TimerInit(GetSysClock() / 2);          // 500 ms (en ciclos de reloj)
  TMR0_ClearITFlag(TMR0_3_IT_CYC_END);
  TMR0_ITCfg(ENABLE, TMR0_3_IT_CYC_END);
  NVIC_EnableIRQ(TMR0_IRQn);
}

void loop() {
  // nada: el parpadeo lo hace la interrupcion
}
