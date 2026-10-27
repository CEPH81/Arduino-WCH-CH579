/*
 * LogDemo - Log por niveles en color. Adaptado del proyecto Keil "ch579_log_demo".
 *
 * Cada 5 mensajes cambia de nivel/color y marca el inicio del bloque:
 *   verde (Info) -> amarillo (Warn) -> azul (Debug) -> rojo (Error) -> ...
 *
 * Use una terminal con colores ANSI (PuTTY, Tera Term) a 115200 baudios.
 * En el Monitor Serie del IDE, llame a Log_SetColors(false).
 */
#include <CH579_Log.h>

static const LOG_LevelTypeDef ciclo[]  = { LOG_INFO, LOG_WARN, LOG_DEBUG, LOG_ERROR };
static const char *const      nombre[] = { "VERDE", "AMARILLO", "AZUL", "ROJO" };
const uint8_t NUM_COLORES = 4;
const uint8_t POR_BLOQUE  = 5;
uint32_t contador = 0;

void setup() {
  Log_Init(115200);
  Log_SetLevel(LOG_DEBUG);     // deja pasar los 4 niveles
  // Log_SetColors(false);     // para el Monitor Serie del Arduino IDE
}

void loop() {
  uint8_t idx = (uint8_t)((contador / POR_BLOQUE) % NUM_COLORES);
  LOG_LevelTypeDef nivel = ciclo[idx];

  if ((contador % POR_BLOQUE) == 0) {
    Log_Write(nivel, "CICLO", "--- inicio bloque %s ---", nombre[idx]);
  }
  Log_Write(nivel, "LOOP", "mensaje #%lu", (unsigned long)contador);
  contador++;
  delay(500);
}
