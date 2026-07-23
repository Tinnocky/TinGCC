#include <stdlib.h>
#include "../include/lexer.h"
#include "../include/parser.h"
// #include "../include/analysis.h"
#include "../include/utils.h"
#include "../include/testing.h"


// private function declarations
static void arguments_check(int argc, char *argv[]);
static void run_lexer_test(char *filename);
static void run_parser_test(char *filename);
static void run_analysis_test(char *filename);


// check if arguments passed are correct and if the file passed is a .ting file
static void arguments_check(int argc, char *argv[]){
    if (argc != 2){
        print_error("Wrong usage. Should call: %s <filename>\n", argv[0]);
    }

    char *filename = argv[1];
    if (!ends_with(filename, ".ting")){
        print_error("Wrong usage. Should pass file ending with .ting");
    }
}

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

/*
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
    */


int main(int argc, char *argv[]){
    arguments_check(argc, argv);
    
    char *filename = argv[1];

    // which test to run...
    // run_lexer_test(filename);
    run_parser_test(filename);
    // run_analysis_test(filename);

    return 0;
}
