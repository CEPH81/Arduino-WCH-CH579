/*
 * hooks.c - Funciones que el usuario puede redefinir
 */
#include "Arduino.h"

/* yield() se llama mientras delay() espera; un planificador puede usarla */
static void __empty(void) { }
void yield(void) __attribute__((weak, alias("__empty")));
