#ifndef OLED_FONT5X7_H
#define OLED_FONT5X7_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/**
  * @brief  Devuelve el glifo de 5 columnas (5 bytes, bit0 = fila superior)
  *         para el caracter c. Si c no esta soportado, devuelve un espacio
  *         en blanco.
  * @param  c     : caracter ASCII a buscar
  * @param  glyph : puntero de salida al arreglo de 5 bytes
  * @return Ancho del glifo en columnas (siempre 5 en esta fuente)
  */
uint8_t Font5x7_GetGlyph(char c, const uint8_t **glyph);

#ifdef __cplusplus
}
#endif

#endif  /* OLED_FONT5X7_H */
