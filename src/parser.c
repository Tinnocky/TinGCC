#include <stdlib.h>
#include <string.h>
#include "../include/parser.h"
#include "../include/utils.h"


// private function declarations
/* ----- TypeInfo "methods" ----- */
static void free_type_info(TypeInfo *);

/* ----- AST "methods" ----- */
static ASTNode *init_ast_node(NodeType type, int line);
static LinkedASTNode *init_linked_ast(ASTNode *ast_node);
static void free_linked_ast(LinkedASTNode *);

/* ----- Parser "methods" ----- */
static inline int current_line(Parser *parser);
static inline TokenType current_type(Parser *parser);
static inline TokenType next_type(Parser *parser);
static inline char *current_string(Parser *parser);
static void advance(Parser *parser);
static void expect_must(Parser *parser, TokenType expected);
static bool expect_optional(Parser *parser, TokenType expected);
static void expect_must_statement_end(Parser *parser);
static bool skip_consecutive_newlines(Parser *parser, TokenType stop_token, TokenType stop_token2);

/* ----- "Main" Parsing functions (that mostly redirect to other parsing functions) ----- */
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_identifier_statement(Parser *parser);

/* ----- Parsing statement functions (that parse one full statement) ----- */
static ASTNode *parse_function(Parser *parser);
static ASTNode *parse_function_call(Parser *parser);
static ASTNode *parse_create_var(Parser *parser);
static ASTNode *parse_assignment(Parser *parser);
static ASTNode *parse_if(Parser *parser);
static ASTNode *parse_else(Parser *parser);
static ASTNode *parse_while(Parser *parser);
static ASTNode *parse_repeat(Parser *parser);
static ASTNode *parse_repeat_on(Parser *parser);
static ASTNode *parse_say(Parser *parser);
static ASTNode *parse_return(Parser *parser);

/* ----- inner parsing functions (that parse parts of statements) ----- */
static ASTNode *parse_function_param(Parser *parser);
static LinkedASTNode *parse_full_scope(Parser *parser);
static LinkedASTNode *parse_body(Parser *parser, TokenType stop_token, TokenType stop_token2);
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_term(Parser *parser);
static ASTNode *parse_factor(Parser *parser);
static ASTNode *parse_atom(Parser *parser);
static ASTNode *parse_bool(Parser *parser);
static ASTNode *parse_or(Parser *parser);
static ASTNode *parse_and(Parser *parser);
static ASTNode *parse_comparison(Parser *parser);
static ASTNode *parse_literal(Parser *parser);
static ASTNode *parse_list_literal(Parser *parser);
static TypeInfo *parse_type(Parser *parser, bool is_function);
static char *parse_identifier(Parser *parser);


// variable declarations
#define PROGRAM_NODE_LINE 0


/* ----- TypeInfo "methods" ----- */
// initalize a new type info node and add it a type
// doesnt add the inner node, each parsing function will
// Note: this is also used in analysis.c
TypeInfo *init_type_info(Type type){
    TypeInfo *new_type_info = calloc(1, sizeof(TypeInfo));
    check_nullptr(new_type_info, "Parser: Malloc to initialize a TypeInfo Node failed. \n");

    new_type_info->type = type;

    return new_type_info;
}

static void free_type_info(TypeInfo *type_info_node){
    if (type_info_node == NULL){
        return;
    }

    if (type_info_node->inner != NULL){
        free_type_info(type_info_node->inner);
    }

    free(type_info_node);
}


/* ----- AST "methods" ----- */
// initialize a new ast node and add it a type
// doesnt add data because every data structure is different. each parsing function will
static ASTNode *init_ast_node(NodeType type, int line){
    ASTNode *new_node = calloc(1, sizeof(ASTNode));
    check_nullptr(new_node, "Parser: Malloc to initialize an AST Node failed. \n");

    new_node->node_type = type;
    new_node->line = line;

    return new_node;
}

