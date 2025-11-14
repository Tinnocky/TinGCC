#ifndef TOKEN_H
#define TOKEN_H


typedef enum {
    /* --- KEYWORDS --- */
    CREATE = 0,          /* create variable */
    AS = 1,              /* declare type of variable */
    START_SCOPE = 2,     /* open scope */
    END_SCOPE = 3,       /* end scope */
    IF = 4,              /* if statement */
    ELSE = 5,            /* else statement */
    WHILE = 6,           /* while loop */
    REPEAT = 7,          /* for */
    REPEAT_ON = 8,       /* for each, must appear after repeat token */
    STOP = 9,            /* break */
    SKIP = 10,           /* continue */
    SAY = 11,            /* print */
    INPUT = 12,          /* input a specific type */
    WITH = 13,           /* shows function parameters */
    RETURN = 14,         /* return */
    TRUE = 15,           /* boolean true */
    FALSE = 16,          /* boolean false */

    /* --- TYPES --- */
    INT = 17,            /* integer type */
    FLOAT = 18,          /* floating-point type */
    CHAR = 19,           /* character type */
    LIST = 20,           /* array with fixed type and length */
    STRING = 21,         /* a char list, dynamic */

    /* --- BOOLEAN OPERATORS --- */
    IS = 22,             /* == */
    IS_NOT = 23,         /* != */
    MORE_THAN = 24,      /* x > */
    NOT_MORE_THAN = 25,  /* x <= */
    LESS_THAN = 26,      /* x < */
    NOT_LESS_THAN = 27,  /* x >= */
    AND = 28,            /* && */
    OR = 29,             /* || */

    /* --- ARITHMETIC OPERATORS --- */
    PLUS = 30,           /* + */
    MINUS = 31,          /* - */
    MULT = 32,           /* * */
    DIVIDE = 33,         /* / */
    MODULO = 34,         /* % */
    INCREMENT = 35,      /* ++ */
    DECREMENT = 36,      /* -- */
    ADD_TO = 37,         /* += */
    SUB_TO = 38,         /* -= */
    MULT_TO = 39,        /* *= */
    DIVIDE_TO = 40,      /* /= */
    MOD_TO = 41,         /* %= */

    /* --- ASSIGNMENT --- */
    ASSIGN = 42,         /* = */

    /* --- PUNCTUATION / DELIMITERS --- */
    OPEN_PAREN = 43,     /* (  — function calls, ifs, whiles, etc. */
    CLOSE_PAREN = 44,    /* ) */
    OPEN_BRACKET = 45,   /* [  — function params, list init, etc. */
    CLOSE_BRACKET = 46,  /* ] */
    END_OF_LINE = 47,    /* ;  — end of statement */
    COLON = 48,          /* :  — various uses */

    /* --- LITERALS --- */
    INTEGER_LITERAL = 49, /* integer constant */
    FLOAT_LITERAL = 50,   /* floating-point constant */
    CHAR_LITERAL = 51,    /* character constant */
    STRING_LITERAL = 52,  /* string constant */
    BOOLEAN_LITERAL = 53, /* boolean constant */

    /* --- IDENTIFIERS --- */
    IDENTIFIER = 54,      /* variable/function name */

    /* --- SPECIAL --- */
    EOF_TOKEN = 55,        /* end of file */
    UNKNOWN = -1,
    NOT_FOUND = -2
} TokenType;


typedef struct {
    char *string;
    TokenType type;
} Keyword;


// array ends with a null so we know when it ends.
extern Keyword keywords[];
/* 
Keyword keywords[] = {
    {"create", CREATE}, {"as", AS}, {"start", START_SCOPE}, {"end", END_SCOPE},
    {"if", IF}, {"else", ELSE}, {"while", WHILE}, {"stop", STOP}, {"skip", SKIP}, 
    {"say", SAY}, {"input", INPUT}, {"with", WITH}, {"return", RETURN}, 
    {"true", TRUE}, {"false", FALSE},

    {"int", INT}, {"float", FLOAT}, {"char", CHAR}, {"list", LIST}, {"string", STRING},

    {"and", AND}, {"or", OR},

    {"=", ASSIGN}, {"+", PLUS}, {"-", MINUS}, {"*", MULT}, {"/", DIVIDE},
    {"%%", MODULO}, {"++", INCREMENT}, {"--", DECREMENT},
    {"+=", ADD_TO}, {"-=", SUB_TO}, {"*=", MULT_TO}, {"/=", DIVIDE_TO}, {"%%=", MOD_TO},

    {"(", OPEN_PAREN}, {")", CLOSE_PAREN}, {"[", OPEN_BRACKET}, {"]", CLOSE_BRACKET},
    {",", END_OF_LINE}, {":", COLON},

    {NULL, UNKNOWN}
};
*/


typedef struct {
    TokenType type;
    char *string;
    int length;
    int line;
} Token;


#endif