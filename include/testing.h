#ifndef TESTING_H
#define TESTING_H

#include "lexer.h"
#include "parser.h"
#include "utils.h"


// public function declarations
/* ----- Lexer ----- */
void print_tokens_list(TokenNode *tokens_head);

/* ----- Parser ----- */
void print_ast(ASTNode *ast_root);


#endif