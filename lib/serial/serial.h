#pragma once
#include <stdio.h>

void serial_init(void);
void serial_print(const char *fmt, ...);
void serial_println(const char *fmt, ...);
