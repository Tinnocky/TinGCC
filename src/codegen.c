#include <stdlib.h>
#include <string.h>
#include "../include/analysis.h"
#include "../include/codegen.h"
#include "../include/utils.h"


// private function declarations
/* ----- Function "methods" -----*/
static Function *init_function(ASTNode *func_node);
static LinkedFunction *init_linked_function(Function *function);
static LinkedASTNode *get_function_params(Codegen *codegen, char *name, int line);
static void free_linked_function(LinkedFunction *linked_func);

/* ----- Main functions ----- */
static LinkedFunction *gen_function_declarations(Codegen *codegen, ASTNode *ast_root);
static void gen_statement(Codegen *codegen, ASTNode *statement);
static void gen_expression(Codegen *codegen, ASTNode *expression);

/* ----- Statement generation functions ----- */
static void gen_function(Codegen *codegen, ASTNode *node);
static void gen_create_var(Codegen *codegen, ASTNode *node);
static void gen_create_var_list(Codegen *codegen, ASTNode *node);
static void gen_assignment(Codegen *codegen, ASTNode *node);
static void gen_assignment_index_expr(Codegen *codegen, ASTNode *node);
static void gen_assignment_list(Codegen *codegen, ASTNode *node);
static void gen_if(Codegen *codegen, ASTNode *node);
static void gen_else(Codegen *codegen, ASTNode *node);
static void gen_while(Codegen *codegen, ASTNode *node);
static void gen_repeat(Codegen *codegen, ASTNode *node);
static void gen_repeat_on(Codegen *codegen, ASTNode *node);
static void gen_say(Codegen *codegen, ASTNode *node);
static void gen_return(Codegen *codegen, ASTNode *node);
static void gen_stop(Codegen *codegen, ASTNode *node);
static void gen_skip(Codegen *codegen, ASTNode *node);

/* ----- Expression generation functions ----- */
static void gen_function_call(Codegen *codegen, ASTNode *node);
static void gen_identifier(Codegen *codegen, ASTNode *node);
static void gen_input(Codegen *codegen, ASTNode *node);
static void gen_binary_expr(Codegen *codegen, ASTNode *node);
static void gen_unary(Codegen *codegen, ASTNode *node);
static void gen_index(Codegen *codegen, ASTNode *node);
static void gen_literal(Codegen *codegen, ASTNode *node);
static void gen_list_literal(Codegen *codegen, ASTNode *node);

/* ----- Builtin generation functions ----- */
static bool gen_built_in_call(Codegen *codegen, ASTNode *node);
static void gen_length_prefix(Codegen *codegen, ASTNode *node);
static void gen_to_int_prefix(Codegen *codegen, ASTNode *node);
static void gen_to_float_prefix(Codegen *codegen, ASTNode *node);
static void gen_to_char_prefix(Codegen *codegen, ASTNode *node);
static void gen_to_string_prefix(Codegen *codegen, ASTNode *node);
static void gen_add_call(Codegen *codegen, ASTNode *node);
static void gen_remove_call(Codegen *codegen, ASTNode *node);

/* ----- Inner generation functions ----- */
static void gen_body(Codegen *codegen, LinkedASTNode *statements);
static void gen_function_param(Codegen *codegen, ASTNode *node);
static void gen_type(Codegen *codegen, Type type);
static const char *op_to_c_string(TokenType operator);
static TokenType compound_to_binary_op(TokenType op);
static const char *type_to_specifier(Type type);
static void gen_box_item(Codegen *codegen, Type type, ASTNode *value);
static const char *box_prefix(Type type);
static const char *box_suffix(Type type);
static const char *unbox_prefix(Type type);
static const char *unbox_suffix(Type type);
static ASTNode *get_arg(LinkedASTNode *args, unsigned int index);


/* ----- Function "methods" -----*/
static Function *init_function(ASTNode *func_node){
    Function *new_function = malloc(sizeof(Function));

    new_function->name = strdup(func_node->data.function.name);
    check_nullptr(new_function->name, "Codegen: Strdup for a function's name failed. \n");

    new_function->params = func_node->data.function.params;

    return new_function;
}

static LinkedFunction *init_linked_function(Function *function){
    LinkedFunction *new_linked_func = malloc(sizeof(LinkedFunction));

    new_linked_func->func = function;
    new_linked_func->next = NULL;

    return new_linked_func;
}

static LinkedASTNode *get_function_params(Codegen *codegen, char *name, int line){
    LinkedFunction *functions = codegen->functions;

    while (functions != NULL){
        if (strcmp(functions->func->name, name) == 0){
            return functions->func->params;
        }

        functions = functions->next;
    }

    print_error("Codegen (line %d): Tried to get function params, but function was not found. \n", line);
    return NULL;
}

