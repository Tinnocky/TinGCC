#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>

#define INITIAL_TOKEN_LENGTH 10

bool is_naming_correct(const char *filename);
void check_nullptr(const void* ptr, const char *format, ...);
void print_error(const char *format, ...);

#endif 