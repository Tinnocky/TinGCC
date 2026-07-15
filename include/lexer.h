#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdbool.h>


#define INITIAL_TOKEN_LENGTH 10


// all possible tokens valued as an enum TokenType
typedef enum {
    /* KEYWORDS */
    CREATE_TOKEN,           /* create variable */
    AS_TOKEN,               /* declare type of variable */
    START_SCOPE_TOKEN,      /* open scope */
    END_SCOPE_TOKEN,        /* end scope */
    IF_TOKEN,               /* if statement */
    ELSE_TOKEN,             /* else statement */
    WHILE_TOKEN,            /* while loop */
    REPEAT_TOKEN,           /* for */
    TO_TOKEN,               /* in a repeat, the range */
    STEP_TOKEN,             /* in a repeat, the steps */
    ON_TOKEN,               /* for each, must appear after repeat token */
    IN_TOKEN,               /* in repeat on, declare an index IN list */
    STOP_TOKEN,             /* break */
    SKIP_TOKEN,             /* continue */
    SAY_TOKEN,              /* print */
    WITH_TOKEN,             /* shows function parameters */
    RETURN_TOKEN,           /* return */
    TRUE_TOKEN,             /* boolean true */
    FALSE_TOKEN,            /* boolean false */

    /* TYPES */
    INT_TOKEN,              /* integer type */
    FLOAT_TOKEN,            /* floating-point type */
    CHAR_TOKEN,             /* character type */
    LIST_TOKEN,             /* array with fixed type and length */
    STRING_TOKEN,           /* a char list, dynamic */
    BOOL_TOKEN,             /* true or false */
    VOID_TOKEN,             /* means nothing, should only be applied to function that do not return (i think) */

    /* BOOLEAN OPERATORS */
    IS_TOKEN,               /* == */
    NOT_TOKEN,              /* != */
    MORE_TOKEN,             /* x > */
    LESS_TOKEN,             /* x < */
    THAN_TOKEN,             /* shows up after MORE or LESS */
    AND_TOKEN,              /* && */
    OR_TOKEN,               /* || */

    /* ARITHMETIC OPERATORS */
    PLUS_TOKEN,             /* + */
    MINUS_TOKEN,            /* - */
    MULT_TOKEN,             /* * */
    DIVIDE_TOKEN,           /* / */
    MODULO_TOKEN,           /* % */
    ADD_TO_TOKEN,           /* += */
    SUB_TO_TOKEN,           /* -= */
    MULT_TO_TOKEN,          /* *= */
    DIVIDE_TO_TOKEN,        /* /= */
    MOD_TO_TOKEN,           /* %= */

    /* ASSIGNMENT */
    ASSIGN_TOKEN,           /* = */
 
    /* PUNCTUATION, DELIMITERS */
    OPEN_PAREN_TOKEN,       /* (  — function calls, ifs, whiles, etc. */
    CLOSE_PAREN_TOKEN,      /* ) */
    OPEN_BRACKET_TOKEN,     /* [  — function params, list init, etc. */
    CLOSE_BRACKET_TOKEN,    /* ] */
    END_OF_LINE_TOKEN,      /* \n  — end of statement */
    COLON_TOKEN,            /* :  — various uses */
    COMMA_TOKEN,            /* ,  — various uses */

    /* LITERALS */
    INTEGER_LITERAL_TOKEN,  /* integer constant */
    FLOAT_LITERAL_TOKEN,    /* floating-point constant */
    CHAR_LITERAL_TOKEN,     /* character constant */
    STRING_LITERAL_TOKEN,   /* string constant */

    /* IDENTIFIERS */
    IDENTIFIER_TOKEN,       /* variable/function name */

    /* SPECIAL */
    EOF_TOKEN,              /* end of file */
    UNKNOWN_TOKEN = -1,
    NOT_FOUND_TOKEN = -2
} TokenType;


// keeps all of the relevant data about a token
typedef struct {
    TokenType type;
    char *string;
    int length;
    int line;
} Token;


typedef struct TokenNode TokenNode; // forward declaration of the TokenNode so we can get its *next TokenNode.
// contains a token and a pointer to the next token from the read .ting file
struct TokenNode{
    Token token;
    TokenNode *next;
};


typedef struct Lexer Lexer; // because its being forward declared in utils.h
struct Lexer {
    FILE *file;
    int line;
    int current_char; // character from the file stream the lexer currently holds. is an int because it might hold EOF
    int bracket_depth; // if we are inside an unclosed bracket
    bool inside_interp_string; // if we were inside an interpolated string, we need to come back to collect it
    char interp_quote_type; // to know what ends the string
    int interp_bracket_depth; // bracket_depth value when interpolation [ was opened
};


// public function declarations
/* ----- TokenNode "methods" ----- */
void free_tokens_list(TokenNode *token_node);

/* ----- Lexer "methods" ----- */
Lexer *init_lexer(const char *filename);
void close_lexer(Lexer *lexer);
TokenNode *run_lexer(Lexer *lexer);



#endif