void free_ast(ASTNode *ast_node){
    if (ast_node == NULL){
        return;
    }
    
    // free contents on the node, recursively
    switch(ast_node->node_type){
        case PROGRAM_NODE:
            free_linked_ast(ast_node->data.program.statements);
            break;
        
        case FUNCTION_NODE:
            free_type_info(ast_node->data.function.return_type_info);
            free(ast_node->data.function.name);
            free_linked_ast(ast_node->data.function.params);
            free_linked_ast(ast_node->data.function.body);
            break;

        case FUNCTION_PARAM_NODE:
            free(ast_node->data.function_param.name);
            free_type_info(ast_node->data.function_param.type_info);
            if (ast_node->data.function_param.default_val){ // optional
                free_ast(ast_node->data.function_param.default_val);
            }
            break;
        
        case FUNCTION_CALL_NODE:
            free(ast_node->data.function_call.name);
            free_linked_ast(ast_node->data.function_call.params);
            break;

        case CREATE_VAR_NODE:
            free(ast_node->data.create_var.name);
            free_type_info(ast_node->data.create_var.type_info);
            if (ast_node->data.create_var.value){ // optional
                free_ast(ast_node->data.create_var.value);
            }
            break;

        case ASSIGNMENT_NODE:
            free(ast_node->data.assignment.name);
            if (ast_node->data.assignment.index_expr){ // optional
                free_ast(ast_node->data.assignment.index_expr);
            }
            free_ast(ast_node->data.assignment.value);
            break;

        case IF_NODE:
            free_ast(ast_node->data.if_statement.condition);
            free_linked_ast(ast_node->data.if_statement.body);
            if (ast_node->data.if_statement.else_branch){ // optional
                free_linked_ast(ast_node->data.if_statement.else_branch);
            }
            break;

        case ELSE_NODE:
            if (ast_node->data.else_statement.condition){ // optional
                free_ast(ast_node->data.else_statement.condition);
            }
            free_linked_ast(ast_node->data.else_statement.body);
            break;

        case WHILE_NODE:
            free_ast(ast_node->data.while_loop.condition);
            free_linked_ast(ast_node->data.while_loop.body);
            break;

        case REPEAT_NODE:
            free(ast_node->data.repeat.var_name);
            free_ast(ast_node->data.repeat.from);
            free_ast(ast_node->data.repeat.to);
            if (ast_node->data.repeat.step){ // optional
                free_ast(ast_node->data.repeat.step);
            }
            free_linked_ast(ast_node->data.repeat.body);
            break;

        case REPEAT_ON_NODE:
            free(ast_node->data.repeat_on.var_name);
            free(ast_node->data.repeat_on.list_name);
            free_linked_ast(ast_node->data.repeat_on.body);
            break;

        case SAY_NODE:
            free_linked_ast(ast_node->data.say.values);
            break;

        case RETURN_NODE:
            if (ast_node->data.return_statement.value){ // optional
                free_ast(ast_node->data.return_statement.value);
            }
            break;

        case STOP_NODE:
        case SKIP_NODE:
            break;

        case ARITHMETIC_EXPR_NODE:
        case COMPARISON_EXPR_NODE:
        case LOGICAL_EXPR_NODE:
            free_ast(ast_node->data.expression.left_val);
            free_ast(ast_node->data.expression.right_val);
            break;

        case UNARY_NODE:
            free_ast(ast_node->data.unary.operand);
            break;

        case INDEX_NODE:
            free(ast_node->data.index.list_name);
            free_ast(ast_node->data.index.index_expr);
            break;

        case IDENTIFIER_NODE:
            free(ast_node->data.identifier.name);
            break;

        case LITERAL_NODE:
            free(ast_node->data.literal.value);
            break;

        case LIST_LITERAL_NODE:
            free_linked_ast(ast_node->data.list_literal.values);
            break;
    }

    // finally, free the node itself
    free(ast_node);
}

// initializes a new linked ast node with the ast node it holds
static LinkedASTNode *init_linked_ast(ASTNode *ast_node){
    LinkedASTNode *new_node = malloc(sizeof(LinkedASTNode));
    check_nullptr(new_node, "Parser: Malloc to initialize a Linked AST Node failed. \n");

    new_node->node = ast_node;
    new_node->next = NULL;

    return new_node;
}

static void free_linked_ast(LinkedASTNode *linked_ast_node){
    while (linked_ast_node){
        LinkedASTNode *next = linked_ast_node->next;

        free_ast(linked_ast_node->node);
        free(linked_ast_node);

        linked_ast_node = next;
    }
}


/* ----- Parser "methods" ----- */
// initialize a new parser with the list of tokens
// Note: tokens_head should be const, but since its attached to current_token its not
Parser *init_parser(TokenNode *tokens_head){
    Parser *parser = malloc(sizeof(Parser));
    check_nullptr(parser, "Parser: Malloc to initialize a Parser failed. \n");

    parser->tokens_head = tokens_head; // does not change
    parser->current_token = tokens_head;

    return parser;
}

void close_parser(Parser *parser){
    free(parser);
}

// simple helper to return the current examined token's line
static inline int current_line(Parser *parser){
    return parser->current_token->token.line;
}

// simple helper to return the current examined token's type
static inline TokenType current_type(Parser *parser){
    return parser->current_token->token.type;
}

// simple helper to return the next token's type without consuming the current one
static inline TokenType next_type(Parser *parser){
    if (parser->current_token->next != NULL){
        return parser->current_token->next->token.type;
    }

    return EOF_TOKEN; // next token is NULL so this is EOF
}

// simple helper to return the current examined token's string
static inline char *current_string(Parser *parser){
    return parser->current_token->token.string;
}

// consume the next token from the tokens_list to the parser
// prints an error if we try to advance EOF as it shouldnt happen
static void advance(Parser *parser){
    if (parser->current_token->next != NULL){
        parser->current_token = parser->current_token->next;
    } 
    else {
        print_error("Parser (line %d): unexpected EOF while advancing. \n", current_line(parser));
    }
}

// check if the type of the current token inside the parser is the same one as the expected type
// if its not, print an error, as it must be the same. if it is then advance
static void expect_must(Parser *parser, TokenType expected_type){
    if (current_type(parser) != expected_type){
        print_error("Parser (line %d): Expected token type %d, but got %d. \n", current_line(parser), expected_type, current_type(parser));
    }

    advance(parser);
}

// check if the type of the current token inside the parser is the same one as the expected type
// if it is return true and advance, and if its not return false
static bool expect_optional(Parser *parser, TokenType expected_type){
    if (current_type(parser) != expected_type){
        return false;
    }

    advance(parser);
    return true;
}

