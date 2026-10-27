/*
 * abi.cpp - Soporte minimo de C++ (new/delete, funciones virtuales puras)
 */
#include <stdlib.h>
#include <new>

extern "C" void __cxa_pure_virtual(void) { while (1) { } }
extern "C" void __cxa_deleted_virtual(void) { while (1) { } }

void *operator new(size_t size)   { return malloc(size); }
void *operator new[](size_t size) { return malloc(size); }
void *operator new(size_t size, const std::nothrow_t &) noexcept   { return malloc(size); }
void *operator new[](size_t size, const std::nothrow_t &) noexcept { return malloc(size); }
void operator delete(void *p) noexcept   { free(p); }
void operator delete[](void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept   { free(p); }
void operator delete[](void *p, size_t) noexcept { free(p); }
