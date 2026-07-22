#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdio.h>
#include "parser.h"


#define OUTPUT_FILENAME "output.c"
#define RUNTIME_FILENAME "\"ting_runtime.h\""


typedef struct {
    FILE *file; // we write the code to here, .c file
    int bracket_depth; // TODO: make this do something. add it later
    int temp_var_count; // sometimes ill need to create variables for stuff that didnt get a name in the .ting code. this keeps track
} Codegen;


// public function declarations
/* ----- Codegen "methods" ----- */
Codegen *init_codegen(void);
void free_codegen(Codegen *codegen);

/* ----- Main functions ----- */
void run_codegen(Codegen *codegen, ASTNode *ast_root);


#endif