/*
 * 15_Captura_TMR0 - Mide el tiempo entre flancos de una senal en PA3 con la
 * captura de entrada de TMR0 y lo envia por Serial en microsegundos.
 * Adaptado del proyecto Keil "ch579_tim0_capture".
 *
 * Senal de 0 a 3.3 V en PA3 (no conecte 5 V directo).
 *
 * Correccion respecto al original: en modo Edge_To_Edge el bit 25 del dato
 * capturado no es tiempo, indica el nivel medido (1 = alto, 0 = bajo)
 * (datasheet, cap. 9). El original lo sumaba al tiempo y los pulsos en
 * alto salian ~1.05 s mas largos. Aqui se separa.
 */
#define CAP_LEVEL_BIT   (1UL << 25)
#define CAP_TIME_MASK   (CAP_LEVEL_BIT - 1)

uint32_t fsys;

void setup() {
  Serial.begin(115200);
  fsys = GetSysClock();

  pinMode(PA3, INPUT_PULLUP);                 // PA3 = entrada de captura de TMR0
  TMR0_CAPTimeoutCfg(fsys / 10);              // ~100 ms sin flancos = timeout
  TMR0_CapInit(Edge_To_Edge);                 // tiempo entre flancos consecutivos
}

void loop() {
  while (TMR0_CAPDataCounter() > 0) {
    uint32_t data  = TMR0_CAPGetData();
    uint32_t ticks = data & CAP_TIME_MASK;
    uint32_t us    = (uint32_t)(((uint64_t)ticks * 1000000UL) / fsys);

    Serial.print("Pulso ");
    Serial.print((data & CAP_LEVEL_BIT) ? "alto" : "bajo");
    Serial.print(": ");
    Serial.print(us);
    Serial.println(" us");
  }
}
