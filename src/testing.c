#include <stdio.h>
#include <string.h>
#include "../include/testing.h"


// private function declarations
/* ----- Lexer ----- */
static const char *token_type_to_string(TokenType type);

/* ----- Parser ----- */
static const char *node_type_to_string(NodeType type);
static void print_ast_node(ASTNode *ast_node, const char *prefix, bool is_last);
static void print_linked_list(LinkedASTNode *linked_ast, const char *prefix);
static const char *type_to_string(Type type);
static void print_type_info(TypeInfo *type_info);


// global variables
#define AST_PREFIX_LENGTH 256


/* ----- Lexer ----- */
static const char *token_type_to_string(TokenType type){
    switch (type){
        case CREATE_TOKEN:          return "CREATE_TOKEN";
        case AS_TOKEN:              return "AS_TOKEN";
        case START_SCOPE_TOKEN:     return "START_SCOPE_TOKEN";
        case END_SCOPE_TOKEN:       return "END_SCOPE_TOKEN";
        case IF_TOKEN:              return "IF_TOKEN";
        case ELSE_TOKEN:            return "ELSE_TOKEN";
        case WHILE_TOKEN:           return "WHILE_TOKEN";
        case REPEAT_TOKEN:          return "REPEAT_TOKEN";
        case TO_TOKEN:              return "TO_TOKEN";
        case STEP_TOKEN:            return "STEP_TOKEN";
        case ON_TOKEN:              return "ON_TOKEN";
        case IN_TOKEN:              return "IN_TOKEN";
        case STOP_TOKEN:            return "STOP_TOKEN";
        case SKIP_TOKEN:            return "SKIP_TOKEN";
        case SAY_TOKEN:             return "SAY_TOKEN";
        case WITH_TOKEN:            return "WITH_TOKEN";
        case RETURN_TOKEN:          return "RETURN_TOKEN";
        case TRUE_TOKEN:            return "TRUE_TOKEN";
        case FALSE_TOKEN:           return "FALSE_TOKEN";
        case INT_TOKEN:             return "INT_TOKEN";
        case FLOAT_TOKEN:           return "FLOAT_TOKEN";
        case CHAR_TOKEN:            return "CHAR_TOKEN";
        case LIST_TOKEN:            return "LIST_TOKEN";
        case STRING_TOKEN:          return "STRING_TOKEN";
        case BOOL_TOKEN:            return "BOOL_TOKEN";
        case VOID_TOKEN:            return "VOID_TOKEN";
        case IS_TOKEN:              return "IS_TOKEN";
        case NOT_TOKEN:             return "NOT_TOKEN";
        case MORE_TOKEN:            return "MORE_TOKEN";
        case LESS_TOKEN:            return "LESS_TOKEN";
        case THAN_TOKEN:            return "THAN_TOKEN";
        case AND_TOKEN:             return "AND_TOKEN";
        case OR_TOKEN:              return "OR_TOKEN";
        case PLUS_TOKEN:            return "PLUS_TOKEN";
        case MINUS_TOKEN:           return "MINUS_TOKEN";
        case MULT_TOKEN:            return "MULT_TOKEN";
        case DIVIDE_TOKEN:          return "DIVIDE_TOKEN";
        case MODULO_TOKEN:          return "MODULO_TOKEN";
        case ADD_TO_TOKEN:          return "ADD_TO_TOKEN";
        case SUB_TO_TOKEN:          return "SUB_TO_TOKEN";
        case MULT_TO_TOKEN:         return "MULT_TO_TOKEN";
        case DIVIDE_TO_TOKEN:       return "DIVIDE_TO_TOKEN";
        case MOD_TO_TOKEN:          return "MOD_TO_TOKEN";
        case ASSIGN_TOKEN:          return "ASSIGN_TOKEN";
        case OPEN_PAREN_TOKEN:      return "OPEN_PAREN_TOKEN";
        case CLOSE_PAREN_TOKEN:     return "CLOSE_PAREN_TOKEN";
        case OPEN_BRACKET_TOKEN:    return "OPEN_BRACKET_TOKEN";
        case CLOSE_BRACKET_TOKEN:   return "CLOSE_BRACKET_TOKEN";
        case END_OF_LINE_TOKEN:     return "END_OF_LINE_TOKEN";
        case COLON_TOKEN:           return "COLON_TOKEN";
        case COMMA_TOKEN:           return "COMMA_TOKEN";
        case INTEGER_LITERAL_TOKEN: return "INTEGER_LITERAL_TOKEN";
        case FLOAT_LITERAL_TOKEN:   return "FLOAT_LITERAL_TOKEN";
        case CHAR_LITERAL_TOKEN:    return "CHAR_LITERAL_TOKEN";
        case STRING_LITERAL_TOKEN:  return "STRING_LITERAL_TOKEN";
        case IDENTIFIER_TOKEN:      return "IDENTIFIER_TOKEN";
        case EOF_TOKEN:             return "EOF_TOKEN";
        case UNKNOWN_TOKEN:         return "UNKNOWN_TOKEN";
        case NOT_FOUND_TOKEN:       return "NOT_FOUND_TOKEN";
        default:                    return "<INVALID_TOKEN_TYPE>";
    }
}