// Note: doesnt freee the linked_func->func->params because its borrowed and freed at the AST free
static void free_linked_function(LinkedFunction *linked_func){
    while (linked_func != NULL){
        LinkedFunction *next = linked_func->next;

        free(linked_func->func->name);
        free(linked_func->func);
        free(linked_func);

        linked_func = next;
    }
}


/* ----- Codegen "methods" ----- */
// initialize a new codegen struct with all data
// Note: the output file's name doesnt matter because its getting compiled straight away and deleted afterwards
Codegen *init_codegen(void){
    Codegen *new_codegen = malloc(sizeof(Codegen));

    new_codegen->file = fopen(OUTPUT_FILENAME, "w");
    check_nullptr(new_codegen->file, "Codegen: Could not open the output file. \n");

    new_codegen->functions = NULL;
    new_codegen->temp_var_count = 0;

    return new_codegen;
}

void free_codegen(Codegen *codegen){
    free_linked_function(codegen->functions);
    fclose(codegen->file);
    free(codegen);
}


/* ----- Main functions ----- */
// the main function, going through all top level statements and writing them one by one
// works basically the same way parser and analysis work
// also adds libraries ;)
void run_codegen(Codegen *codegen, ASTNode *ast_root){
    fprintf(codegen->file, "#include <stdio.h> \n");
    fprintf(codegen->file, "#include <string.h> \n");
    fprintf(codegen->file, "#include %s \n", RUNTIME_FILENAME);

    LinkedASTNode *statements = ast_root->data.program.statements;

    codegen->functions = gen_function_declarations(codegen, ast_root); // add function declarations + save function data

    while (statements != NULL){
        gen_statement(codegen, statements->node);

        statements = statements->next;
    }
}

// generates declarations for all functions and saves data about them in a linked function
static LinkedFunction *gen_function_declarations(Codegen *codegen, ASTNode *ast_root){
    LinkedASTNode *statements = ast_root->data.program.statements;

    LinkedFunction *head = NULL;
    LinkedFunction *tail = NULL;

    while (statements != NULL){
        if (statements->node->node_type != FUNCTION_NODE){
            statements = statements->next;
            continue;
        }

        ASTNode *function = statements->node;

        // save the functions data for default values
        LinkedFunction *new_node = init_linked_function(init_function(function));

        if (head == NULL){
            head = new_node;
        }
        else {
            tail->next = new_node;
        }
        tail = new_node;

        // write the declarations
        gen_type(codegen, function->data.function.return_type_info->type);
        fprintf(codegen->file, " %s(", function->data.function.name);

        LinkedASTNode *params = function->data.function.params;
        while (params != NULL){
            gen_function_param(codegen, params->node);
            if (params->next != NULL){
                fprintf(codegen->file, ", ");
            }
            params = params->next;
        }

        fprintf(codegen->file, "); \n");

        statements = statements->next;
    }

    return head;
}

// based on the node type, redirects to the specific gen functions
static void gen_statement(Codegen *codegen, ASTNode *statement){
    switch(statement->node_type){
        case FUNCTION_NODE: 
            return gen_function(codegen, statement);

        case FUNCTION_CALL_NODE:
            gen_function_call(codegen, statement);
            fprintf(codegen->file, "; \n");
            return;

        case CREATE_VAR_NODE:
            return gen_create_var(codegen, statement);

        case ASSIGNMENT_NODE:
            return gen_assignment(codegen, statement);

        case IF_NODE: // as with all other times, this too also handles else nodes
            return gen_if(codegen, statement);

        case WHILE_NODE:
            return gen_while(codegen, statement);

        case REPEAT_NODE:
            return gen_repeat(codegen, statement);

        case REPEAT_ON_NODE:
            return gen_repeat_on(codegen, statement);

        case SAY_NODE:
            return gen_say(codegen, statement);

        case RETURN_NODE:
            return gen_return(codegen, statement);

        case STOP_NODE:
            return gen_stop(codegen, statement);

        case SKIP_NODE:
            return gen_skip(codegen, statement);

        default:
            print_error("Codegen (line %d): Expected a full statement AST node but got %d", statement->line, statement->node_type);
    }
}

