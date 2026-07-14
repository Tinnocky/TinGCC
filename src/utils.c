#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "../include/utils.h"
#include "../include/lexer.h" // only for free_tokens_list, close_lexer


// private function declarations
/* ----- error handling ----- */
static _Noreturn void verror(const char *prefix, const char *format, va_list args);


/* ----- error handling ----- */
// prints an error message and exits the program
// already takes the va_list as a parameter
// Note: all va_lists that get passed here arent being passed to va_end(), this should be ok tho
static _Noreturn void verror(const char *prefix, const char *format, va_list args){
    fprintf(stderr, "%s: ", prefix);
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");

    exit(1);
}

// prints an error message and exits the program
_Noreturn void print_error(const char *format, ...){
    va_list args;
    va_start(args, format);

    verror("Error", format, args);
}

// checks if a pointer is null
// if it is then print an error and exit the program
void check_nullptr(const void* ptr, const char *format, ...){
    if (ptr == NULL){
        va_list args;
        va_start(args, format);

        verror("Null Pointer Error", format, args);
    }
}


/* ----- all other ----- */
// reallocates more memory to a pointer, only if needed 
void realloc_check(char **token_string, int *token_length, int *token_size){
    if (*token_length >= *token_size){
        *token_size *= 2; // double size

        *token_string = realloc(*token_string, *token_size * sizeof(char));
        check_nullptr(*token_string, "Realloc for a token string failed. \n");
    }
}


// cleans the program by freeing everything before exit. runs on program end
// TODO: add parser stuff to here
void global_cleanup(void){
    // free the tokens_list
    if (global_tokens_head != NULL){
        free_tokens_list(global_tokens_head);
    }

    // free the lexer
    if (global_lexer != NULL){
        close_lexer(global_lexer);
    }
}


// checks if the passed string ends with the passed suffix
// had to make this myself cuz theres no library that does this
bool ends_with(const char *string, const char *suffix){
    int string_length = strlen(string);
    int suffix_length = strlen(suffix);

    if (suffix_length > string_length){
        return false; // suffix cant be longer than the string itself
    }

    int length_without_suffix = string_length - suffix_length;
    return strcmp(string + length_without_suffix, suffix) == 0; // compare the two suffixes, true if theyre same
}