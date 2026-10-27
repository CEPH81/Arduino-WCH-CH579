/*
 * ch57x_gcc_compat.h
 * Compatibilidad del SDK de WCH (escrito para ARM Compiler 5 / Keil) con GCC.
 * Lo incluye CH579SFR.h, asi que llega a todos los archivos del SDK.
 */
#ifndef CH57X_GCC_COMPAT_H
#define CH57X_GCC_COMPAT_H

#if defined(__GNUC__) && !defined(__ARMCC_VERSION)

/* __align(n) de ARMCC -> atributo de alineacion de GCC */
#ifndef __align
#define __align(n)  __attribute__((aligned(n)))
#endif

/* Intrinseco __nop() de ARMCC */
#ifndef __nop
#define __nop()     __asm volatile ("nop")
#endif

/* Las estructuras USB del SFR se declaran con __PACKED; ver CH579SFR.h */
#ifndef __PACKED
#define __PACKED
#endif

#endif /* __GNUC__ */
#endif /* CH57X_GCC_COMPAT_H */
