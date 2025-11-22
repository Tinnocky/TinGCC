#include "../include/lexer.h"
#include "../include/utils.h"


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


// intialization and main stuff
Lexer *init_lexer(const char *filename){
    Lexer *lexer = malloc(sizeof(Lexer));
    check_nullptr(lexer, "Lexer - Malloc for lexer failed. \n");

    lexer->file = fopen(filename, "r");
    check_nullptr(lexer->file, "Lexer - Could not open file. \n");

    // initialize important info
    lexer->line = 1;
    lexer->current_char = fgetc(lexer->file);

    // start getting tokens!
    return lexer;
}

void advance(Lexer *lexer){
    lexer->current_char = fgetc(lexer->file);
}

void go_back(Lexer *lexer){
    ungetc(lexer->current_char, lexer->file);
}

void close_lexer(const Lexer *lexer){
    fclose(lexer->file);
    free(lexer);
}


void free_ast(ASTNode *ast){
    while (ast){
        ASTNode *next = ast->next;
        free(ast->token.string);
        free(ast);
        ast = next;
    }
}


ASTNode *run_lexer(Lexer *lexer){
    // create AST
    ASTNode *ast = malloc(sizeof(ASTNode));
    check_nullptr(ast, "Lexer - malloc for ASTNode failed. \n");
    ast->next = NULL;
    ASTNode *ast_head = ast;

    // run!
    while (lexer->current_char != EOF){

        // get the next string
        int token_size = INITIAL_TOKEN_LENGTH;
        int token_length = 0;
        bool is_string = false;
        char *token_string = get_token_string(lexer, &token_length, &token_size, &is_string);

        // got full string - time to turn it to a token!*/
        Token *current_token = get_token(lexer, token_string, token_length, is_string);
        free(token_string);

        //store token in ast here
        ast->token = *current_token;
        free(current_token);

        // malloc next node in the AST
        if (lexer->current_char != EOF){
            ASTNode *next = malloc(sizeof(ASTNode));
            check_nullptr(next, "Lexer - malloc for ASTNode failed. \n");
            ast->next = next;
            next->next = NULL;
    
            ast = next;
        }
    }

    return ast_head;
}



// determine token type
TokenType determine_token_type(Lexer *lexer, const char *token_string, const int token_length){
    /* go through each available keyword and token type and determine what type is our token.
    doesnt check strings! that part is handled earlier. */
    // KEYWORDS, TYPES, AND, OR, PUNCTUATION, DELIMITERS, ASSIGNMENT, etc... check in keywords[] for specifics
    int word = 0;
    while (keywords[word].string != NULL){
        if (strcmp(keywords[word].string, token_string) == 0){
            return keywords[word].type;
        }
        word++;
    }

    // check if the token is a number
    TokenType number_check_output = determine_number_token(lexer, token_string, token_length);
    if (number_check_output != -2){
        return number_check_output;
    }

    // failed all other checks. must be an identifier
    if (token_string[0] == '_' || isalpha(token_string[0])){
        if (token_length > 1){
            for (int i = 1; i < token_length; i++){
                if (token_string[i] != '_' && !isalpha(token_string[i]) && !isdigit(token_string[i])){
                    return UNKNOWN; // because identifiers can only have alphas, digits and _.
                }
            }
        }
        return IDENTIFIER;
    }

    return UNKNOWN;
}


TokenType determine_number_token(Lexer *lexer, const char *token_string, const int token_length){
    /* check if token is an int or a float or neither. the + or - token check
        was already done before. */
    if (is_digit(token_string[0])){
        // determine what kind of number - int or float
        TokenType number_type = INTEGER_LITERAL; //start with possible integer
        bool saw_dot = false;

        for (int i = 1; i < token_length; i++){
            if (!saw_dot && token_string[i] == '.'){
                saw_dot = true;
                number_type = FLOAT_LITERAL;
            }
            else if (!is_digit(token_string[i])){
                return UNKNOWN;
            }
        }

        if (token_length == 1 && !is_digit(token_string[0])){
            return UNKNOWN;
        }
        return number_type;
    }
    return NOT_FOUND;
}



// make a token
Token *get_token(Lexer *lexer, const char *token_string, int token_length, bool is_string){
    /* turn the given string into a full token. add needed fields
        and figure out what token type does it represent */
    Token *token = malloc(sizeof(Token));
    check_nullptr(token, "Lexer - Malloc for a token failed. \n");

    // get the token type
    if (is_string){
        // check if its either a string or a character
        if (token_length <= 1){ // char
            token->type = CHAR_LITERAL;
        }
        else if (token_length > 1){
            token->type = STRING_LITERAL;
        }
    }
    else { // not a string or character
        token->type = determine_token_type(lexer, token_string, token_length);
        if (token->type == UNKNOWN){
            print_error("Token %s at line %d is not a viable token. \n", token_string, lexer->line);
        }
    }

    // enter other fields
    token->length = token_length;
    token->line = lexer->line;
    token->string = strndup(token_string, token_length);
    check_nullptr(token->string, "Lexer - Strndup failed \n");

    if (token->type == END_OF_LINE){ //increment line if needed
        lexer->line++;
    }

    return token;
}


char *get_token_string(Lexer *lexer, int *token_length, int *token_size, bool *is_string){
    /* get the next token in the file, as a string. */
    // check for a string or character literal

    // check for multi character keywords

    // check for single character keywords

    // check for words or numbers

    // is probably an identifier now that weve checked everything else
}


void skip_unnecessary(Lexer *lexer){
    /* skips unnecessary stuff such as comments or whitspaces.
        advances lines. */
}


bool read_string_literal(Lexer *lexer, char *token_string, int *token_length, int *token_size){
    /* reads the next token if its in between quotes into *token_string.
        returns bool for if its a string or not. */
}


bool read_operator(Lexer *lexer, char *token_string, int *token_length, int *token_size){
    /* reads the next token operator into *token_string.
        returns bool for if its an operator or not. */
}


bool read_word(Lexer *lexer, char *token_string, int *token_length, int *token_size){
    /* reads the next token into *token_string until a whitespace or delimiter is hit.
        could be numbers, words, etc */
}