// based on the node type, redirects to the specific gen functions
static void gen_expression(Codegen *codegen, ASTNode *expression){
    switch(expression->node_type){
        case FUNCTION_CALL_NODE:
            return gen_function_call(codegen, expression);

        case ARITHMETIC_EXPR_NODE:
        case COMPARISON_EXPR_NODE:
        case LOGICAL_EXPR_NODE:
            return gen_binary_expr(codegen, expression);

        case IDENTIFIER_NODE:
            return gen_identifier(codegen, expression);

        case INPUT_NODE:
            return gen_input(codegen, expression);

        case UNARY_NODE:
            return gen_unary(codegen, expression);

        case INDEX_NODE:
            return gen_index(codegen, expression);

        case LITERAL_NODE:
            return gen_literal(codegen, expression);

        case LIST_LITERAL_NODE:
            return gen_list_literal(codegen, expression);

        default:
            print_error("Codegen (line %d): Expected an expression AST node but got %d", expression->line, expression->node_type);
    }
}


/* ----- Statement generation functions ----- */
static void gen_function(Codegen *codegen, ASTNode *node){
    gen_type(codegen, node->data.function.return_type_info->type);
    fprintf(codegen->file, " %s(", node->data.function.name);

    // parameters
    LinkedASTNode *params = node->data.function.params;

    while (params != NULL){
        gen_function_param(codegen, params->node);
        if (params->next != NULL){
            fprintf(codegen->file, ", ");
        }

        params = params->next;
    }

    fprintf(codegen->file, "){ \n");
    gen_body(codegen, node->data.function.body);
    fprintf(codegen->file, "} \n");
}

// <var_decl>    ::= "create" IDENTIFIER "as" <type> ( "=" <expr> )? "\n"
static void gen_create_var(Codegen *codegen, ASTNode *node){
    if (node->data.create_var.type_info->type == TYPE_LIST){ // lists require a different initialization
        return gen_create_var_list(codegen, node);
    }

    gen_type(codegen, node->data.create_var.type_info->type);
    fprintf(codegen->file, " %s", node->data.create_var.name);

    // theres a value to assign too
    if (node->data.create_var.value != NULL){
        fprintf(codegen->file, " = ");
        gen_expression(codegen, node->data.create_var.value);
    }

    fprintf(codegen->file, "; \n");
}

// writes a list with possible assignment
static void gen_create_var_list(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "List *%s = ", node->data.create_var.name);
       
    ASTNode *value = node->data.create_var.value;

    if (value == NULL){
        fprintf(codegen->file, "init_list(); \n");
        return;
    }

    gen_expression(codegen, node->data.create_var.value);
    fprintf(codegen->file, "; \n");
}

// <assign>      ::= <lvalue> <assign_op> <expr> "\n"
static void gen_assignment(Codegen *codegen, ASTNode *node){
    // assigning into a list element
    if (node->data.assignment.target->node_type == INDEX_NODE){
        return gen_assignment_index_expr(codegen, node);
    }

    // whole-list reassignment
    if (node->data.assignment.value->type_info->type == TYPE_LIST){
        return gen_assignment_list(codegen, node);
    }

    // scalar
    gen_expression(codegen, node->data.assignment.target);
    fprintf(codegen->file, " %s ", op_to_c_string(node->data.assignment.assign_op));
    gen_expression(codegen, node->data.assignment.value);
    fprintf(codegen->file, "; \n");
}

// assigning stuff to an index in a list is complicated
static void gen_assignment_index_expr(Codegen *codegen, ASTNode *node){
    ASTNode *target = node->data.assignment.target; // whole expression (like xs[i])
    ASTNode *list = target->data.index.target; // the xs part
    ASTNode *index = target->data.index.index_expr; // the i part

    Type elem_type = target->type_info->type; // what the list holds

    // plain assignment: list_set(<list>, <index>, box_x(value))
    if (node->data.assignment.assign_op == ASSIGN_TOKEN){
        fprintf(codegen->file, "list_set(");
        gen_expression(codegen, list);
        fprintf(codegen->file, ", ");
        gen_expression(codegen, index);
        fprintf(codegen->file, ", ");
        gen_box_item(codegen, elem_type, node->data.assignment.value);
        fprintf(codegen->file, "); \n");

        return;
    }

    // compound assignment (+=, -=, etc) needs a read-modify-write.
    // the list and index go into temps so theyre only evaluated once
    // becomes: { List *_lst = <list>; int _idx = <index>;
    //            list_set(_lst, _idx, box_x(unbox_x(list_get(_lst, _idx)) <op> <value>)); }
    if (elem_type == TYPE_STRING || elem_type == TYPE_LIST){
        print_error("Codegen (line %d): Compound assignment cannot be used on this element type. \n", node->line);
    }

    int id_num = codegen->temp_var_count++;

    fprintf(codegen->file, "{ \n"); // open temp block

    fprintf(codegen->file, "List *_lst%d = ", id_num);
    gen_expression(codegen, list);
    fprintf(codegen->file, "; \n");

    fprintf(codegen->file, "int _idx%d = ", id_num);
    gen_expression(codegen, index);
    fprintf(codegen->file, "; \n");

    fprintf(codegen->file, "list_set(_lst%d, _idx%d, ", id_num, id_num);

    // box the whole (old_value <op> new_value) expression
    fprintf(codegen->file, "%s", box_prefix(elem_type));

    // the old value, unboxed
    fprintf(codegen->file, "%slist_get(_lst%d, _idx%d)%s",
        unbox_prefix(elem_type), id_num, id_num, unbox_suffix(elem_type));

    // the operator, without its "=" part (+= becomes +)
    fprintf(codegen->file, " %s ", op_to_c_string(compound_to_binary_op(node->data.assignment.assign_op)));

    // the new value
    gen_expression(codegen, node->data.assignment.value);

    fprintf(codegen->file, ")); \n"); // close box_x and list_set
    fprintf(codegen->file, "} \n");   // close temp block

    // Note: this wasnt fun
}

