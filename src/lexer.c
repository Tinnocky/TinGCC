#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/lexer.h"
#include "../include/utils.h"


// private function declarations
/* ----- Lexer "methods" ----- */
static inline void advance(Lexer *lexer);

/* ----- read characters ----- */
static char *get_token_string(Lexer *lexer, int *token_length, int *token_size, bool *is_literal);
static bool read_interp_string_continuation(Lexer *lexer, char **token_string, int *token_length, int *token_size);
static void skip_unnecessary(Lexer *lexer);
static bool read_string_literal(Lexer *lexer, char **token_string, int *token_length, int *token_size);
static bool read_operator(Lexer *lexer, char *token_string, int *token_length);
static bool read_word(Lexer *lexer, char **token_string, int *token_length, int *token_size);

/* ----- make a token ----- */
static Token *get_token(Lexer *lexer, const char *token_string, int token_length, bool is_literal);
static TokenType determine_token_type(const char *token_string, const int token_length, bool is_literal);
static TokenType determine_number_token_type(const char *token_string, const int token_length);


// initialize importants stuff
typedef struct { // struct containing a keyword and their enum TokenType companion
    char *string;
    TokenType type;
} Keyword;

// array containing all possible keywords and their enum TokenType companion
// ends with a null so we know when it ends.
Keyword keywords[] = {
    {"create", CREATE_TOKEN}, {"as", AS_TOKEN}, {"start", START_SCOPE_TOKEN}, {"end", END_SCOPE_TOKEN},
    {"if", IF_TOKEN}, {"else", ELSE_TOKEN}, {"while", WHILE_TOKEN}, {"stop", STOP_TOKEN}, {"skip", SKIP_TOKEN}, 
    {"say", SAY_TOKEN}, {"with", WITH_TOKEN}, {"return", RETURN_TOKEN}, {"in", IN_TOKEN},
    {"true", TRUE_TOKEN}, {"false", FALSE_TOKEN}, {"repeat", REPEAT_TOKEN}, {"to", TO_TOKEN}, {"step", STEP_TOKEN},
    {"on", ON_TOKEN}, {"than", THAN_TOKEN},

    {"int", INT_TOKEN}, {"float", FLOAT_TOKEN}, {"char", CHAR_TOKEN}, {"list", LIST_TOKEN},
    {"string", STRING_TOKEN}, {"bool", BOOL_TOKEN}, {"void", VOID_TOKEN},

    {"is", IS_TOKEN}, {"not", NOT_TOKEN}, {"more", MORE_TOKEN}, {"less", LESS_TOKEN},
    {"and", AND_TOKEN}, {"or", OR_TOKEN},

    {"=", ASSIGN_TOKEN}, {"+", PLUS_TOKEN}, {"-", MINUS_TOKEN}, {"*", MULT_TOKEN}, {"/", DIVIDE_TOKEN},
    {"%", MODULO_TOKEN}, {"+=", ADD_TO_TOKEN}, {"-=", SUB_TO_TOKEN}, {"*=", MULT_TO_TOKEN}, {"/=", DIVIDE_TO_TOKEN},
    {"%=", MOD_TO_TOKEN},

    {"(", OPEN_PAREN_TOKEN}, {")", CLOSE_PAREN_TOKEN}, {"[", OPEN_BRACKET_TOKEN}, {"]", CLOSE_BRACKET_TOKEN},
    {"\n", END_OF_LINE_TOKEN}, {":", COLON_TOKEN}, {",", COMMA_TOKEN},

    {NULL, UNKNOWN_TOKEN}
};

Lexer *global_lexer = NULL;
TokenNode *global_tokens_head = NULL;


/* ----- TokenNode "methods" ----- */
// Note: the passed *tokens_node should be the head of the linked list.
void free_tokens_list(TokenNode *token_node){
    while (token_node){
        TokenNode *next = token_node->next;

        free(token_node->token.string);
        free(token_node);

        token_node = next;
    }
}