// check if the type of the current token inside the parser is END_OF_LINE_TOKEN or EOF_TOKEN
// if its not, print an error. if it is then advance
static void expect_must_statement_end(Parser *parser){
    if (current_type(parser) != END_OF_LINE_TOKEN && current_type(parser) != EOF_TOKEN){
        print_error("Parser (line %d): Expected EOL or EOF, but got token %d. \n", current_line(parser), current_type(parser));
    }

    if (current_type(parser) != EOF_TOKEN){
        advance(parser);
    }
}


// skips consecutive newlines in the tokens_list
// returns true if there are more statements to parse, false if scope/program ended
// Note: some functions may require a second stop_token. ones that dont will pass the same token twice
static bool skip_consecutive_newlines(Parser *parser, TokenType stop_token, TokenType stop_token2){
    while (expect_optional(parser, END_OF_LINE_TOKEN));

    if (current_type(parser) == stop_token || current_type(parser) == stop_token2){
        return false;
    }
    
    return true;
}


/* ----- "Main" Parsing functions (that mostly redirect to other parsing functions) ----- */
// the entry point of parsing the tokens list
// creates the ast and parses the program node, until EOF is hit
ASTNode *run_parser(Parser *parser){
    // create the ast root (program node)
    ASTNode *ast_program = init_ast_node(PROGRAM_NODE, PROGRAM_NODE_LINE);

    ast_program->data.program.statements = parse_body(parser, EOF_TOKEN, EOF_TOKEN);

    return ast_program;
}

// parse a stand-alone statement (full statements)
// redirect to the corresponding parsing function
static ASTNode *parse_statement(Parser *parser){
    switch(current_type(parser)){
        // full function. list_token is not a case as the type inside it will be before it
        case INT_TOKEN:
        case FLOAT_TOKEN:
        case CHAR_TOKEN:
        case STRING_TOKEN:
        case BOOL_TOKEN:
        case VOID_TOKEN:
            return parse_function(parser);

        // function call or variable assignment
        case IDENTIFIER_TOKEN:
            return parse_identifier_statement(parser);

        // variable creation
        case CREATE_TOKEN:
            return parse_create_var(parser);

        // if statement (and elses following it too)
        case IF_TOKEN:
            return parse_if(parser);

        // while loop
        case WHILE_TOKEN:
            return parse_while(parser);

        // repeat loop, repeat on loop
        case REPEAT_TOKEN:
            return parse_repeat(parser);

        // say
        case SAY_TOKEN:
            return parse_say(parser);

        // return
        case RETURN_TOKEN:
            return parse_return(parser);

        case STOP_TOKEN: {
            int stop_token_line = current_line(parser); // get here because we advance
            advance(parser);
            expect_must_statement_end(parser);
            return init_ast_node(STOP_NODE, stop_token_line);
        }

        case SKIP_TOKEN: {
            int skip_token_line = current_line(parser); // get here because we advance
            advance(parser);
            expect_must_statement_end(parser);
            return init_ast_node(SKIP_NODE, skip_token_line);
        }

        default:
            print_error("Parser (line %d): Token %d cannot appear at start of statement. \n", current_line(parser), current_type(parser));
    }
}

// redirect to parsing functions that start with an identifier
// function call or variable assignment
static ASTNode *parse_identifier_statement(Parser *parser){
    switch(next_type(parser)){
        // function call
        case OPEN_PAREN_TOKEN:
            return parse_function_call(parser);

        // variable assignment
        case ASSIGN_TOKEN:
        case ADD_TO_TOKEN:
        case SUB_TO_TOKEN:
        case MULT_TO_TOKEN:
        case DIVIDE_TO_TOKEN:
        case MOD_TO_TOKEN:
        case OPEN_BRACKET_TOKEN:
            return parse_assignment(parser);

        default:
            print_error("Parser (line %d): Got token %d, but didn't expect it. \n", current_line(parser), next_type(parser));
    }
}


/* ----- Parsing statement functions (that parse one full statement) ----- */
// <func_def>    ::= <ret_type> IDENTIFIER ( "with" "[" <params> "]" )? "start" ":" <statement>* "end"
static ASTNode *parse_function(Parser *parser){
    ASTNode *new_node = init_ast_node(FUNCTION_NODE, current_line(parser));

    new_node->data.function.return_type_info = parse_type(parser, true); // is_function = true
    new_node->data.function.name = parse_identifier(parser);

    // with [ params ] (optional)
    if (expect_optional(parser, WITH_TOKEN)){
        expect_must(parser, OPEN_BRACKET_TOKEN);

        // args (optional)
        LinkedASTNode *args_tail = new_node->data.function.params; // tail is head as of start

        while(current_type(parser) != CLOSE_BRACKET_TOKEN){
            LinkedASTNode *linked_arg = init_linked_ast(parse_function_param(parser));

            if (new_node->data.function.params == NULL){ // for first arg
                new_node->data.function.params = linked_arg;
            }
            else {
                args_tail->next = linked_arg;
            }

            args_tail = linked_arg;

            // after argument can only come , or ]. the while condition already handles the ] case
            if (!expect_optional(parser, COMMA_TOKEN)){
                break;
            }
        } 
        
        expect_must(parser, CLOSE_BRACKET_TOKEN);
    }

    new_node->data.function.body = parse_full_scope(parser);

    return new_node;
} 