// deletes the contents of an existing list and assigns new ones from scratch
static void gen_assignment_list(Codegen *codegen, ASTNode *node){
    if (node->data.assignment.assign_op != ASSIGN_TOKEN){
        print_error("Codegen (line %d): Cannot do this type of assignment on a list. \n", node->line);
    }

    gen_expression(codegen, node->data.assignment.target);
    fprintf(codegen->file, " = ");
    gen_expression(codegen, node->data.assignment.value);
    fprintf(codegen->file, "; \n");
}

// also generates the else branch
// <if>          ::= "if" "(" <bool> ")" "start" ":" <statement>* <else_branch>* "end"
static void gen_if(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "if (");
    gen_expression(codegen, node->data.if_statement.condition);
    fprintf(codegen->file, "){ \n");
    gen_body(codegen, node->data.if_statement.body);
    fprintf(codegen->file, "} \n");

    // else branch
    LinkedASTNode *else_branch = node->data.if_statement.else_branch;

    while (else_branch != NULL){
        gen_else(codegen, else_branch->node);

        else_branch = else_branch->next;
    }
}

// <else_branch> ::= "else" ( "(" <bool> ")" )? "start" ":" <statement>*
static void gen_else(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "else ");

    // for an else-if
    if (node->data.else_statement.condition != NULL){
        fprintf(codegen->file, "if (");
        gen_expression(codegen, node->data.else_statement.condition);
        fprintf(codegen->file, ")");
    }

    fprintf(codegen->file, "{ \n");
    gen_body(codegen, node->data.else_statement.body);
    fprintf(codegen->file, "} \n");
}

// <while>       ::= "while" "(" <bool> ")" "start" ":" <statement>* "end"
static void gen_while(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "while (");
    gen_expression(codegen, node->data.while_loop.condition);
    fprintf(codegen->file, "){ \n");
    gen_body(codegen, node->data.while_loop.body);
    fprintf(codegen->file, "} \n");
}

// <repeat>      ::= "repeat" "(" IDENTIFIER "," <expr> "to" <expr> ("," "step" <expr>)? ")" "start" ":" <statement>* "end"
// Note: we generate the variables earlier in the code, not inside the for loop init
static void gen_repeat(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "{ \n"); // open var initialization block

    int id_num = codegen->temp_var_count++;

    // init i = from
    fprintf(codegen->file, "int %s = ", node->data.repeat.var_name);
    gen_expression(codegen, node->data.repeat.from);
    fprintf(codegen->file, "; \n");

    // init to
    fprintf(codegen->file, "int _to%d = ", id_num);
    gen_expression(codegen, node->data.repeat.to);
    fprintf(codegen->file, ";\n");

    // init step
    fprintf(codegen->file, "int _step%d = ", id_num);
    if (node->data.repeat.step == NULL){ // defaults to +1
        fprintf(codegen->file, "1");
    }
    else {
        gen_expression(codegen, node->data.repeat.step);
    }
    fprintf(codegen->file, ";\n");

    // for loop
    fprintf(codegen->file, "for (;"); // init already done before
    fprintf(codegen->file,
        "(_step%d > 0 && %s < _to%d) || (_step%d < 0 && %s > _to%d)",
        id_num, node->data.repeat.var_name, id_num,
        id_num, node->data.repeat.var_name, id_num);
    fprintf(codegen->file, "; %s += _step%d) {\n", node->data.repeat.var_name, id_num);
    gen_body(codegen, node->data.repeat.body);
    fprintf(codegen->file, "}\n");

    fprintf(codegen->file, "} \n"); // close var initialization block
}