/* ----- Lexer "methods" ----- */
// initialize a new lexer and its fields
// returns the lexer
Lexer *init_lexer(const char *filename){
    Lexer *lexer = malloc(sizeof(Lexer));
    check_nullptr(lexer, "Lexer: Malloc to initialize a lexer failed. \n");
    global_lexer = lexer;

    lexer->file = fopen(filename, "r");
    check_nullptr(lexer->file, "Lexer: Could not open the provided file. \n");

    // initialize lexer fields
    lexer->line = 1;
    lexer->bracket_depth = 0;
    lexer->inside_interp_string = false;
    lexer->interp_quote_type = '\0';
    lexer->interp_bracket_depth = 0;
    advance(lexer); // get the first character

    // start getting tokens!
    return lexer;
}

void close_lexer(Lexer *lexer){
    fclose(lexer->file);
    free(lexer);
}

// consume the next character from the file stream onto lexer->current_char
static inline void advance(Lexer *lexer){
    lexer->current_char = fgetc(lexer->file);
}

// our "main" function for the lexer
// initializes the tokens list
// runs the full lexing loop of getting the next string, turning it to a token and storing it in the tokens list.
// returns the head of the tokens list
TokenNode *run_lexer(Lexer *lexer){
    // create the tokens list
    TokenNode *token_node = malloc(sizeof(TokenNode));
    check_nullptr(token_node, "Lexer: malloc to initialize the first TokenNode failed. \n");
    token_node->next = NULL;
    TokenNode *tokens_head = token_node;
    global_tokens_head = tokens_head;

    // lexing loop!!!
    while (true){
        // 1. get the next string
        int token_size = INITIAL_TOKEN_LENGTH;
        int token_length = 0;
        bool is_literal = false;

        char *token_string = get_token_string(lexer, &token_length, &token_size, &is_literal);

        if (token_string == NULL){ // got EOF token
            break;
        }

        // 2. got full string, time to turn it to a token!
        Token *current_token = get_token(lexer, token_string, token_length, is_literal);
        free(token_string); // dont need it anymore

        // 3. store token in tokens list here
        token_node->token = *current_token;
        free(current_token); // dont need it anymore

        // 4. malloc next node in the tokens list
        TokenNode *next = malloc(sizeof(TokenNode));
        check_nullptr(next, "Lexer (line %d): malloc to initialize a TokenNode failed. \n", lexer->line);
        token_node->next = next;
        next->next = NULL;
    
        token_node = next;
    }

    // add an EOF token to the end of the tokens list
    // token_node points to the last token now
    token_node->token.type = EOF_TOKEN;
    token_node->token.line = lexer->line;
    token_node->token.string = strdup("EOF");
    check_nullptr(token_node->token.string, "Lexer: strdup for the EOF Token's string failed. \n");
    token_node->token.length = 0;

    return tokens_head;
}


/* ----- read characters ----- */
// get the next full token in file stream as a string
// this function acts as the entry point and will redirect to a few other more focused functions
static char *get_token_string(Lexer *lexer, int *token_length, int *token_size, bool *is_literal){   
    char *token_string = malloc(*token_size * sizeof(char));
    check_nullptr(token_string, "Lexer: malloc for token_string failed.\n");

    // 1. check if were resuming an interpolated string
    if (lexer->inside_interp_string){
        *is_literal = true;
        bool result = read_interp_string_continuation(lexer, &token_string, token_length, token_size);
        if (!result) return NULL;
        return token_string;
    }

    // 2. skip whitespaces and comments
    skip_unnecessary(lexer);
    if (lexer->current_char == EOF){
        if (lexer->interp_quote_type != '\0'){
            print_error("Lexer (line %d): unterminated interpolated string — missing ']' and closing quote. \n", lexer->line);
        }

        free(token_string);
        return NULL;
    }
    
    // 3. check for a string or character literal
    if (read_string_literal(lexer, &token_string, token_length, token_size)){
        *is_literal = true;
        return token_string;
    }

    // 4. check for operators
    if (read_operator(lexer, token_string, token_length)){
        return token_string;
    }

    // 5. check for words or numbers
    if (read_word(lexer, &token_string, token_length, token_size)){
        return token_string;
    }

    free(token_string);
    print_error("Lexer (line %d): Unable to get string token. \n", lexer->line);
    return NULL;
}