// parse a full function call with possible arguments inside
// <call>        ::= IDENTIFIER "(" <args> ")"
// <args>        ::= ( <expr> ( "," <expr> )* )?
static ASTNode *parse_function_call(Parser *parser){
    ASTNode *new_node = init_ast_node(FUNCTION_CALL_NODE, current_line(parser));

    new_node->data.function_call.name = parse_identifier(parser);
    expect_must(parser, OPEN_PAREN_TOKEN);

    // args (optional)
    LinkedASTNode *args_tail = new_node->data.function_call.params; // tail is head as of start

    while(current_type(parser) != CLOSE_PAREN_TOKEN){
        LinkedASTNode *linked_arg = init_linked_ast(parse_expression(parser));

        if (new_node->data.function_call.params == NULL){ // for first arg
            new_node->data.function_call.params = linked_arg;
        }
        else {
            args_tail->next = linked_arg;
        }

        args_tail = linked_arg;

        // after argument can only come , or ). the while condition already handles the ) case
        if (!expect_optional(parser, COMMA_TOKEN)){ // no comma means there are no more args
            break;
        }

        // getting here means there was a comma right before, so there shouldnt be a parentheses
        if (current_type(parser) == CLOSE_PAREN_TOKEN){
            print_error("Parser (line %d): Expected another argument but got a closing parentheses. \n", current_line(parser));
        }
    }

    expect_must(parser, CLOSE_PAREN_TOKEN);

    return new_node;
}

// parse a create variable statement, optionally with assignment too
// <var_decl>    ::= "create" IDENTIFIER "as" <type> ( "=" <expr> )? "\n"
static ASTNode *parse_create_var(Parser *parser){
    ASTNode *new_node = init_ast_node(CREATE_VAR_NODE, current_line(parser));

    expect_must(parser, CREATE_TOKEN); 
    new_node->data.create_var.name = parse_identifier(parser);
    expect_must(parser, AS_TOKEN);
    new_node->data.create_var.type_info = parse_type(parser, false); // is_function = false

    // = <expr> (optional)
    if (expect_optional(parser, ASSIGN_TOKEN)){
            new_node->data.create_var.value = parse_expression(parser);
    }

    expect_must_statement_end(parser);

    return new_node;
}

// parse an assignment statement (could be =, +=, *=, ...)
// <assign>      ::= <lvalue> <assign_op> <expr> "\n"
// <lvalue>      ::= IDENTIFIER ( "[" <expr> "]" )?
static ASTNode *parse_assignment(Parser *parser){
    ASTNode *new_node = init_ast_node(ASSIGNMENT_NODE, current_line(parser));

    new_node->data.assignment.name = parse_identifier(parser);

    // optional index expression (inside the lvalue)
    if (expect_optional(parser, OPEN_BRACKET_TOKEN)){
        new_node->data.assignment.index_expr = parse_expression(parser);
        expect_must(parser, CLOSE_BRACKET_TOKEN);
    }

    // assign op
    switch (current_type(parser)){
        case ASSIGN_TOKEN:
        case ADD_TO_TOKEN:
        case SUB_TO_TOKEN:
        case MULT_TO_TOKEN:
        case DIVIDE_TO_TOKEN:
        case MOD_TO_TOKEN:
            new_node->data.assignment.assign_op = current_type(parser);
            advance(parser);
            break;

        default:
            print_error("Parser (line %d): expected assignment operator. \n", current_line(parser));
    }

    new_node->data.assignment.value = parse_expression(parser);
    expect_must_statement_end(parser);

    return new_node;
}

// parse an if statements, with all of its following else branches
// <if>          ::= "if" "(" <bool> ")" "start" ":" <statement>* <else_branch>* "end"
// Note: parse_full_scope isn't useful here as there could be an else_branch inside the scope
static ASTNode *parse_if(Parser *parser){
    ASTNode *new_node = init_ast_node(IF_NODE, current_line(parser));

    expect_must(parser, IF_TOKEN);
    expect_must(parser, OPEN_PAREN_TOKEN);
    new_node->data.if_statement.condition = parse_bool(parser);
    expect_must(parser, CLOSE_PAREN_TOKEN);
    expect_must(parser, START_SCOPE_TOKEN);    
    expect_must(parser, COLON_TOKEN);
    new_node->data.if_statement.body = parse_body(parser, ELSE_TOKEN, END_SCOPE_TOKEN);

    // else branch
    LinkedASTNode *elses_tail = new_node->data.if_statement.else_branch;

    while(current_type(parser) != END_SCOPE_TOKEN){
        LinkedASTNode *linked_else = init_linked_ast(parse_else(parser));

        if (new_node->data.if_statement.else_branch == NULL){ // first node
            new_node->data.if_statement.else_branch = linked_else;
        }
        else {
            elses_tail->next = linked_else;
        }

        elses_tail = linked_else;
    }

    expect_must(parser, END_SCOPE_TOKEN);
    
    return new_node;
}

