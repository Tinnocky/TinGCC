#include "../include/lexer.h"
#include "../include/utils.h"


Keyword keywords[] = {
    {"create", CREATE}, {"as", AS}, {"start", START_SCOPE}, {"end", END_SCOPE},
    {"if", IF}, {"else", ELSE}, {"while", WHILE}, {"stop", STOP}, {"skip", SKIP}, 
    {"say", SAY}, {"input", INPUT}, {"with", WITH}, {"return", RETURN}, 
    {"true", TRUE}, {"false", FALSE}, {"and", AND}, {"or", OR},

    {"int", INT}, {"float", FLOAT}, {"char", CHAR}, {"list", LIST}, {"string", STRING},

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


void close_lexer(Lexer *lexer){
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

        // break at EOF
        if (token_string[0] == '\0') {
            // add EOF token to end of file
            Token *current_token = malloc(sizeof(Token));
            current_token->type = EOF_TOKEN;
            current_token->line = lexer->line;
            current_token->string = "";
            current_token->length = 0;
            free(token_string);
            ast->token = *current_token;
            free(current_token);

            break; // end of file or just spaces
        }

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
    // KEYWORDS, TYPES, AND, OR, PUNCTUATION, DELIMITERS, ASSIGNMENT, check in keywords[] for specifics
    int word = 0;
    while (keywords[word].string != NULL){
        if (strcmp(keywords[word].string, token_string) == 0){
            return keywords[word].type;
        }
        word++;
    }

    // check if the token is a multiword-token
    TokenType multiword_check_output = determine_multiword_tokens(lexer, token_string, token_length);
    if (multiword_check_output != -2){
        return multiword_check_output;
    }

    // check if the token is a number
    TokenType number_check_output = determine_number_token(lexer, token_string, token_length);
    if (number_check_output != -2){
        return number_check_output;
    }

    // failed all other checks. must be an identifier
    if (token_string[0] == "_" || isalpha(token_string[0])){
        if (token_length > 1){
            for (int i = 1; i < token_length; i++){
                if (token_string[i] != "_" && !isalpha(token_string[i]) && !isdigit(token_string[i])){
                    return UNKNOWN; // because identifiers can only have alphas, digits and _.
                }
            }
        }
        return IDENTIFIER;
    }

    return UNKNOWN;
}


TokenType determine_multiword_tokens(Lexer *lexer, const char *token_string, const int token_length){
    // check if the token is repeat/repeat on and multiword boolean operators
    // REPEAT AND REPEAT ON 
    if (strcmp(token_string, "repeat") == 0){
        // check for repeat on
        if (match_expected_token(lexer, "on")){
            consume_filler_token(lexer);
            return REPEAT_ON;
        }
        return REPEAT;
    }
        
    // IS AND IS NOT
    if (strcmp(token_string, "is") == 0){
        // check for is not
        if (match_expected_token(lexer, "not")){
            consume_filler_token(lexer);
            return IS_NOT;
        }
        return IS;
    }

    // NOT LESS THAN AND NOT MORE THAN
    if (strcmp(token_string, "not") == 0){
        // not less than
        if (match_expected_token(lexer, "less")){
            consume_filler_token(lexer);

            if (match_expected_token(lexer, "than")){
                consume_filler_token(lexer);
                return NOT_LESS_THAN;
            }
            return UNKNOWN;
        }
        // not more than
        else if (match_expected_token(lexer, "more")){
            consume_filler_token(lexer);

            if (match_expected_token(lexer, "than")){
                consume_filler_token(lexer);
                return NOT_MORE_THAN;
            }
            return UNKNOWN;
        }
        return UNKNOWN;
    }

    // LESS THAN
    if (strcmp(token_string, "less") == 0){
        if (match_expected_token(lexer, "than")){
            consume_filler_token(lexer);
            return LESS_THAN;
        }
        return UNKNOWN;
    }

    // MORE THAN
    if (strcmp(token_string, "more") == 0){
        if (match_expected_token(lexer, "than")){
            consume_filler_token(lexer);
            return MORE_THAN;
        }
        return UNKNOWN;
    }

    //wasnt any of these
    return NOT_FOUND;
}

TokenType determine_number_token(Lexer *lexer, const char *token_string, const int token_length){
    /* check if token is an int or a float or neither. the + or - token check
        was already done before. */
    if (is_digit(token_string[0]) || token_string[0] == '+' || token_string[0] == '-'){
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



// get next string and such
char *get_token_string(Lexer *lexer, int *token_length, int *token_size, bool *is_string){
    /* gets the next token in the file as a string */
    char *token_string = malloc(*token_size);
    check_nullptr(token_string, "Lexer - Malloc for a token string failed. \n");
    skip_unnecessary(lexer);

    // next token is a string or character
    if (lexer->current_char == '\'' || lexer->current_char == '\"'){
        char opening = lexer->current_char;
        lexer->current_char = fgetc(lexer->file); // skip first ' or "

        while (lexer->current_char != EOF &&
            ((opening == '\'' && lexer->current_char != '\'') || 
             (opening == '\"' && lexer->current_char != '\"'))) {

            token_string[(*token_length)++] = lexer->current_char;

            // realloc if needed
            if (*token_length == *token_size){
                *token_size *= 2;
                token_string = realloc(token_string, *token_size);
                check_nullptr(token_string, "Lexer - Realloc for a token string failed\n");
            }
            lexer->current_char = fgetc(lexer->file);
        }

        if (lexer->current_char == EOF){
            print_error("Lexer - String not closed\n");
        }

        lexer->current_char = fgetc(lexer->file); // skip '
        token_string[*token_length] = '\0';
        *is_string = true; // mark as a string (or character)
        return token_string; // return inside of string or character
    }

    // next token is a not a string or character
    while (lexer->current_char != ' ' && lexer->current_char != '\t' && lexer->current_char != '\n' && 
        lexer->current_char != EOF && lexer->current_char != ','){
        token_string[(*token_length)++] = lexer->current_char;

        // realloc if needed
        if (*token_length == *token_size){
            *token_size *= 2;
            token_string = realloc(token_string, *token_size);
            check_nullptr(token_string, "Lexer - Realloc for a token string failed\n");
        }
        lexer->current_char = fgetc(lexer->file);
    }

    token_string[*token_length] = '\0'; // null-terminate
    return token_string;
}


void skip_unnecessary(Lexer *lexer){
    /* skips unnecessary characters in the file such as whitespaces and comments. */
    // skip whitespace
    bool finished_skipping = false;
    while(!finished_skipping){

        while (lexer->current_char == ' ' || lexer->current_char == '\t' || lexer->current_char == '\n') {
            lexer->current_char = fgetc(lexer->file);
        }
     
        // skip comments
        if (lexer->current_char == '|'){
            lexer->current_char = fgetc(lexer->file); // skip first |
        
            while (lexer->current_char != '|' && lexer->current_char != EOF){
                if (lexer->current_char == '\n'){
                    lexer->line++; // still increment using \n for comments
                }
                lexer->current_char = fgetc(lexer->file);
            }
    
            if (lexer->current_char == EOF){
                print_error("Lexer - Comment not closed \n");
            }

            lexer->current_char = fgetc(lexer->file); // skip ending |
            continue;
        }

        finished_skipping = true;
    }
}


char *peek_token_string(Lexer *lexer, int *token_length, int *token_size, bool *is_string){
    Lexer saved_lexer = *lexer; // copy current lexer character

    char *token_string = get_token_string(lexer, token_length, token_size, is_string);

    *lexer = saved_lexer; // restore lexer to original
    return token_string;
}


bool match_expected_token(Lexer *lexer, const char *expected_string){
    int token_size = INITIAL_TOKEN_LENGTH;
    int token_length = 0;
    bool is_string = false;
    char *token_string = peek_token_string(lexer, &token_length, &token_size, &is_string);
    
    if (strcmp(token_string, expected_string) == 0){
        free(token_string);
        return true;
    }
    free(token_string);
    return false;
}


void consume_filler_token(Lexer *lexer){
    // eats the next string in the file and frees it immediatly after.
    int filler_length, filler_size;
    bool filler_is_string;
    char *filler_token = get_token_string(lexer, &filler_length, &filler_size, &filler_is_string); // eat next token cuz we found it
    free(filler_token); // free because we dont use it
}