static bool read_interp_string_continuation(Lexer *lexer, char **token_string, int *token_length, int *token_size){
    while (lexer->current_char != lexer->interp_quote_type){
        if (lexer->current_char == EOF){
            print_error("Lexer (line %d): string left unterminated until EOF. \n", lexer->line);
            return false;
        }

        if (lexer->current_char == '\n'){
            lexer->line++;
            advance(lexer);
            continue;
        }

        if (lexer->current_char == '['){
            (*token_string)[*token_length] = '\0';
            lexer->inside_interp_string = false;
            lexer->interp_bracket_depth = lexer->bracket_depth;
            return true;
        }

        if (lexer->current_char == '\\'){
            advance(lexer);
            if (lexer->current_char == EOF){
                print_error("Lexer (line %d): string left unterminated until EOF. \n", lexer->line);
                return false;
            }
            switch (lexer->current_char){
                case 'n': (*token_string)[(*token_length)++] = '\n'; break;
                case 't': (*token_string)[(*token_length)++] = '\t'; break;
                default:
                    (*token_string)[(*token_length)++] = '\\';
                    (*token_string)[(*token_length)++] = lexer->current_char;
                    break;
            }
            realloc_check(token_string, token_length, token_size);
            advance(lexer);
            continue;
        }

        (*token_string)[(*token_length)++] = lexer->current_char;
        realloc_check(token_string, token_length, token_size);
        advance(lexer);
    }

    // hit the ending quote — string is done
    advance(lexer);
    (*token_string)[*token_length] = '\0';
    lexer->inside_interp_string = false;
    lexer->interp_quote_type = '\0';
    return true;
}

// skip unnecessary stuff such as comments or whitespaces
// Note: increments lexer->line for multi-line comments or whitespaces.
static void skip_unnecessary(Lexer *lexer){
    bool finished = false;

    while (!finished){
        // check for whitespaces, and newlines ONLY IF bracket_depth >0
        while (lexer->current_char == '\t' || lexer->current_char == ' ' ||
            (lexer->current_char == '\n' && lexer->bracket_depth > 0)){
                if (lexer->current_char == '\n'){
                    lexer->line++; // increment line count without appending the newline token
                }
                advance(lexer);
        }
    
        // check for comments
        if (lexer->current_char == '|'){
            advance(lexer); // skip starting '|'  symbol
    
            while (lexer->current_char != '|'){
                if (lexer->current_char == '\n'){
                    lexer->line++; // increment line count without appending the newline token
                }

                else if (lexer->current_char == EOF){
                    print_error("Lexer (line %d): comment left unterminated until EOF. \n", lexer->line);
                }
    
                advance(lexer);
            }

            advance(lexer); // skip ending '|' symbol
            continue; // go to start of loop to check if there are more whitespaces or commentes
        }
        
        finished = true; // if it got to here means it didnt enter the comments scope
    }
}

