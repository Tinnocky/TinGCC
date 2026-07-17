#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include "lexer.h" // for TokenType


typedef enum {
    /* MAIN */
    PROGRAM_NODE,            /* occupying the whole program */

    /* FUNCTION RELATED */
    FUNCTION_NODE,           /* a function definition and body */
    FUNCTION_PARAM_NODE,     /* the parameters list that the function expects. */
    FUNCTION_CALL_NODE,      /* a call to a function */

    /* VARIABLE RELATED */
    CREATE_VAR_NODE,         /* creating a variable, optionally also assigning it a value */
    ASSIGNMENT_NODE,         /* assign a value to an existing variable */

    /* OPERATE ON CONDITION */
    IF_NODE,                 /* an if statement condition and body, also holds all of its else statements in a list */
    ELSE_NODE,               /* can both have a condition (like elif) and not have any condition if its at the bottom */
    WHILE_NODE,              /* a while statement condition and body */
    REPEAT_NODE,             /* a repeat statement condition and body */
    REPEAT_ON_NODE,          /* a repeat_on statement condition and body*/

    /* COMMAND-LIKE */
    SAY_NODE,                /* holds an expression. should also parse interpolated strings from the expression */
    INPUT_NODE,              /* holds a scalar type */
    RETURN_NODE,             /* holds the expression to return */
    STOP_NODE,               /* just the word "stop" */
    SKIP_NODE,               /* just the word "skip" */

    /* EXPRESSIONS */
    // Note: all share the same exact process in the parser
    ARITHMETIC_EXPR_NODE,    /* a + b, a * b, ... */
    COMPARISON_EXPR_NODE,    /* a less than b, a is b, ... */
    LOGICAL_EXPR_NODE,       /* expr_a and expr_b, expr_a or expr_b, ... */

    /* OTHER */
    UNARY_NODE,              /* both NOT and -1 (unary operator) */
    INDEX_NODE,              /* holds the position of an array or something like that */
    IDENTIFIER_NODE,         /* variable/function names */
    LITERAL_NODE,            /* some sort of int, float, char, string or bool. */
    LIST_LITERAL_NODE,       /* an actual list (array) of stuff */
} NodeType;


typedef enum {
    TYPE_INT, 
    TYPE_FLOAT, 
    TYPE_CHAR, 
    TYPE_STRING, 
    TYPE_LIST,
    TYPE_BOOL,
    TYPE_VOID
} Type;


typedef struct TypeInfo TypeInfo;
struct TypeInfo {
    Type type;
    TypeInfo *inner; // only used for lists, the thing directly inside (can nest: int list list -> list(list(int)))
};


typedef struct ASTNode ASTNode; // forward declaration of ASTNode for LinkedASTNode


// stores a linked list of ASTNodes, to store statements in order of appearance
// such as an if_statement's body, it consists of a list of other statements inside it
typedef struct LinkedASTNode LinkedASTNode; // so we can get its LinkedASTNOde *next
struct LinkedASTNode {
    ASTNode *node; // the statement itself
    LinkedASTNode *next; // pointer to the next statement
};


// one statement, its type and data.
struct ASTNode {
    NodeType node_type;
    TypeInfo *type_info; // contains the type of certain nodes that need it, added in analysis stage
    int line; // used for adding the line in errors at later stages of compilation
    union { // containing the data structures for all NodeTypes (that carry data). only one should be in use for each node.
        /* MAIN */
        struct {
            LinkedASTNode *statements; // top-level statements (functions or global stuff) in order
        } program;


        /* FUNCTION RELATED */
        struct {
            TypeInfo *return_type_info;
            char *name;
            LinkedASTNode *params; // list of function_params
            LinkedASTNode *body;
        } function;

        struct { 
            char *name;
            TypeInfo *type_info;
            ASTNode *default_val; // assigned where no parameter is passed to the function call. can be an expression too! (arithmetic)
        } function_param;

        struct { 
            char *name; 
            LinkedASTNode *params; // a list of expressions
        } function_call;


        /* VARIABLE RELATED */
        struct {
            char *name;
            TypeInfo *type_info;
            ASTNode *value; // literal or arithmetic expression (optional)
        } create_var;

        struct {
            char *name; // name of an identifier
            ASTNode *index_expr; // optional, for assigning value to a list
            TokenType assign_op; // such as =, +=, *=, ...
            ASTNode *value; // literal or arithmetic
        } assignment;

        /* OPERATE ON CONDITION */
        struct {
            ASTNode *condition; // an expression (comparison/logical)
            LinkedASTNode *else_branch; // all else statements following this in order
            LinkedASTNode *body; // list of statements
        } if_statement;

        struct { 
            ASTNode *condition; // an expression (comparison/logical)
            LinkedASTNode *body;  // list of statements
        } else_statement;

        struct { 
            ASTNode *condition; // an expression (comparison/logical)
            LinkedASTNode *body;  // list of statements
        } while_loop;

        struct {
            char *var_name;  // the loop variable (always new variable just for the loop)
            ASTNode *from;   // start expression
            ASTNode *to;     // end expression
            ASTNode *step;   // optional step expression (NULL = 1)
            LinkedASTNode *body;  // list of statements
        } repeat;

        struct { 
            char *var_name;
            char *list_name;
            LinkedASTNode *body;  // list of statements
        } repeat_on;


        /* COMMAND-LIKE */
        struct {
            LinkedASTNode *values; // an expression to print or a linked list of string + interpolated variables
        } say;

        struct {
            Type type;
        } input;

        struct { 
            ASTNode *value; // any expression to return (optional cuz of void)
        } return_statement;

        // Note: input, stop and skip do not carry data so theyre not here


        /* EXPRESSIONS */
        struct {
            ASTNode *left_val;
            TokenType op;
            ASTNode *right_val;
        } expression; // for both ARITHMETIC_EXPR, COMPARISON_EXPR and LOGICAL_EXPR


        /* OTHER */
        struct {
            TokenType op; // either NOT or MINUS
            ASTNode *operand; // the thing being operated on
        } unary;

        struct {
            char *list_name; // list being indexed
            ASTNode *index_expr; // the expression inside the brackets (the index itself)
        } index;

        struct { // Note: this is only used in parse_atom because it has to return a plain identifier and has to return an *ASTNode
            char *name; 
        } identifier;

        struct {
            Type type; // literals (not list literals) never might be list, thus TypeInfo * is useless here
            char *value;
        } literal;

        struct {
            LinkedASTNode *values; // list of expressions
        } list_literal;
    } data;
};


typedef struct {
    TokenNode *tokens_head;
    TokenNode *current_token;
} Parser;


// public function declarations
/* ----- TypeInfo "methods" ----- */
TypeInfo *init_type_info(Type type);

/* ----- AST "methods" ----- */
void free_ast(ASTNode *ast_root);

/* ----- Parser "methods" ----- */
Parser *init_parser(TokenNode *tokens_head);
void close_parser(Parser *parser);

/* ----- "Main" Parsing functions (that mostly redirect to other parsing functions) ----- */
ASTNode *run_parser(Parser *parser); // aka parse program


#endif