// generates a for loop with the length of the list and assign the item value each time
// <repeat_on>   ::= "repeat" "on" "(" IDENTIFIER "in" <postfix> ")" "start" ":" <statement>* "end"
static void gen_repeat_on(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "{ \n"); // open var initialization block

    int id_num = codegen->temp_var_count++;
    Type elem_type = node->type_info->type;

    // the list expression is evaluated once into a temp so it isnt re-run every iteration
    fprintf(codegen->file, "List *_lst%d = ", id_num);
    gen_expression(codegen, node->data.repeat_on.target);
    fprintf(codegen->file, "; \n");

    // length is snapshotted at the start of the loop
    fprintf(codegen->file, "int _len%d = list_length(_lst%d); \n", id_num, id_num);

    // for loop
    fprintf(codegen->file, "for (int _i%d = 0; ", id_num);
    fprintf(codegen->file, "_i%d < _len%d; ", id_num, id_num);
    fprintf(codegen->file, "_i%d += 1) {\n", id_num); // step always +1

    // loop variable: <elem_type> <name> = unbox_x(list_get(_lst, _i));
    gen_type(codegen, elem_type);
    fprintf(codegen->file, " %s = %slist_get(_lst%d, _i%d)%s; \n",
        node->data.repeat_on.var_name,
        unbox_prefix(elem_type), id_num, id_num, unbox_suffix(elem_type));

    // continue for loop
    gen_body(codegen, node->data.repeat_on.body);
    fprintf(codegen->file, "}\n");

    fprintf(codegen->file, "} \n"); // close var initialization block
}

// Note: actually emits consecutive printf's for each value
static void gen_say(Codegen *codegen, ASTNode *node){
    LinkedASTNode *values = node->data.say.values;

    while (values != NULL){
        const char *specifier = type_to_specifier(values->node->type_info->type);

        fprintf(codegen->file, "printf(\"%s\", ", specifier);
        gen_expression(codegen, values->node);  
        fprintf(codegen->file, ");\n");

        values = values->next;
    }
}

// <return>      ::= "return" <expr>? "\n"
static void gen_return(Codegen *codegen, ASTNode *node){
    // void return
    if (node->data.return_statement.value == NULL){
        fprintf(codegen->file, "return; \n");
        return;
    }

    // has value
    fprintf(codegen->file, "return ");
    gen_expression(codegen, node->data.return_statement.value);
    fprintf(codegen->file, "; \n");
}

static void gen_stop(Codegen *codegen, ASTNode *node){
    (void)node; // unused but still here for symmetry
    fprintf(codegen->file, "break; \n");
}

static void gen_skip(Codegen *codegen, ASTNode *node){
    (void)node; // unused but still here for symmetry
    fprintf(codegen->file, "continue; \n");
}


/* ----- Expression generation functions ----- */
static void gen_function_call(Codegen *codegen, ASTNode *node){
    if (gen_built_in_call(codegen, node)){
        return;
    }

    fprintf(codegen->file, "%s(", node->data.function_call.name);
 
    // arguments
    LinkedASTNode *args = node->data.function_call.args;
    LinkedASTNode *params = get_function_params(codegen, node->data.function_call.name, node->line);

    while (params != NULL){
        if (args == NULL){ // param exists but arg doesnt. add default value
            gen_expression(codegen, params->node->data.function_param.default_val);
        }
        else {
            gen_expression(codegen, args->node);
        }
        
        if (params->next != NULL){
            fprintf(codegen->file, ", ");
        }

        if (args != NULL){
            args = args->next;  
        }
        params = params->next;
    }

    fprintf(codegen->file, ")");
}

static void gen_identifier(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "%s", node->data.identifier.name);
}

static void gen_input(Codegen *codegen, ASTNode *node){
    switch(node->data.input.type){
        case TYPE_INT:
            fprintf(codegen->file, "input_int()");
            break;

        case TYPE_FLOAT:
            fprintf(codegen->file, "input_float()");
            break;

        case TYPE_CHAR:
            fprintf(codegen->file, "input_char()");
            break;

        case TYPE_STRING:
            fprintf(codegen->file, "input_string()");
            break;
        
        default:
            print_error("Codegen (line %d): Input only accepts int, float, char or string. \n", node->line);
    }
}