// <else_branch> ::= "else" ( "(" <bool> ")" )? "start" ":" <statement>*
// Note: parse_full_scope isn't useful here because the scope doesnt end with END
static ASTNode *parse_else(Parser *parser){
    ASTNode *new_node = init_ast_node(ELSE_NODE, current_line(parser));

    expect_must(parser, ELSE_TOKEN);
    
    // ( <bool> ) (optional)
    if (expect_optional(parser, OPEN_PAREN_TOKEN)){
        new_node->data.else_statement.condition = parse_bool(parser);
        expect_must(parser, CLOSE_PAREN_TOKEN);
    }

    expect_must(parser, START_SCOPE_TOKEN);
    expect_must(parser, COLON_TOKEN);
    new_node->data.else_statement.body = parse_body(parser, ELSE_TOKEN, END_SCOPE_TOKEN);
    
    return new_node;
}

// <while>       ::= "while" "(" <bool> ")" "start" ":" <statement>* "end"
static ASTNode *parse_while(Parser *parser){
    ASTNode *new_node = init_ast_node(WHILE_NODE, current_line(parser));

    expect_must(parser, WHILE_TOKEN);
    expect_must(parser, OPEN_PAREN_TOKEN);
    new_node->data.while_loop.condition = parse_bool(parser);
    expect_must(parser, CLOSE_PAREN_TOKEN);
    new_node->data.while_loop.body = parse_full_scope(parser);

    return new_node;
}

// parse a repeat or repeat_on statement
// if its a repeat_on then redirect to the parse_repeat_on function and return it
// <repeat>      ::= "repeat" "(" IDENTIFIER "," <expr> "to" <expr> ("," "step" <expr>)? ")" "start" ":" <statement>* "end"
static ASTNode *parse_repeat(Parser *parser){
    if (next_type(parser) == ON_TOKEN){
        return parse_repeat_on(parser);
    }

    ASTNode *new_node = init_ast_node(REPEAT_NODE, current_line(parser));

    expect_must(parser, REPEAT_TOKEN);
    expect_must(parser, OPEN_PAREN_TOKEN);
    new_node->data.repeat.var_name = parse_identifier(parser);
    expect_must(parser, COMMA_TOKEN);
    new_node->data.repeat.from = parse_expression(parser);
    expect_must(parser, TO_TOKEN);
    new_node->data.repeat.to = parse_expression(parser);

    // step count (optional)
    // Note: no written step count defaults to 1, this node will stay NULL
    // Note: meaning that when we handle it, having repeat.step = NULL is equivalent to = 1
    if (expect_optional(parser, COMMA_TOKEN)){
        expect_must(parser, STEP_TOKEN);
        new_node->data.repeat.step = parse_expression(parser);
    }

    expect_must(parser, CLOSE_PAREN_TOKEN);
    new_node->data.repeat.body = parse_full_scope(parser);

    return new_node;
}

// parse a repeat_on statement specifically
// <repeat_on>   ::= "repeat" "on" "(" IDENTIFIER "in" IDENTIFIER ")" "start" ":" <statement>* "end"
static ASTNode *parse_repeat_on(Parser *parser){
    ASTNode *new_node = init_ast_node(REPEAT_ON_NODE, current_line(parser));

    expect_must(parser, REPEAT_TOKEN);
    expect_must(parser, ON_TOKEN);
    expect_must(parser, OPEN_PAREN_TOKEN);
    new_node->data.repeat_on.var_name = parse_identifier(parser);
    expect_must(parser, IN_TOKEN);
    new_node->data.repeat_on.list_name = parse_identifier(parser);
    expect_must(parser, CLOSE_PAREN_TOKEN);
    new_node->data.repeat_on.body = parse_full_scope(parser);
    
    return new_node;
}

// parse a say statement
// if it prints a string, check for any interpolated variables and save them
// <say>         ::= "say" <expr> "\n"
static ASTNode *parse_say(Parser *parser){
    ASTNode *new_node = init_ast_node(SAY_NODE, current_line(parser));
    expect_must(parser, SAY_TOKEN);

    LinkedASTNode *values_head = NULL;
    LinkedASTNode *values_tail = NULL;

    // first part: either a string chunk, or a plain expression (say x + 1)
    ASTNode *first = parse_expression(parser);
    values_head = init_linked_ast(first);
    values_tail = values_head;

    // if the lexer split an interpolated string, we now see [ expr ] "next chunk" pairs
    while (current_type(parser) == OPEN_BRACKET_TOKEN){
        advance(parser); // consume [

        ASTNode *expr = parse_expression(parser);
        LinkedASTNode *linked_expr = init_linked_ast(expr);
        values_tail->next = linked_expr;
        values_tail = linked_expr;

        expect_must(parser, CLOSE_BRACKET_TOKEN);

        // the lexer may emit a string chunk after ] if there's content
        if (current_type(parser) == STRING_LITERAL_TOKEN){
            ASTNode *chunk = parse_expression(parser); // resolves to a LITERAL_NODE
            LinkedASTNode *linked_chunk = init_linked_ast(chunk);
            values_tail->next = linked_chunk;
            values_tail = linked_chunk;
        }
    }

    new_node->data.say.values = values_head;
    expect_must_statement_end(parser);
    return new_node;
}

