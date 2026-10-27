/*
 * ch579.ld - Script de enlazado GCC para WCH CH579
 *
 * Mapa de memoria (datasheet CH579, cap. 2):
 *   CodeFlash  0x00000000  250 KB  (programa)
 *   DataFlash  0x0003E800    2 KB  (datos de usuario, tipo EEPROM)
 *   SRAM       0x20000000   32 KB
 */

MEMORY
{
  FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 250K
  RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 32K
}

/* Pila al final de la RAM; reserva minima para que el heap no la invada */
_estack          = ORIGIN(RAM) + LENGTH(RAM);
_min_stack_size  = 0x800;

ENTRY(Reset_Handler)
/* Fuerza que la tabla de vectores y las llamadas al sistema de newlib
 * (syscalls.c) salgan del archivo core.a aunque nada las pida aun */
EXTERN(__Vectors)
EXTERN(_sbrk _write _read _close _fstat _isatty _lseek _exit _kill _getpid)

SECTIONS
{
  .text :
  {
    KEEP(*(.isr_vector))
    *(.text .text.* .gnu.linkonce.t.*)
    *(.glue_7) *(.glue_7t)
    *(.rodata .rodata* .gnu.linkonce.r.*)
    *(.constdata*)

    KEEP(*(.init))
    KEEP(*(.fini))

    . = ALIGN(4);
    __preinit_array_start = .;
    KEEP(*(.preinit_array))
    __preinit_array_end = .;

    . = ALIGN(4);
    __init_array_start = .;
    KEEP(*(SORT(.init_array.*)))
    KEEP(*(.init_array))
    __init_array_end = .;

    . = ALIGN(4);
    __fini_array_start = .;
    KEEP(*(SORT(.fini_array.*)))
    KEEP(*(.fini_array))
    __fini_array_end = .;

    . = ALIGN(4);
  } > FLASH

  .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH

  .ARM.exidx :
  {
    __exidx_start = .;
    *(.ARM.exidx* .gnu.linkonce.armexidx.*)
    __exidx_end = .;
  } > FLASH

  . = ALIGN(4);
  _etext = .;
  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data .data.* .gnu.linkonce.d.*)
    *(.ramfunc .ramfunc.*)
    . = ALIGN(4);
    _edata = .;
  } > RAM AT > FLASH

  .bss (NOLOAD) :
  {
    . = ALIGN(4);
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss .bss.* .gnu.linkonce.b.*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } > RAM

  .noinit (NOLOAD) :
  {
    *(.noinit .noinit.*)
    . = ALIGN(8);
    end = .;
    _end = .;
    __end__ = .;
  } > RAM

  /* Comprobacion: debe quedar sitio para la pila */
  ASSERT(_end + _min_stack_size <= _estack, "No queda RAM suficiente para la pila")

  /DISCARD/ : { *(.ARM.attributes) }
}