// print a tokens list in order
void print_tokens_list(TokenNode *tokens_head){
    TokenNode *token_node = tokens_head;

    while (token_node){
        const char *token_string = token_node->token.string;

        if (token_node->token.type == END_OF_LINE_TOKEN || strcmp(token_node->token.string, "\n") == 0){
            token_string = "\\n";
        }
        else if (token_node->token.type == EOF_TOKEN){
            token_string = "EOF";
        }
        else if (strcmp(token_node->token.string, "\t") == 0){
            token_string = "\\t";
        }

        printf(
            "%s -> %s, LEN: %d, LINE: %d \n",
            token_string,
            token_type_to_string(token_node->token.type),
            token_node->token.length,
            token_node->token.line
        );

        token_node = token_node->next;
    }
}


/* ----- Parser ----- */
static const char *node_type_to_string(NodeType type){
    switch (type){
        case PROGRAM_NODE:         return "PROGRAM_NODE";
        case FUNCTION_NODE:        return "FUNCTION_NODE";
        case FUNCTION_PARAM_NODE:  return "FUNCTION_PARAM_NODE";
        case FUNCTION_CALL_NODE:   return "FUNCTION_CALL_NODE";
        case CREATE_VAR_NODE:      return "CREATE_VAR_NODE";
        case ASSIGNMENT_NODE:      return "ASSIGNMENT_NODE";
        case IF_NODE:              return "IF_NODE";
        case ELSE_NODE:            return "ELSE_NODE";
        case WHILE_NODE:           return "WHILE_NODE";
        case REPEAT_NODE:          return "REPEAT_NODE";
        case REPEAT_ON_NODE:       return "REPEAT_ON_NODE";
        case SAY_NODE:             return "SAY_NODE";
        case INPUT_NODE:           return "INPUT_NODE";
        case RETURN_NODE:          return "RETURN_NODE";
        case STOP_NODE:            return "STOP_NODE";
        case SKIP_NODE:            return "SKIP_NODE";
        case ARITHMETIC_EXPR_NODE: return "ARITHMETIC_EXPR_NODE";
        case COMPARISON_EXPR_NODE: return "COMPARISON_EXPR_NODE";
        case LOGICAL_EXPR_NODE:    return "LOGICAL_EXPR_NODE";
        case UNARY_NODE:           return "UNARY_NODE";
        case INDEX_NODE:           return "INDEX_NODE";
        case IDENTIFIER_NODE:      return "IDENTIFIER_NODE";
        case LITERAL_NODE:         return "LITERAL_NODE";
        case LIST_LITERAL_NODE:    return "LIST_LITERAL_NODE";
        default:                   return "<INVALID_NODE_TYPE>";
    }
}

static const char *type_to_string(Type type){
    switch (type){
        case TYPE_INT:    return "TYPE_INT";
        case TYPE_FLOAT:  return "TYPE_FLOAT";
        case TYPE_CHAR:   return "TYPE_CHAR";
        case TYPE_STRING: return "TYPE_STRING";
        case TYPE_LIST:   return "TYPE_LIST";
        case TYPE_BOOL:   return "TYPE_BOOL";
        case TYPE_VOID:   return "TYPE_VOID";
        default:          return "<INVALID_TYPE>";
    }
}

// recursively, print a type info node and its contents
static void print_type_info(TypeInfo *type_info) {
    if (!type_info){ printf("<NULL_TYPE>"); return; }
    if (type_info->type == TYPE_LIST) {
        printf("TYPE_LIST(");
        print_type_info(type_info->inner);
        printf(")");
    } else {
        printf("%s", type_to_string(type_info->type));
    }
}

// build a child prefix for a label that uses a connector
// "├─" → append "│  ", "└─" → append "   "
static void make_label_prefix(const char *parent, bool is_mid, char *out){
    snprintf(out, AST_PREFIX_LENGTH, "%s%s", parent, is_mid ? "│  " : "   ");
}