// parse a return statement
// <return>      ::= "return" <expr>? "\n"
static ASTNode *parse_return(Parser *parser){
    ASTNode *new_node = init_ast_node(RETURN_NODE, current_line(parser));

    expect_must(parser, RETURN_TOKEN);

    //* added EOF_TOKEN to here too
    if (expect_optional(parser, END_OF_LINE_TOKEN) || expect_optional(parser, EOF_TOKEN)){ // no expression for void returns
        return new_node;
    }

    new_node->data.return_statement.value = parse_expression(parser);
    expect_must_statement_end(parser);

    return new_node;
}


/* ----- inner parsing functions (that parse parts of statements) ----- */
// <param>      ::= IDENTIFIER "as" <type> ( "=" <expr> )?
static ASTNode *parse_function_param(Parser *parser){
    ASTNode *new_node = init_ast_node(FUNCTION_PARAM_NODE, current_line(parser));

    new_node->data.function_param.name = parse_identifier(parser);
    expect_must(parser, AS_TOKEN);
    new_node->data.function_param.type_info = parse_type(parser, false); // is_function = false

    // default value (optional)
    if (expect_optional(parser, ASSIGN_TOKEN)){
        new_node->data.function_param.default_val = parse_expression(parser);
    }

    return new_node;
}

// parses a full scope (inside a function, if statement, while, ...)
// <scope>      ::= "start" ":" <statement>* "end"
static LinkedASTNode *parse_full_scope(Parser *parser){
    expect_must(parser, START_SCOPE_TOKEN);
    expect_must(parser, COLON_TOKEN);
    LinkedASTNode *statements_head = parse_body(parser, END_SCOPE_TOKEN, END_SCOPE_TOKEN);
    expect_must(parser, END_SCOPE_TOKEN);

    return statements_head;
} 

// parses consecutive statements (inside the body of a scope)
// Note: some functions may require a second stop_token. ones that dont will pass the same token twice
static LinkedASTNode *parse_body(Parser *parser, TokenType stop_token, TokenType stop_token2){
    LinkedASTNode *statements_head = NULL;
    LinkedASTNode *statements_tail = NULL;

    while(current_type(parser) != stop_token && current_type(parser) != stop_token2){
        // skip useless \n's that were flagged as EOL tokens
        if (!skip_consecutive_newlines(parser, stop_token, stop_token2)){
            break;
        }

        LinkedASTNode *linked_statement = init_linked_ast(parse_statement(parser));

        if (statements_head == NULL){ // for first arg
            statements_head = linked_statement;
        }
        else {
            statements_tail->next = linked_statement;
        }

        statements_tail = linked_statement;
    }

    return statements_head;
}

// parse any arithmetic expression
// goes by the same pattern as used and explained in parse_term, just different operators
// <expr>        ::= <term> ( ( "+" | "-" ) <term> )*
static ASTNode *parse_expression(Parser *parser){
    ASTNode *left_val = parse_term(parser);

    // look for ( ( "+" | "-" ) <term> )*
    while(current_type(parser) == PLUS_TOKEN || current_type(parser) == MINUS_TOKEN){  
        int expr_line = current_line(parser); // get it now because we advance
        TokenType op = current_type(parser);
        advance(parser);
        ASTNode *right_val = parse_term(parser);

        ASTNode *new_node = init_ast_node(ARITHMETIC_EXPR_NODE, expr_line); // put everything in the right struct
        new_node->data.expression.left_val = left_val;
        new_node->data.expression.op = op;
        new_node->data.expression.right_val = right_val;

        left_val = new_node; // becomes the new left for next iteration
    }

    return left_val; // despite the name, keeps the whole thing
}

// parse a full term (basically arithmetic expression but only with *, / or % for PEMDAS)
// we basically always push the full term expression to the ASTNode *left_val and return it
// <term>        ::= <factor> ( ( "*" | "/" | "%" ) <factor> )*
static ASTNode *parse_term(Parser *parser){
    ASTNode *left_val = parse_factor(parser);

    // look for ( ( "*" | "/" | "%" ) <factor> )*
    while(current_type(parser) == MULT_TOKEN || current_type(parser) == DIVIDE_TOKEN ||
          current_type(parser) == MODULO_TOKEN){

        int expr_line = current_line(parser); // get it now because we advance
        TokenType op = current_type(parser);
        advance(parser);
        ASTNode *right_val = parse_factor(parser);

        ASTNode *new_node = init_ast_node(ARITHMETIC_EXPR_NODE, expr_line); // put everything in the right struct
        new_node->data.expression.left_val = left_val;
        new_node->data.expression.op = op;
        new_node->data.expression.right_val = right_val;

        left_val = new_node; // becomes the new left for next iteration
    }

    return left_val; // despite the name, keeps the whole thing
}

// parses a factor with unary before it (goes recursivee) or just a regular atom
// <factor>      ::= "-" <factor> | <atom>
static ASTNode *parse_factor(Parser *parser){
    if (expect_optional(parser, MINUS_TOKEN)){
        ASTNode *new_node = init_ast_node(UNARY_NODE, current_line(parser));

        new_node->data.unary.op = MINUS_TOKEN;
        new_node->data.unary.operand = parse_factor(parser); // wow...recursion

        return new_node;
    }

    return parse_atom(parser);
}