static void gen_binary_expr(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "(");
    gen_expression(codegen, node->data.expression.left_val);
    fprintf(codegen->file, " %s ", op_to_c_string(node->data.expression.op));
    gen_expression(codegen, node->data.expression.right_val);
    fprintf(codegen->file, ")");
}

// Note: always prints the op covering the operand in parens because its either needed or not harmful
static void gen_unary(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "%s(", op_to_c_string(node->data.unary.op));
    gen_expression(codegen, node->data.unary.operand);
    fprintf(codegen->file, ")");
}

// generates an expression with an index. like xs[i] 
static void gen_index(Codegen *codegen, ASTNode *node){
    Type elem_type = node->type_info->type;

    fprintf(codegen->file, "%slist_get(", unbox_prefix(elem_type));
    gen_expression(codegen, node->data.index.target);
    fprintf(codegen->file, ", ");
    gen_expression(codegen, node->data.index.index_expr);
    fprintf(codegen->file, ")%s", unbox_suffix(elem_type));
}

static void gen_literal(Codegen *codegen, ASTNode *node){
    // list
    if (node->data.literal.type == TYPE_LIST){
        return gen_list_literal(codegen, node); // shouldnt even get here...
    }

    // char. add quotes '
    if (node->data.literal.type == TYPE_CHAR){
        fprintf(codegen->file, "\'%s\'", node->data.literal.value);
        return;
    }

    // string. add quotes "
    if (node->data.literal.type == TYPE_STRING){
        fprintf(codegen->file, "\"%s\"", node->data.literal.value);
        return;
    }

    // bool. true = 1, false = 0
    if (node->data.literal.type == TYPE_BOOL){
        if (strcmp(node->data.literal.value, "true") == 0){
            fprintf(codegen->file, "1");
        }
        else {
            fprintf(codegen->file, "0");
        }

        return;
    }

    // int, float
    fprintf(codegen->file, "%s", node->data.literal.value);
}

// writes the runtime function list_of with the passed variables
static void gen_list_literal(Codegen *codegen, ASTNode *node){
    LinkedASTNode *values = node->data.list_literal.values;
    int amount = 0;

    while (values != NULL){
        amount++;
        values = values->next;
    }

    values = node->data.list_literal.values;

    fprintf(codegen->file, "list_of(%d", amount);

    // add a variadic amount of values
    for (int i = 0; i < amount; i++){
        fprintf(codegen->file, ", ");
        gen_box_item(codegen, node->type_info->inner->type, values->node);

        values = values->next;
    }

    fprintf(codegen->file, ")");
}


/* ----- Builtin generation functions ----- */
// call the corresponding built_in_X function. if it was a prefix then write the remaining stuff
static bool gen_built_in_call(Codegen *codegen, ASTNode *node){
    char *function_name = node->data.function_call.name;
    bool is_prefix = false;

    // only prefix
    if (strcmp(function_name, "random") == 0){
        fprintf(codegen->file, "ting_random(");
        is_prefix = true;
    }

    if (strcmp(function_name, "length") == 0){
        gen_length_prefix(codegen, node);
        is_prefix = true;
    }

    if (strcmp(function_name, "to_int") == 0){
        gen_to_int_prefix(codegen, node);
        is_prefix = true;
    }

    if (strcmp(function_name, "to_float") == 0){
        gen_to_float_prefix(codegen, node);
        is_prefix = true;
    }

    if (strcmp(function_name, "to_char") == 0){
        gen_to_char_prefix(codegen, node);
        is_prefix = true;
    }

    if (strcmp(function_name, "to_string") == 0){
        gen_to_string_prefix(codegen, node);
        is_prefix = true;
    }

    // full call
    if (strcmp(function_name, "add") == 0){
        gen_add_call(codegen, node);
        return true;
    }

    if (strcmp(function_name, "remove") == 0){
        gen_remove_call(codegen, node);
        return true;
    }

    if (!is_prefix){
        return false;
    }

    // only wrote prefix, need to write remaining call.
    LinkedASTNode *args = node->data.function_call.args;

    while (args != NULL){
        gen_expression(codegen, args->node);
        
        if (args->next != NULL){
            fprintf(codegen->file, ", ");
        }

        args = args->next;
    }

    fprintf(codegen->file, ")");

    return true;
}

static void gen_length_prefix(Codegen *codegen, ASTNode *node){
    ASTNode *param = node->data.function_call.args->node;
    
    switch(param->type_info->type){
        case TYPE_STRING:
            fprintf(codegen->file, "(int)strlen(");
            break;

        case TYPE_LIST:
            fprintf(codegen->file, "list_length(");
            break;

        default:
            print_error("Codegen (line %d): length() can only be used on a string or a list. \n", node->line);
    }
}

