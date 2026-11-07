/*
 * 20_AhorroMemoria - mide la memoria de la CH579M (250 KB de flash, 32 KB de
 * RAM) y muestra, con numeros reales, que tecnicas ahorran RAM. Monitor serie
 * a 115200.
 *
 * Reglas que se comprueban aqui:
 *  1. Las tablas que no cambian van con "const": se quedan en la flash
 *     (direccion 0x0000xxxx) y no gastan RAM. Sin const se copian a la RAM
 *     al arrancar (0x2000xxxx).
 *  2. Un bufer grande y local va en la pila solo mientras se usa la funcion;
 *     si es "static" o global ocupa la RAM siempre. Use local lo que es
 *     temporal y global solo lo que debe durar.
 *  3. String reserva y libera memoria dinamica (heap) y la fragmenta: en
 *     programas que corren dias, mejor char[] de tamano fijo y snprintf.
 *  4. En ARM no hace falta F("texto"): los textos ya quedan en la flash.
 *  5. Herramientas > Optimizacion > Tamano (-Os) da el codigo mas chico.
 *     printf con %f (float) agrega varios KB: imprima enteros si puede
 *     (por ejemplo los mV en vez de los V con decimales).
 */
#include <stdlib.h>

extern "C" {
extern char _sdata, _edata, _sbss, _ebss, _end, _estack, _etext, _sidata;
void *_sbrk(int incr);
}

/* 1. misma tabla: con const (flash) y sin const (RAM) */
const uint16_t tablaFlash[64] = { 1, 2, 3, 4, 5, 6, 7, 8 };
uint16_t tablaRAM[64] = { 1, 2, 3, 4, 5, 6, 7, 8 };

/* RAM libre entre el final del heap y la pila (lo que queda de verdad) */
unsigned long ramLibre(void) {
  char marca;                                  /* esta en la pila */
  char *finHeap = (char *)_sbrk(0);
  return (unsigned long)(&marca - finHeap);
}

void linea(const char *texto, unsigned long valor, const char *unidad) {
  Serial.print(texto);
  Serial.print(valor);
  Serial.println(unidad);
}

void direccion(const char *texto, const void *p) {
  Serial.print(texto);
  Serial.print("0x");
  Serial.print((unsigned long)p, HEX);
  Serial.println(((unsigned long)p < 0x20000000UL) ? "  (flash: no usa RAM)" : "  (RAM)");
}

/* 2. bufer local: solo ocupa pila mientras se ejecuta la funcion */
unsigned long usarBuferLocal(void) {
  volatile char bufer[2048];                   /* volatile: que el compilador no lo quite */
  for (int i = 0; i < 2048; i++) bufer[i] = (char)i;
  return ramLibre();                           /* RAM libre con el bufer en la pila */
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\r\n=== CH579M: AhorroMemoria ===");

  Serial.println("-- Uso fijo (del mapa de memoria) --");
  linea("Flash: codigo y constantes  ", (unsigned long)(&_etext) + (&_edata - &_sdata), " bytes");
  linea("RAM: variables con valor    ", &_edata - &_sdata, " bytes (.data)");
  linea("RAM: variables en cero      ", &_ebss - &_sbss, " bytes (.bss)");
  linea("RAM libre al arrancar       ", ramLibre(), " bytes de 32768");

  Serial.println("-- 1. const --");
  direccion("tablaFlash (const): ", tablaFlash);
  direccion("tablaRAM (sin const): ", tablaRAM);
  linea("tablaRAM gasta ", sizeof(tablaRAM), " bytes de RAM; tablaFlash 0");

  Serial.println("-- 2. bufer local de 2048 bytes --");
  unsigned long antes = ramLibre();
  unsigned long durante = usarBuferLocal();
  linea("RAM libre antes   ", antes, "");
  linea("RAM libre durante ", durante, "  (2 KB menos solo mientras se usa)");
  linea("RAM libre despues ", ramLibre(), "  (se devuelve al salir)");

  Serial.println("-- 3. String frente a char[] --");
  antes = ramLibre();
  {
    String s = "";
    for (int i = 0; i < 50; i++) s += String(i) + ",";   /* crece de a poco */
    linea("String de 140 caracteres: heap usado ", antes - ramLibre(), " bytes");
  }
  linea("Despues de liberarlo la RAM libre queda en ", ramLibre(), " (el heap no se encoge)");
  char texto[160];
  int n = 0;
  for (int i = 0; i < 50; i++) n += snprintf(texto + n, sizeof(texto) - n, "%d,", i);
  linea("Con char[160] y snprintf: ", n, " caracteres, 0 bytes de heap");

  Serial.println("Listo. Pruebe tambien Herramientas > Optimizacion > Tamano (-Os).");
}

void loop() {
}
