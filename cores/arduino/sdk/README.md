# SDK de WCH para CH579

Drivers de periféricos (`StdPeriphDriver`) y cabeceras del CH579 proporcionados
por WCH (Nanjing Qinheng Microelectronics), tomados de la plantilla de proyecto
del usuario y del pack de Keil `WCH57x_DFP 1.1.0`. CMSIS Core (Cortex-M0) de ARM.

Cambios hechos para compilar con GCC (todos marcados con `[arduino-wch-ch579]`):

- Comentarios convertidos de GB2312 a UTF-8.
- `inc/ch57x_gcc_compat.h` (nuevo): `__align(n)` y `__nop()` de ARM Compiler.
- `inc/CH579SFR.h`: incluye `ch57x_gcc_compat.h`; las estructuras USB usan
  `__attribute__((packed))` en lugar de `__packed`.
- Se retiraron `CH57x_usbhostBase.c` y `CH57x_usbhostClass.c` porque dependen de
  la librería cerrada `CH579UFI.lib` (disco USB).

La licencia de este SDK es la de WCH; el `license.txt` del pack no la especifica.
Por eso este repositorio se mantiene privado hasta aclarar la redistribución.
