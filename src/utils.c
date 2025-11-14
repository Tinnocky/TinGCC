#include "../include/utils.h"


void check_nullptr(const void* ptr, const char *format, ...){
    if (ptr == NULL){
        va_list args;
        va_start(args, format);

        fprintf(stderr, "Null Pointer Error: ");
        vfprintf(stderr, format, args);
        fprintf(stderr, "\n");

        va_end(args);

        exit(1);
    }
}


void print_error(const char *format, ...){
    va_list args;
    va_start(args, format);

    fprintf(stderr, "Error: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    va_end(args);

    exit(1);
}