// <atom>        ::= <call> | IDENTIFIER "[" <expr> "]" | IDENTIFIER | <literal> | "(" <expr> ")"
// <call>        ::= IDENTIFIER "(" <args> ")"
static ASTNode *parse_atom(Parser *parser){
    switch(current_type(parser)){
        // <call> | IDENTIFIER "[" <expr> "]" | IDENTIFIER
        case IDENTIFIER_TOKEN:
            switch(next_type(parser)){
                // <call> 
                case OPEN_PAREN_TOKEN:
                    return parse_function_call(parser);

                // IDENTIFIER "[" <expr> "]" 
                case OPEN_BRACKET_TOKEN:
                    ASTNode *new_index = init_ast_node(INDEX_NODE, current_line(parser));

                    new_index->data.index.list_name = parse_identifier(parser);
                    expect_must(parser, OPEN_BRACKET_TOKEN);
                    new_index->data.index.index_expr = parse_expression(parser);
                    expect_must(parser, CLOSE_BRACKET_TOKEN);
                    
                    return new_index;

                // plain identifier
                default:
                    // Note: this is the only usage for an IDENTIFIER_NODE
                    // Note: all other functions directly call parse_identifier and store it as a (char *)
                    ASTNode *new_identifier = init_ast_node(IDENTIFIER_NODE, current_line(parser));

                    new_identifier->data.identifier.name = parse_identifier(parser);

                    return new_identifier;
            }
 
        // <literal>
        case INTEGER_LITERAL_TOKEN:
        case FLOAT_LITERAL_TOKEN:
        case STRING_LITERAL_TOKEN:
        case CHAR_LITERAL_TOKEN:
        case TRUE_TOKEN:
        case FALSE_TOKEN:
        case OPEN_BRACKET_TOKEN:
            return parse_literal(parser);

        // "(" <expr> ")"
        case OPEN_PAREN_TOKEN:
            expect_must(parser, OPEN_PAREN_TOKEN);

            ASTNode *new_expr = parse_bool(parser);
            expect_must(parser, CLOSE_PAREN_TOKEN);

            return new_expr;

        default:
            print_error("Parser (line %d): Expected an <atom> but got token %d. \n", current_line(parser), current_type(parser));
    }
}

// parse any logical / comparison expression
// <bool>        ::= "not" <bool> | <or>
static ASTNode *parse_bool(Parser *parser){
    if (expect_optional(parser, NOT_TOKEN)){
        ASTNode *new_node = init_ast_node(UNARY_NODE, current_line(parser));

        new_node->data.unary.op = NOT_TOKEN;
        new_node->data.unary.operand = parse_bool(parser); // more recursion!!!

        return new_node;
    }

    return parse_or(parser);
}

// we basically always push the full term expression to the ASTNode *left_val and return it
// also implemented in parse_and, parse_term and parse_expression
// <or>          ::= <and> ( "or" <and> )*
static ASTNode *parse_or(Parser *parser){
    ASTNode *left_val = parse_and(parser);

    // ( "or" <and> )*
    while(current_type(parser) == OR_TOKEN){  
        int expr_line = current_line(parser);      
        TokenType op = current_type(parser);
        advance(parser);
        ASTNode *right_val = parse_and(parser);

        ASTNode *new_node = init_ast_node(LOGICAL_EXPR_NODE, expr_line); // put everything in the right struct
        new_node->data.expression.left_val = left_val;
        new_node->data.expression.op = op;
        new_node->data.expression.right_val = right_val;

        left_val = new_node; // becomes the new left for next iteration
    }

    return left_val; // despite the name, keeps the whole thing
}

// <and>         ::= <cmp> ( "and" <cmp> )*
static ASTNode *parse_and(Parser *parser){
    ASTNode *left_val = parse_comparison(parser);

    // ( "and" <cmpp> )*
    while(current_type(parser) == AND_TOKEN){ 
        int expr_line = current_line(parser);             
        TokenType op = current_type(parser);
        advance(parser);
        ASTNode *right_val = parse_comparison(parser);

        ASTNode *new_node = init_ast_node(LOGICAL_EXPR_NODE, expr_line); // put everything in the right struct
        new_node->data.expression.left_val = left_val;
        new_node->data.expression.op = op;
        new_node->data.expression.right_val = right_val;

        left_val = new_node; // becomes the new left for next iteration
    }

    return left_val; // despite the name, keeps the whole thing
}

// <cmp>         ::= "(" <bool> ")" | <expr> <cmp_op> <expr>
// <cmp_op>      ::= "is" | "more" "than" | "less" "than"
static ASTNode *parse_comparison(Parser *parser){
    int cmp_line = current_line(parser); // get before parsing left_val
    ASTNode *left_val = parse_expression(parser);

    TokenType operator_type = current_type(parser);
    if (operator_type != IS_TOKEN && operator_type != MORE_TOKEN && operator_type != LESS_TOKEN){
        return left_val; // no <cmp_op>
    }

    ASTNode *new_node = init_ast_node(COMPARISON_EXPR_NODE, cmp_line);
    new_node->data.expression.left_val = left_val;

    switch(operator_type){
        case IS_TOKEN:
            advance(parser);
            break;

        case MORE_TOKEN:
        case LESS_TOKEN:
            advance(parser);
            expect_must(parser, THAN_TOKEN);
            break;

        default: break;
    }

    // for More than / less than only store the first word (more/less)
    new_node->data.expression.op = operator_type;
    new_node->data.expression.right_val = parse_expression(parser);

    return new_node;
}