// reads the next token if its in between quotes (a string or character literal) into **token_string
// returns bool for if it was a literal or not
// Note: the first check for \n is if for enter was pressed between the string, second is if "\n" was typed (then the string should format)
static bool read_string_literal(Lexer *lexer, char **token_string, int *token_length, int *token_size){
        if (lexer->current_char != '\'' && lexer->current_char != '\"'){
            return false;
        }

        char quote_type = lexer->current_char; // ending quote should be the same type.
        advance(lexer); // skip starting qoute
        
        // gets characters until hits the ending quote
        while (lexer->current_char != quote_type){
            if (lexer->current_char == '\n'){ // increment line count without appending the newline token
                lexer->line++;
                advance(lexer);
                continue; // shouldnt go to the next part
            }

            else if (lexer->current_char == EOF){
                print_error("Lexer (line %d): string left unterminated until EOF. \n", lexer->line);
                return false;
            }

            // getting an interpolated string
            else if (lexer->current_char == '['){
                (*token_string)[*token_length] = '\0';
                lexer->interp_quote_type = quote_type;
                lexer->inside_interp_string = false;
                lexer->interp_bracket_depth = lexer->bracket_depth; // snapshot depth BEFORE this [ is counted
                return true;
            }

            // handling escape sequences written into the string
            else if (lexer->current_char == '\\'){
                advance(lexer);
                
                if (lexer->current_char == EOF){ // could appear 
                    print_error("Lexer (line %d): string left unterminated until EOF. \n", lexer->line);
                    return false;
                }

                switch (lexer->current_char){
                    case 'n':
                        (*token_string)[(*token_length)++] = '\n';
                        break;

                    case 't':
                        (*token_string)[(*token_length)++] = '\t';
                        break;

                    default: // unknown escape sequence, keep backslash and add next character
                        (*token_string)[(*token_length)++] = '\\';
                        (*token_string)[(*token_length)++] = lexer->current_char;
                        break;
                }

                realloc_check(token_string, token_length, token_size);
                advance(lexer);

                continue; // shouldnt go to the next part
            }


            (*token_string)[(*token_length)++] = lexer->current_char;

            realloc_check(token_string, token_length, token_size);
            advance(lexer);
        }

        advance(lexer); // skip ending quote
        
        (*token_string)[*token_length] = '\0';
        return true;
}

// reads the next token operator into *token_string
// returns bool for if it was an operator or not
static bool read_operator(Lexer *lexer, char *token_string, int *token_length){
    // check for bracket operators
    if (lexer->current_char == '(' || lexer->current_char == ')' ||
        lexer->current_char == '[' || lexer->current_char == ']'){
            char c = lexer->current_char;
            token_string[(*token_length)++] = c;
            token_string[*token_length] = '\0';

            if (c == '(' || c == '['){
                lexer->bracket_depth++;
            }
            else {
                lexer->bracket_depth--;

                // resume string only if this ] brings us back to the depth where interpolation started
                if (c == ']' && lexer->interp_quote_type != '\0' && !lexer->inside_interp_string &&
                    lexer->bracket_depth == lexer->interp_bracket_depth){
                    lexer->inside_interp_string = true;
                }
            }

            advance(lexer);
            return true;
        }

    // check for all other one character operators
    if (lexer->current_char == ':' || lexer->current_char == ',' ||
        lexer->current_char == '=' || lexer->current_char == '\n'){
            token_string[(*token_length)++] = lexer->current_char;
            token_string[*token_length] = '\0';
            
            if (token_string[0] == '\n'){
                lexer->line++;
            }
            
            advance(lexer);
            return true;
        }

    if (lexer->current_char == '+'){
        token_string[(*token_length)++] = lexer->current_char;
        advance(lexer);
        if (lexer->current_char == '='){
            token_string[(*token_length)++] = lexer->current_char;
            token_string[*token_length] = '\0';
            advance(lexer);
            return true;
        }
        token_string[*token_length] = '\0';
        return true;
    }

    if (lexer->current_char == '-'){
        token_string[(*token_length)++] = lexer->current_char;
        advance(lexer);
        if (lexer->current_char == '='){
            token_string[(*token_length)++] = lexer->current_char;
            token_string[*token_length] = '\0';
            advance(lexer);
            return true;
        }
        token_string[*token_length] = '\0';
        return true;
    }

    if (lexer->current_char == '*' || lexer->current_char == '/' || lexer->current_char == '%'){
        token_string[(*token_length)++] = lexer->current_char;
        advance(lexer);
        if (lexer->current_char == '='){
            token_string[(*token_length)++] = lexer->current_char;
            token_string[*token_length] = '\0';
            advance(lexer);
            return true;
        }
        token_string[*token_length] = '\0';
        return true;
    }

    return false;
}