static void gen_to_int_prefix(Codegen *codegen, ASTNode *node){
    ASTNode *arg = node->data.function_call.args->node;

    switch(arg->type_info->type){
        case TYPE_INT:
        case TYPE_FLOAT:
            fprintf(codegen->file, "(int)(");   // plain C cast
            break;

        case TYPE_STRING:
            fprintf(codegen->file, "string_to_int(");
            break;

        default:
            print_error("Codegen (line %d): to_int() cannot convert this type. \n", node->line);
    }
}

static void gen_to_float_prefix(Codegen *codegen, ASTNode *node){
    ASTNode *arg = node->data.function_call.args->node;

    switch(arg->type_info->type){
        case TYPE_INT:
        case TYPE_FLOAT:
            fprintf(codegen->file, "(float)(");
            break;

        case TYPE_STRING:
            fprintf(codegen->file, "string_to_float(");
            break;

        default:
            print_error("Codegen (line %d): to_float() cannot convert this type. \n", node->line);
    }
}

static void gen_to_char_prefix(Codegen *codegen, ASTNode *node){
    ASTNode *arg = node->data.function_call.args->node;

    switch(arg->type_info->type){
        case TYPE_INT:
            fprintf(codegen->file, "(char)(");
            break;

        case TYPE_STRING:
            fprintf(codegen->file, "string_to_char(");
            break;

        default:
            print_error("Codegen (line %d): to_char() cannot convert this type. \n", node->line);
    }
}

static void gen_to_string_prefix(Codegen *codegen, ASTNode *node){
    ASTNode *arg = node->data.function_call.args->node;

    switch(arg->type_info->type){
        case TYPE_INT:    fprintf(codegen->file, "int_to_string(");   break;
        case TYPE_FLOAT:  fprintf(codegen->file, "float_to_string("); break;
        case TYPE_CHAR:   fprintf(codegen->file, "char_to_string(");  break;
        case TYPE_BOOL:   fprintf(codegen->file, "bool_to_string(");  break;

        case TYPE_STRING:
            fprintf(codegen->file, "(");   // already a string
            break;

        default:
            print_error("Codegen (line %d): to_string() cannot convert this type. \n", node->line);
    }
}

static void gen_add_call(Codegen *codegen, ASTNode *node){
    ASTNode *list_arg  = get_arg(node->data.function_call.args, 0);
    ASTNode *value_arg = get_arg(node->data.function_call.args, 1);

    fprintf(codegen->file, "list_add(");
    gen_expression(codegen, list_arg);
    fprintf(codegen->file, ", ");

    // box the value based on the list's element type
    gen_box_item(codegen, list_arg->type_info->inner->type, value_arg);

    fprintf(codegen->file, ")");
}

static void gen_remove_call(Codegen *codegen, ASTNode *node){
    ASTNode *list_arg  = get_arg(node->data.function_call.args, 0);
    ASTNode *index_arg = get_arg(node->data.function_call.args, 1);

    fprintf(codegen->file, "list_remove(");
    gen_expression(codegen, list_arg);
    fprintf(codegen->file, ", ");
    gen_expression(codegen, index_arg);
    fprintf(codegen->file, ")");
}


/* ----- Inner generation functions ----- */
static void gen_body(Codegen *codegen, LinkedASTNode *statements){
    while (statements != NULL){
        gen_statement(codegen, statements->node);

        statements = statements->next;
    }
}

// Note: doesnt do anything with the default values here, only writes the parameter list as C code
static void gen_function_param(Codegen *codegen, ASTNode *node){
    gen_type(codegen, node->data.function_param.type_info->type);
    fprintf(codegen->file, " %s", node->data.function_param.name);
}

static void gen_type(Codegen *codegen, Type type){
    switch(type){
        case TYPE_INT:
            fprintf(codegen->file, "int");
            break;

        case TYPE_FLOAT:
            fprintf(codegen->file, "float");
            break;

        case TYPE_CHAR:
            fprintf(codegen->file, "char");
            break;

        case TYPE_STRING:
            fprintf(codegen->file, "char *");
            break;

        case TYPE_LIST:
            fprintf(codegen->file, "List *");
            break;

        case TYPE_BOOL:
            fprintf(codegen->file, "int"); // we treat bool as int inside the code
            break;

        case TYPE_VOID:
            fprintf(codegen->file, "void");
            break;

        default: // Note: shouldnt even get here
            print_error("Codegen: unknown type %d.\n", type);
    }
}