// parse any literal, redirect to parse_list_literal for list literals
// <literal>     ::= INT_LITERAL | FLOAT_LITERAL | CHAR_LITERAL | STRING_LITERAL | "true" | "false" | <list_literal>
static ASTNode *parse_literal(Parser *parser){
    if (current_type(parser) == OPEN_BRACKET_TOKEN){
        return parse_list_literal(parser);
    }

    ASTNode *new_node = init_ast_node(LITERAL_NODE, current_line(parser));

    switch(current_type(parser)){
        case INTEGER_LITERAL_TOKEN:
            new_node->data.literal.type = TYPE_INT;
            break;

        case FLOAT_LITERAL_TOKEN:
            new_node->data.literal.type = TYPE_FLOAT;
            break;

        case CHAR_LITERAL_TOKEN:
            new_node->data.literal.type = TYPE_CHAR;
            break;

        case STRING_LITERAL_TOKEN:
            new_node->data.literal.type = TYPE_STRING;
            break;

        case TRUE_TOKEN:
        case FALSE_TOKEN:
            new_node->data.literal.type = TYPE_BOOL;
            break;

        default:
            print_error("Parser (line %d): Expected a literal but got token %d. \n", current_line(parser), current_type(parser));

    }

    new_node->data.literal.value = strdup(current_string(parser));
    check_nullptr(new_node->data.literal.value, "Parser: Strdup for a literal's string failed. \n");

    advance(parser); // still need to move past this token

    return new_node;
}

// parse a list literal, add a linked list of all the values inside it in order
// <list_literal>::= "[" ( <expr> ( "," <expr> )* )? "]"
static ASTNode *parse_list_literal(Parser *parser){
    ASTNode *new_node = init_ast_node(LIST_LITERAL_NODE, current_line(parser));

    expect_must(parser, OPEN_BRACKET_TOKEN);


    // ( <expr> ( "," <expr> )* )?
    LinkedASTNode *expressions_tail = new_node->data.list_literal.values; // tail is head as of start

    while(current_type(parser) != CLOSE_BRACKET_TOKEN){
        LinkedASTNode *linked_expr = init_linked_ast(parse_expression(parser));

        if (new_node->data.list_literal.values == NULL){ // for first expression
            new_node->data.list_literal.values = linked_expr;
        }
        else {
            expressions_tail->next = linked_expr;
        }

        expressions_tail = linked_expr;

        // after value can only come , or ]. the while condition already handles the ] case
        if (!expect_optional(parser, COMMA_TOKEN)){
            break;
        }

        // getting here means there was a comma right before, so there shouldnt be a bracket
        if (current_type(parser) == CLOSE_BRACKET_TOKEN){
            print_error("Parser (line %d): Expected another argument but got a closing bracket. \n", current_line(parser));
        }
    }

    expect_must(parser, CLOSE_BRACKET_TOKEN);

    return new_node;
}

// parse type declarations (int, string, int list, ...) as a TypeInfo *
// also handles "void"
// <type>   ::= <scalar> <list_suffix>*
// <scalar>      ::= "int" | "float" | "char" | "string" | "bool"
// <list_suffix> ::= "list"
static TypeInfo *parse_type(Parser *parser, bool is_function){
    // void type can only be for functions
    if (!is_function && current_type(parser) == VOID_TOKEN){
        print_error("Parser (line %d): Variable cannot be a void type. \n", current_line(parser));
    }

    TypeInfo *inner_node = NULL; // init it here so we can use it in the while loop

    switch(current_type(parser)){
        case INT_TOKEN:
            inner_node = init_type_info(TYPE_INT);
            break;

        case FLOAT_TOKEN:
            inner_node = init_type_info(TYPE_FLOAT);
            break;

        case CHAR_TOKEN:
            inner_node = init_type_info(TYPE_CHAR);
            break;

        case STRING_TOKEN:
            inner_node = init_type_info(TYPE_STRING);
            break;

        case BOOL_TOKEN:
            inner_node = init_type_info(TYPE_BOOL);
            break;

        // Note: lists cannot come after TYPE_VOID (a "void list ...")
        case VOID_TOKEN:
            advance(parser); // advance before return
            return init_type_info(TYPE_VOID);

        default:
            print_error("Parser (line %d): Expected a type but got token %d. \n", current_line(parser), current_type(parser));
    }

    advance(parser);

    // <list_suffix>*
    while(current_type(parser) == LIST_TOKEN){
        TypeInfo *outer_node = init_type_info(TYPE_LIST);
        outer_node->inner = inner_node;

        inner_node = outer_node;

        advance(parser);
    }

    return inner_node;
}

// parses an identifier token and returns it's string (name)
static char *parse_identifier(Parser *parser){
    char *name = strdup(current_string(parser)); // expect_must advances and we need to keep token.string

    check_nullptr(name, 
        "Parser (line %d): strdup for token %d string failed. \n", current_line(parser), current_type(parser)
    );

    expect_must(parser, IDENTIFIER_TOKEN); 

    return name;
}