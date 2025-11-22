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
    ON = 8,              /* for each, must appear after repeat token */
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
    NOT = 23,            /* != */
    MORE = 24,           /* x > */
    LESS = 25,           /* x < */
    THAN = 26,
    AND = 27,            /* && */
    OR = 28,             /* || */

    /* --- ARITHMETIC OPERATORS --- */
    PLUS = 29,           /* + */
    MINUS = 30,          /* - */
    MULT = 31,           /* * */
    DIVIDE = 32,         /* / */
    MODULO = 33,         /* % */
    INCREMENT = 34,      /* ++ */
    DECREMENT = 35,      /* -- */
    ADD_TO = 36,         /* += */
    SUB_TO = 37,         /* -= */
    MULT_TO = 38,        /* *= */
    DIVIDE_TO = 39,      /* /= */
    MOD_TO = 40,         /* %= */

    /* --- ASSIGNMENT --- */
    ASSIGN = 41,         /* = */

    /* --- PUNCTUATION / DELIMITERS --- */
    OPEN_PAREN = 42,     /* (  — function calls, ifs, whiles, etc. */
    CLOSE_PAREN = 43,    /* ) */
    OPEN_BRACKET = 44,   /* [  — function params, list init, etc. */
    CLOSE_BRACKET = 45,  /* ] */
    END_OF_LINE = 46,    /* ;  — end of statement */
    COLON = 47,          /* :  — various uses */

    /* --- LITERALS --- */
    INTEGER_LITERAL = 48, /* integer constant */
    FLOAT_LITERAL = 49,   /* floating-point constant */
    CHAR_LITERAL = 50,    /* character constant */
    STRING_LITERAL = 51,  /* string constant */
    BOOLEAN_LITERAL = 52, /* boolean constant */

    /* --- IDENTIFIERS --- */
    IDENTIFIER = 53,      /* variable/function name */

    /* --- SPECIAL --- */
    EOF_TOKEN = 54,        /* end of file */
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
    {"true", TRUE}, {"false", FALSE}, {"repeat", REPEAT}, {"on", ON},

    {"int", INT}, {"float", FLOAT}, {"char", CHAR}, {"list", LIST}, {"string", STRING},

    {"is", IS}, {"not", NOT}, {"more", MORE}, {"less", LESS}, {"and", AND}, {"or", OR},

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