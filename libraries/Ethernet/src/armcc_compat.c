/*
 * armcc_compat.c - Funciones del runtime de ARM Compiler 5 (Keil) que pide
 * CH57xNET.LIB y que GCC no trae.
 */
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

unsigned int __aeabi_uread4(void *p)                  { unsigned int v; memcpy(&v, p, 4); return v; }
unsigned int __aeabi_uwrite4(unsigned int v, void *p) { memcpy(p, &v, 4); return v; }
int __2printf(const char *f, ...)
{
    va_list a; va_start(a, f);
    int r = vprintf(f, a);
    va_end(a);
    return r;
}