// print an ast node and is contents
// takes care of indentation and has a switch for each node type
static void print_ast_node(ASTNode *node, const char *prefix, bool is_last) {
    if (!node){
        return;
    }

    // decide which connector and extensions to use
    const char *connector = is_last ? "└─ " : "├─ ";
    const char *extension = is_last ? "   " : "│  ";

    // print indentation
    printf("%s%s%s (line %d)\n", prefix, connector, node_type_to_string(node->node_type), node->line);

    char child_prefix[AST_PREFIX_LENGTH];
    snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, extension); // copy the prefix onto child_prefix

    // label_mid: prefix for children of a ├─ label
    // label_end: prefix for children of a └─ label
    char label_mid[AST_PREFIX_LENGTH];
    char label_end[AST_PREFIX_LENGTH];
    make_label_prefix(child_prefix, true,  label_mid);
    make_label_prefix(child_prefix, false, label_end);

    // print contents of current node
    switch (node->node_type) {
        case PROGRAM_NODE:
            print_linked_list(node->data.program.statements, child_prefix);
            break;

        case FUNCTION_NODE:
            printf("%s├─ name: '%s'\n", child_prefix, node->data.function.name);
            printf("%s├─ return: ", child_prefix);
            print_type_info(node->data.function.return_type_info);
            printf("\n");
            if (node->data.function.params) {
                printf("%s├─ params:\n", child_prefix);
                print_linked_list(node->data.function.params, label_mid);
            }
            printf("%s└─ body:\n", child_prefix);
            print_linked_list(node->data.function.body, label_end);
            break;

        case FUNCTION_PARAM_NODE:
            printf("%s├─ name: '%s'\n", child_prefix, node->data.function_param.name);
            if (node->data.function_param.default_val) {
                printf("%s├─ type: ", child_prefix);
                print_type_info(node->data.function_param.type_info);
                printf("\n");
                printf("%s└─ default:\n", child_prefix);
                print_ast_node(node->data.function_param.default_val, label_end, true);
            } else {
                printf("%s└─ type: ", child_prefix);
                print_type_info(node->data.function_param.type_info);
                printf("\n");
            }
            break;

        case FUNCTION_CALL_NODE:
            printf("%s├─ name: '%s'\n", child_prefix, node->data.function_call.name);
            printf("%s└─ args:\n", child_prefix);
            print_linked_list(node->data.function_call.params, label_end);
            break;

        case CREATE_VAR_NODE:
            printf("%s├─ name: '%s'\n", child_prefix, node->data.create_var.name);
            if (node->data.create_var.value) {
                printf("%s├─ type: ", child_prefix);
                print_type_info(node->data.create_var.type_info);
                printf("\n");
                printf("%s└─ value:\n", child_prefix);
                print_ast_node(node->data.create_var.value, label_end, true);
            } else {
                printf("%s└─ type: ", child_prefix);
                print_type_info(node->data.create_var.type_info);
                printf("\n");
            }
            break;

        case ASSIGNMENT_NODE:
            printf("%s├─ name: '%s'\n", child_prefix, node->data.assignment.name);
            if (node->data.assignment.index_expr) {
                printf("%s├─ op: %s\n", child_prefix, token_type_to_string(node->data.assignment.assign_op));
                printf("%s├─ index:\n", child_prefix);
                print_ast_node(node->data.assignment.index_expr, label_mid, true);
            } else {
                printf("%s├─ op: %s\n", child_prefix, token_type_to_string(node->data.assignment.assign_op));
            }
            printf("%s└─ value:\n", child_prefix);
            print_ast_node(node->data.assignment.value, label_end, true);
            break;

        case IF_NODE:
            printf("%s├─ condition:\n", child_prefix);
            print_ast_node(node->data.if_statement.condition, label_mid, true);
            if (node->data.if_statement.else_branch) {
                printf("%s├─ body:\n", child_prefix);
                print_linked_list(node->data.if_statement.body, label_mid);
                printf("%s└─ else:\n", child_prefix);
                print_linked_list(node->data.if_statement.else_branch, label_end);
            } else {
                printf("%s└─ body:\n", child_prefix);
                print_linked_list(node->data.if_statement.body, label_end);
            }
            break;

        case ELSE_NODE:
            if (node->data.else_statement.condition) {
                printf("%s├─ condition:\n", child_prefix);
                print_ast_node(node->data.else_statement.condition, label_mid, true);
                printf("%s└─ body:\n", child_prefix);
            } else {
                printf("%s└─ body:\n", child_prefix);
            }
            print_linked_list(node->data.else_statement.body, label_end);
            break;

        case WHILE_NODE:
            printf("%s├─ condition:\n", child_prefix);
            print_ast_node(node->data.while_loop.condition, label_mid, true);
            printf("%s└─ body:\n", child_prefix);
            print_linked_list(node->data.while_loop.body, label_end);
            break;

        case REPEAT_NODE:
            printf("%s├─ var: '%s'\n", child_prefix, node->data.repeat.var_name);
            printf("%s├─ from:\n", child_prefix);
            print_ast_node(node->data.repeat.from, label_mid, true);
            printf("%s├─ to:\n", child_prefix);
            if (node->data.repeat.step) {
                print_ast_node(node->data.repeat.to, label_mid, true);
                printf("%s├─ step:\n", child_prefix);
                print_ast_node(node->data.repeat.step, label_mid, true);
            } else {
                print_ast_node(node->data.repeat.to, label_mid, true);
            }
            printf("%s└─ body:\n", child_prefix);
            print_linked_list(node->data.repeat.body, label_end);
            break;

        case REPEAT_ON_NODE:
            printf("%s├─ var: '%s'\n", child_prefix, node->data.repeat_on.var_name);
            printf("%s├─ list: '%s'\n", child_prefix, node->data.repeat_on.list_name);
            printf("%s└─ body:\n", child_prefix);
            print_linked_list(node->data.repeat_on.body, label_end);
            break;

        case SAY_NODE:
            printf("%s└─ value:\n", child_prefix);
            print_linked_list(node->data.say.values, label_end);
            break;
        
        case INPUT_NODE:
            printf("%s└─ type: %s\n", child_prefix, type_to_string(node->data.input.type));
            break;

        case RETURN_NODE:
            if (node->data.return_statement.value) {
                printf("%s└─ value:\n", child_prefix);
                print_ast_node(node->data.return_statement.value, label_end, true);
            }
            break;

        case STOP_NODE:
        case SKIP_NODE:
            break;

        case ARITHMETIC_EXPR_NODE:
        case COMPARISON_EXPR_NODE:
        case LOGICAL_EXPR_NODE:
            printf("%s├─ op: %s\n", child_prefix, token_type_to_string(node->data.expression.op));
            printf("%s├─ left:\n", child_prefix);
            print_ast_node(node->data.expression.left_val, label_mid, true);
            printf("%s└─ right:\n", child_prefix);
            print_ast_node(node->data.expression.right_val, label_end, true);
            break;

        case UNARY_NODE:
            printf("%s├─ op: %s\n", child_prefix, token_type_to_string(node->data.unary.op));
            printf("%s└─ operand:\n", child_prefix);
            print_ast_node(node->data.unary.operand, label_end, true);
            break;

        case INDEX_NODE:
            printf("%s├─ list: '%s'\n", child_prefix, node->data.index.list_name);
            printf("%s└─ index:\n", child_prefix);
            print_ast_node(node->data.index.index_expr, label_end, true);
            break;

        case IDENTIFIER_NODE:
            printf("%s└─ name: '%s'\n", child_prefix, node->data.identifier.name);
            break;

        case LITERAL_NODE:
            printf("%s└─ %s '%s'\n", child_prefix, type_to_string(node->data.literal.type), node->data.literal.value);
            break;

        case LIST_LITERAL_NODE:
            printf("%s└─ values:\n", child_prefix);
            print_linked_list(node->data.list_literal.values, label_end);
            break;

        default:
            printf("%s└─ <unimplemented>\n", child_prefix);
            break;
    }
}

// prints a linked list of ast nodes, in order
static void print_linked_list(LinkedASTNode *list, const char *prefix){
    while (list){
        bool is_last = (list->next == NULL);
        print_ast_node(list->node, prefix, is_last);

        list = list->next;
    }
}

// prints a whole ast tree starting from this function
void print_ast(ASTNode *ast_root) {
    if (!ast_root){ 
        printf("<empty AST>\n");
        return;
    }

    // print program node
    printf("%s (line %d)\n", node_type_to_string(ast_root->node_type), ast_root->line);

    char child_prefix[AST_PREFIX_LENGTH] = "";

    // forward to other printing functions to start the whole process
    switch (ast_root->node_type) {
        case PROGRAM_NODE:
            print_linked_list(ast_root->data.program.statements, child_prefix);
            break;
        default:
            print_ast_node(ast_root, child_prefix, true); // is_last = true
            break;
    }
}