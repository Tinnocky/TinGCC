#include "../include/codegen.h"


// private function declarations
static void gen_statement(Codegen *codegen, ASTNode *statement);


/* ----- Codegen "methods" ----- */
// initialize a new codegen struct with all data
// Note: the output file's name doesnt matter because its getting compiled straight away and deleted afterwards
Codegen *init_codegen(){
    Codegen *new_codegen = malloc(sizeof(Codegen));

    new_codegen->file = fopen(OUTPUT_FILENAME, "w");
    check_nullptr(new_codegen->file, "Codegen: Could not open the output file. \n");

    new_codegen->bracket_depth = 0;
    new_codegen->temp_var_count = 0;
}

// TODO: do it later at the end
void free_codegen(Codegen *codegen){

}


/* ----- Main functions ----- */
// the main function, going through all top level statements and writing them one by one
// works basically the same way parser and analysis work
void run_codegen(Codegen *codegen, ASTNode *ast_root){
    LinkedASTNode *statements = ast_root->data.program.statements;

    while (statements != NULL){
        gen_statement(codegen, statements->node);

        statements = statements->next;
    }
}

// based on the node type, redirects to the specific gen functions
static void gen_statement(Codegen *codegen, ASTNode *statement){

}