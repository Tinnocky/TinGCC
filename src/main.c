#include <stdlib.h>
#include <string.h>
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/analysis.h"
#include "../include/codegen.h"
#include "../include/utils.h"
#include "../include/testing.h"


// private function declarations
/* ----- Testing functions ----- */
static void run_lexer_test(char *filename);
static void run_parser_test(char *filename);
static void run_analysis_test(char *filename);

/* ----- Helpers ----- */
static char *get_program_name_with_path(const char *filename);
static char *get_program_name(const char *filename);
static void arguments_check(int argc, char *argv[]);

/* ----- Main compiling function ----- */
static void run_compiler(char *filename);


// variable declarations
#define FILENAME_SUFFIX ".ting"
#define COMMAND_SIZE 128
#define COMPILING_COMMAND "gcc " OUTPUT_FILENAME " runtime/ting_runtime.c -Iruntime -o" // without filename


/* ----- Testing functions ----- */
static void run_lexer_test(char *filename){
    Lexer *lexer = init_lexer(filename);

    TokenNode *tokens_head = run_lexer(lexer);
    print_tokens_list(tokens_head);

    free_tokens_list(tokens_head);
    free_lexer(lexer);
}

static void run_parser_test(char *filename){
    Lexer *lexer = init_lexer(filename);
    TokenNode *tokens_head = run_lexer(lexer);
    Parser *parser = init_parser(tokens_head);

    ASTNode *ast_root = run_parser(parser);
    print_ast(ast_root);

    free_ast(ast_root);
    free_parser(parser);
    free_tokens_list(tokens_head);
    free_lexer(lexer);
}

static void run_analysis_test(char *filename){
    Lexer *lexer = init_lexer(filename);
    TokenNode *tokens_head = run_lexer(lexer);
    Parser *parser = init_parser(tokens_head);
    ASTNode *ast_root = run_parser(parser);

    Context *context = init_context();
    run_analysis(context, ast_root);

    // we dont print nothing specific for the analysis tests,
    // its either error or end cleanly

    free_context(context);
    free_ast(ast_root);
    free_parser(parser);
    free_tokens_list(tokens_head);
    free_lexer(lexer);
}


/* ----- Helpers ----- */
// check if arguments passed are correct and if the file passed is a .ting file
static void arguments_check(int argc, char *argv[]){
    if (argc != 2){
        print_error("Wrong usage. Should call: %s <filename>\n", argv[0]);
    }

    char *filename = argv[1];
    if (!ends_with(filename, FILENAME_SUFFIX)){
        print_error("Wrong usage. Should pass file ending with .ting");
    }
}

// returns the filename without the .ting at the end and the the path at the start
// Note: this is used for testings
static char *get_program_name_with_path(const char *filename){
    int program_length = strlen(filename) - strlen(FILENAME_SUFFIX);

    char *program_name = malloc(program_length + 1); // +1 for null terminator
    check_nullptr(program_name, "Compiler: malloc for the program name failed. \n");

    for (int i = 0; i < program_length; i++){
        program_name[i] = filename[i];
    }

    program_name[program_length] = '\0';

    return program_name;
}

// returns the filename without the .ting at the end and the the path at the start
static char *get_program_name(const char *filename){
    // skip past the last slash, if there is one
    const char *base = strrchr(filename, '/');
    if (base != NULL){
        base++; // move past the slash itself
    }
    else {
        base = filename; // no directory in the path so it stays as is
    }

    int program_length = strlen(base) - strlen(FILENAME_SUFFIX);

    char *program_name = malloc(program_length + 1); // +1 for null terminator
    check_nullptr(program_name, "Compiler: malloc for the program name failed. \n");

    for (int i = 0; i < program_length; i++){
        program_name[i] = base[i];
    }

    program_name[program_length] = '\0';

    return program_name;
}

/* ----- Main compiling function ----- */
static void run_compiler(char *filename){
    Lexer *lexer = init_lexer(filename);
    TokenNode *tokens_head = run_lexer(lexer);
    Parser *parser = init_parser(tokens_head);
    ASTNode *ast_root = run_parser(parser);
    Context *context = init_context();
    run_analysis(context, ast_root);
    Codegen *codegen = init_codegen();
    run_codegen(codegen, ast_root); // make output file
    free_codegen(codegen); // file needs to be closed

    char *program_name = get_program_name(filename);
    char full_command[COMMAND_SIZE];
    snprintf(full_command, sizeof(full_command), "%s %s", COMPILING_COMMAND, program_name);

    int result = system(full_command); // compiler output file

    if (result != 0){
        print_error("Compiler: gcc failed to compile the generated code, with return code %d. \n", result);
    }

    // close compiler
    free(program_name);
    free_context(context);
    free_ast(ast_root);
    free_parser(parser);
    free_tokens_list(tokens_head);
    free_lexer(lexer);
}


int main(int argc, char *argv[]){
    arguments_check(argc, argv);
    
    char *filename = argv[1];
    
    run_compiler(filename);

    return 0;
}
