#include "serial.h"
#include "pico/stdlib.h"
#include <stdarg.h>

void serial_init(void) {
    stdio_init_all();
}

void serial_print(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void serial_println(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}
