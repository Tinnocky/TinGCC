#ifndef LEXER_H
#define LEXER_H

#include "token.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>


typedef struct ASTNode ASTNode;
struct ASTNode{
    Token token;
    ASTNode *next;
};


typedef struct {
    FILE *file;
    int line;
    int current_char;
} Lexer;


#include "../include/lexer.h"
#include "../include/utils.h"
#include <ctype.h>


// initialization and main stuff
Lexer *init_lexer(const char *filename);
void close_lexer(Lexer *lexer);
void free_ast(ASTNode *ast);
ASTNode *run_lexer(Lexer *lexer);

// determine token type
TokenType determine_token_type(Lexer *lexer, const char *token_string, const int token_length);
TokenType determine_multiword_tokens(Lexer *lexer, const char *token_string, const int token_length);
TokenType determine_number_token(Lexer *lexer, const char *token_string, const int token_length);

// make a token
Token *get_token(Lexer *lexer, const char *token_string, int token_length);

// get next string and such
char *get_token_string(Lexer *lexer, int *token_length, int *token_size, bool *is_string);
void skip_unnecessary(Lexer *lexer);
char *peek_token_string(Lexer *lexer, int *token_length, int *token_size, bool *is_string);
bool match_expected_token(Lexer *lexer, const char *expected_string);
void consume_filler_token(Lexer *lexer);

#endif