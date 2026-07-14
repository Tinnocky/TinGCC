#ifndef ANALYSIS_H
#define ANALYSIS_H

#include <stdbool.h>
#include "parser.h"


#define SYMBOL_TABLE_LENGTH 97 // a prime number to better fit the hash function


typedef enum {
    SYMBOL_VAR,
    SYMBOL_FUNC
} SymbolKind;


// one function parameter's data
typedef struct ParamInfo ParamInfo;
struct ParamInfo {
    char *name;
    TypeInfo *type_info;
    bool has_default;
    ParamInfo *next;
};


// one symbol's data
typedef struct {
    char *name;
    SymbolKind kind;
    int line; // line it was declared on 
    union {
        struct {
            TypeInfo *type_info;
            bool is_initialized; // if it has a value or holds null
        } var;

        struct {
            TypeInfo *return_type_info;
            ParamInfo *params; // info about the function parameters
        } func;
    } data;
    
} Symbol;


// holds a linked list of symbols
// to be used in the hashmap, if the hash returned an index already taken by another symbol, then
// we append it to the linked list. handles index collision by making lookup traverse the short linked list 
typedef struct LinkedSymbol LinkedSymbol;
struct LinkedSymbol {
    Symbol *symbol;
    LinkedSymbol *next;
};


// hashmap holding a linked list of symbols
typedef struct {
    LinkedSymbol *table[SYMBOL_TABLE_LENGTH]; // hashmap of all symbols
} SymbolTable;


// a linked list (really the stack) of symbol tables
typedef struct Scope Scope;
struct Scope {
    SymbolTable *table;
    Scope *parent; // next in the linked list, lower in the stack.
};


// the main structure that holds everything and is passed
typedef struct {
    Scope *current_scope; // top of the scope stack (always point to the top as its a stack)
    TypeInfo *current_return_type_info; // the return type of the function we are currently inside of (null at top level)
    bool is_inside_loop;
} Context;


// public function declarations
/* ----- Symbol "Methods" ----- */
void free_symbol(Symbol *symbol);

/* ----- SymbolTable "Methods" ----- */
void free_symbol_table(SymbolTable *table);

/* ----- Scope "Methods" (only reachable through Context) ----- */
void free_scope_stack(Context *context);

/* ----- Context "Methods" (along with scope methods, technically) ----- */
Context *init_context(void);

/* ----- "Main" Analysis functions ----- */
void run_analysis(Context *context, ASTNode *ast_root);


#endif