// reads the next token into **token_string until a whitespace or delimiter is hit.
// at this point we checked its not something built-in to ting (operators/keywords) or literals
// could be numbers, variable names,  etc
static bool read_word(Lexer *lexer, char **token_string, int *token_length, int *token_size){
    // check for names
    if (isalpha(lexer->current_char) || lexer->current_char == '_'){
        while (isalpha(lexer->current_char) || lexer->current_char == '_' || isdigit(lexer->current_char)){
            (*token_string)[(*token_length)++] = lexer->current_char;
            realloc_check(token_string, token_length, token_size);
    
            advance(lexer);
        }
        
        (*token_string)[*token_length] = '\0';
        return true;
    }

    // check for numbers
    else if (isdigit(lexer->current_char)){
        int saw_dot = false;
        while (isdigit(lexer->current_char) || lexer->current_char == '.'){
            if (lexer->current_char == '.'){
                if (!saw_dot){
                    saw_dot = true;
                }

                else{
                    print_error("Lexer (line %d): Too many dots in a float. \n", lexer->line);
                }
            }

            (*token_string)[(*token_length)++] = lexer->current_char;
            realloc_check(token_string, token_length, token_size);

            advance(lexer);  
        }

        (*token_string)[*token_length] = '\0';
        return true;
    }

    return false;
}


/* ----- make a token ----- */
// turn the given string into a full Token
// add all fields and figure out what token type does it represent
static Token *get_token(Lexer *lexer, const char *token_string, int token_length, bool is_literal){
    Token *token = malloc(sizeof(Token));
    check_nullptr(token, "Lexer (line %d): Malloc to initialize a token failed. \n", lexer->line);

    // find the token type
    token->type = determine_token_type(token_string, token_length, is_literal);

    if (token->type == UNKNOWN_TOKEN){
        print_error("Lexer (line %d): Token '%s' is not a viable token. \n", lexer->line, token_string);
    }


    // found token type. now enter other fields!
    token->length = token_length;
    token->line = lexer->line;
    token->string = strndup(token_string, token_length);
    check_nullptr(token->string, "Lexer (line %d): Strndup for a token's string failed. \n", lexer->line);

    return token;
}

// goes through each keyword and token type in Keywords[] and determines what type our token is
// determines in different ways other than iterating Keywords[] too
static TokenType determine_token_type(const char *token_string, const int token_length, bool is_literal){
    // check if its a string
    if (is_literal){
        // TODO: decide later if all literal lengthed <= 1 should automatically 
        // TODO: be set to CHAR_LITERAL or i should find a different method to do this
        if (token_length <= 1){
            return CHAR_LITERAL_TOKEN;
        }

        else {
            return STRING_LITERAL_TOKEN;
        }
    }

    // check if the token is in Keywords[]
    int word_index = 0;
    while (keywords[word_index].string != NULL){
        if (strcmp(keywords[word_index].string, token_string) == 0){
            return keywords[word_index].type;
        }

        word_index++;
    }

    // check if the token is a number
    TokenType number_check_output = determine_number_token_type(token_string, token_length);
    if (number_check_output != NOT_FOUND_TOKEN){
        return number_check_output;
    }

    // failed all other checks. must be an identifier!
    if (token_string[0] == '_' || isalpha(token_string[0])){
        return IDENTIFIER_TOKEN;
    }

    return UNKNOWN_TOKEN;
}

// check if token is an int or a float or neither
// the + or - sign in start of token (if existed) was already checked before and isnt passed to this
static TokenType determine_number_token_type(const char *token_string, const int token_length){
    if (isdigit(token_string[0])){
        TokenType number_type = INTEGER_LITERAL_TOKEN; //start with possible integer
        bool saw_dot = false; // having a dot means its a float

        for (int i = 1; i < token_length; i++){
            if (!saw_dot && token_string[i] == '.'){
                saw_dot = true;
                number_type = FLOAT_LITERAL_TOKEN;
            }

            else if (!isdigit(token_string[i])){
                return UNKNOWN_TOKEN;
            }
        }

        return number_type;
    }

    return NOT_FOUND_TOKEN; // not even a number
}