// a switch for operators (which are stored in ast nodes as enums)
// to be converted to their respective C code
static const char *op_to_c_string(TokenType operator){
    switch(operator){
        // arithmetic
        case PLUS_TOKEN:      return "+";
        case MINUS_TOKEN:     return "-";
        case MULT_TOKEN:      return "*";
        case DIVIDE_TOKEN:    return "/";
        case MODULO_TOKEN:    return "%";

        // assignment
        case ASSIGN_TOKEN:    return "=";
        case ADD_TO_TOKEN:    return "+=";
        case SUB_TO_TOKEN:    return "-=";
        case MULT_TO_TOKEN:   return "*=";
        case DIVIDE_TO_TOKEN: return "/=";
        case MOD_TO_TOKEN:    return "%=";

        // comparison
        case IS_TOKEN:        return "==";
        case MORE_TOKEN:      return ">";
        case LESS_TOKEN:      return "<";

        // logical
        case AND_TOKEN:       return "&&";
        case OR_TOKEN:        return "||";

        // unary
        case NOT_TOKEN:       return "!";

        default:
            print_error("Codegen: no C operator for token %d. \n", operator);
            return NULL;
    }
}

static TokenType compound_to_binary_op(TokenType op){
    switch(op){
        case ADD_TO_TOKEN:    return PLUS_TOKEN;
        case SUB_TO_TOKEN:    return MINUS_TOKEN;
        case MULT_TO_TOKEN:   return MULT_TOKEN;
        case DIVIDE_TO_TOKEN: return DIVIDE_TOKEN;
        case MOD_TO_TOKEN:    return MODULO_TOKEN;

        default:
            print_error("Codegen: token %d is not a compound assignment operator. \n", op);
            return op;
    }
}

// a switch that returns the type specifier to be used in prints for the passed type
// only takes a type because gen_say will never print a list as one specifier
static const char *type_to_specifier(Type type){
    switch(type){
        case TYPE_INT:    return "%d";
        case TYPE_FLOAT:  return "%f";
        case TYPE_CHAR:   return "%c";
        case TYPE_STRING: return "%s";
        case TYPE_BOOL:   return "%d"; // bools print as 0/1

        default:
            print_error("Codegen: no printf specifier for type %d. \n", type);
            return NULL;
    }
}

// writes the correct boxing function
static void gen_box_item(Codegen *codegen, Type type, ASTNode *value){
    fprintf(codegen->file, "%s", box_prefix(type));
    gen_expression(codegen, value);
    fprintf(codegen->file, "%s", box_suffix(type));
}

// returns the opening of a boxing call. strings and lists are already pointers so they need none
static const char *box_prefix(Type type){
    switch(type){
        case TYPE_INT:    return "box_int(";
        case TYPE_FLOAT:  return "box_float(";
        case TYPE_CHAR:   return "box_char(";
        case TYPE_BOOL:   return "box_bool(";

        case TYPE_STRING:
        case TYPE_LIST:   
            return ""; // already a pointer, no need to box

        default:
            print_error("Codegen: can't box type %d. \n", type);
            return NULL;
    }
}

static const char *box_suffix(Type type){
    if (type == TYPE_STRING || type == TYPE_LIST){
        return ""; // already a pointer, no need to box
    }

    return ")";
}

// returns the opening of a list-element read: an unbox call for scalars, a cast for pointer types
static const char *unbox_prefix(Type type){
    switch(type){
        case TYPE_INT:    return "unbox_int(";
        case TYPE_FLOAT:  return "unbox_float(";
        case TYPE_CHAR:   return "unbox_char(";
        case TYPE_BOOL:   return "unbox_bool(";
        case TYPE_STRING: return "(char *)";   // already a pointer, just cast
        case TYPE_LIST:   return "(List *)";   // already a pointer, just cast

        default:
            print_error("Codegen: can't unbox type %d. \n", type);
            return NULL;
    }
}

// closes the unbox call for scalars. casts have nothing to close
static const char *unbox_suffix(Type type){
    if (type == TYPE_STRING || type == TYPE_LIST){
        return ""; // wasnt boxed to begin with
    }

    return ")";
}

// get the arg in the passed index. used for builtin functions.
// copied straight away from analysis.c cuz cant put it utils due to circle imports
static ASTNode *get_arg(LinkedASTNode *args, unsigned int index){
    unsigned int counter = 0;

    while (args != NULL){
        if (counter == index){
            return args->node;
        }

        counter++;
        args = args->next;
    }

    return NULL;
}