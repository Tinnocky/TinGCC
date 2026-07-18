#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>


// forward declarations of types from lexer.h
// this lets us not fully including lexer.h and evade circular includes
typedef struct TokenNode TokenNode;
typedef struct Lexer Lexer;


// public function declarations
/* ----- error handling ----- */
_Noreturn void print_error(const char *format, ...);
void check_nullptr(const void* ptr, const char *format, ...);

/* ----- all other ----- */
void realloc_check(char **token_string, int *token_length, int *token_size);
bool ends_with(const char *string, const char *suffix);


#endif 