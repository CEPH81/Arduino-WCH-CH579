/*
 * syscalls.c - Llamadas al sistema minimas para newlib (malloc, printf...)
 */
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>

extern char end;            /* fin de .bss / inicio del heap (ch579.ld) */
extern char _estack;        /* tope de la RAM */
extern char _min_stack_size;

static char *s_heap_end = 0;

void *_sbrk(ptrdiff_t incr)
{
    if (s_heap_end == 0) s_heap_end = &end;

    char *prev = s_heap_end;
    char *limit = &_estack - (uint32_t)&_min_stack_size;

    if (s_heap_end + incr > limit) {
        errno = ENOMEM;
        return (void *)-1;
    }
    s_heap_end += incr;
    return prev;
}

/* Salida de printf(). Si el sketch usa Serial, Serial_UART1.cpp la
 * redefine para que printf() escriba por Serial (tras Serial.begin()). */
__attribute__((weak)) int _write(int fd, const char *buf, int len)
{
    (void)fd; (void)buf;
    return len;
}

int _close(int fd)                 { (void)fd; return -1; }
int _fstat(int fd, struct stat *st){ (void)fd; st->st_mode = S_IFCHR; return 0; }
int _isatty(int fd)                { (void)fd; return 1; }
int _lseek(int fd, int p, int d)   { (void)fd; (void)p; (void)d; return 0; }
int _read(int fd, char *b, int n)  { (void)fd; (void)b; (void)n; return 0; }
int _getpid(void)                  { return 1; }
int _kill(int pid, int sig)        { (void)pid; (void)sig; errno = EINVAL; return -1; }
void _exit(int code)               { (void)code; while (1) { } }
