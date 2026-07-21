#include "../include/codegen.h"


// private function declarations
/* ----- Main functions ----- */
static void gen_statement(Codegen *codegen, ASTNode *statement);
static void gen_expression(Codegen *codegen, ASTNode *expression);

/* ----- Statement generation functions ----- */
static void gen_function(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_create_var(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_create_var_list(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_assignment(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_assignment_list(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_if(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_else(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_while(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_repeat(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_repeat_on(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_say(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_return(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_stop(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_skip(Codegen *codegen, ASTNode *node); //* complete!!!

/* ----- Expression generation functions ----- */
static void gen_function_call(Codegen *codegen, ASTNode *node); //! later, function symbols + builtin
static void gen_identifier(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_input(Codegen *codegen, ASTNode *node); //! later, strings as lists? we will tackle that way later
static void gen_binary_expr(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_unary(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_index(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_literal(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_list_literal(Codegen *codegen, ASTNode *node);

/* ----- Builtin generation functions ----- */
static void gen_random_call(Codegen *codegen, ASTNode *node);
static void gen_length_call(Codegen *codegen, ASTNode *node);
static void gen_add_call(Codegen *codegen, ASTNode *node);
static void gen_remove_call(Codegen *codegen, ASTNode *node);
static void gen_to_int_call(Codegen *codegen, ASTNode *node);
static void gen_to_float_call(Codegen *codegen, ASTNode *node);
static void gen_to_char_call(Codegen *codegen, ASTNode *node);
static void gen_to_string_call(Codegen *codegen, ASTNode *node);

/* ----- Inner generation functions ----- */
static void gen_body(Codegen *codegen, LinkedASTNode *statements); //* complete!!!
static void gen_function_param(Codegen *codegen, ASTNode *node); //* complete!!!
static void gen_type(Codegen *codegen, TypeInfo *type_info);
static void gen_box_item(Codegen *codegen, Type type, ASTNode *value); //* complete!!!
static void gen_unbox_item(Codegen *codegen, Type type, const char *list_name, const char *index_name, ASTNode *index_expr); //* complete!!!
static const char *op_to_c_string(TokenType operator); //* complete!!!
static const char *type_to_specifier(Type type); //* complete!!!


/* ----- Codegen "methods" ----- */
// initialize a new codegen struct with all data
// Note: the output file's name doesnt matter because its getting compiled straight away and deleted afterwards
Codegen *init_codegen(void){
    Codegen *new_codegen = malloc(sizeof(Codegen));

    new_codegen->file = fopen(OUTPUT_FILENAME, "w");
    check_nullptr(new_codegen->file, "Codegen: Could not open the output file. \n");

    new_codegen->bracket_depth = 0;
    new_codegen->temp_var_count = 0;
}

// TODO: do it later at the end
void free_codegen(Codegen *codegen){

}


/* ----- Main functions ----- */
// the main function, going through all top level statements and writing them one by one
// works basically the same way parser and analysis work
// also adds libraries ;)
void run_codegen(Codegen *codegen, ASTNode *ast_root){
    fprintf(codegen->file, "#include <stdio.h>\n");
    fprintf(codegen->file, "#include %s\n", RUNTIME_FILENAME);

    LinkedASTNode *statements = ast_root->data.program.statements;

    while (statements != NULL){
        gen_statement(codegen, statements->node);

        statements = statements->next;
    }
}

// based on the node type, redirects to the specific gen functions
static void gen_statement(Codegen *codegen, ASTNode *statement){
    switch(statement->node_type){
        case FUNCTION_NODE: 
            return gen_function(codegen, statement);

        case FUNCTION_CALL_NODE:
            return gen_function_call(codegen, statement);

        case CREATE_VAR_NODE:
            return gen_create_var(codegen, statement);

        case ASSIGNMENT_NODE:
            return gen_assignment(codegen, statement);

        case IF_NODE:
            return gen_if(codegen, statement);

        case ELSE_NODE:
            return gen_else(codegen, statement);

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
    gen_type(codegen, node->data.function.return_type_info);
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

    gen_type(codegen, node->data.create_var.type_info);
    fprintf(codegen->file, " %s", node->data.create_var.name);

    // theres a value to assign too
    if (node->data.create_var.value != NULL){
        fprintf(codegen->file, " = ");
        gen_expression(codegen->file, node->data.create_var.value);
    }

    fprintf(codegen->file, "; \n");
}

// writes a list with possible assignment
// TODO: this
static void gen_create_var_list(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "List *%s = ", node->data.create_var.name);
       
    ASTNode *value = node->data.create_var.value;

    if (value == NULL){
        fprintf(codegen->file, "init_list(); \n", node->data.create_var.name);
        return;
    }

    if (value->node_type == LIST_LITERAL_NODE){
        fprintf(codegen->file, "init_list(); \n", node->data.create_var.name);

        Type elem_type = node->data.create_var.type_info->inner->type;
        LinkedASTNode *values = value->data.list_literal.values;

        while (values != NULL){
            fprintf(codegen->file, "list_add(%s, ", node->data.create_var.name);
            gen_box_item(codegen, elem_type, values->node);
            fprintf(codegen->file, ");\n");

            values = values->next;
        }
        return;
    }

    // its assigned some sort of expression
    gen_expression(codegen, node->data.create_var.value);
    fprintf(codegen->file, "; \n");
}

// <assign>      ::= <lvalue> <assign_op> <expr> "\n"
// <lvalue>      ::= IDENTIFIER ( "[" <expr> "]" )?
static void gen_assignment(Codegen *codegen, ASTNode *node){
    if (node->data.assignment.value->type_info->type == TYPE_LIST){ // lists require a different assignment
        return gen_assignment_list(codegen, node);
    }

    fprintf(codegen->file, "%s", node->data.assignment.name);

    if (node->data.assignment.index_expr != NULL){
        fprintf(codegen->file, "[");
        gen_expression(codegen, node->data.assignment.index_expr); 
        fprintf(codegen->file, "]");
    }

    fprintf(codegen->file, " %s ", op_to_c_string(node->data.assignment.assign_op));
    gen_expression(codegen->file, node->data.assignment.value);
    fprintf(codegen->file, "; \n");
}

// deletes the contents of an existing list and assigns new ones from scratch
// TODO: this
static void gen_assignment_list(Codegen *codegen, ASTNode *node){
    if (node->data.assignment.index_expr != ASSIGN_TOKEN){
        print_error("Codegen (line %d): Cannot do this type of assignment on a list. \n", node->line);
    }
    
    ASTNode *value = node->data.assignment.value;

    if (value->node_type == LIST_LITERAL_NODE){
        fprintf(codegen->file, "list_clear(%d); \n", node->data.assignment.name);

        Type elem_type = value->type_info->inner->type;
        LinkedASTNode *values = value->data.list_literal.values;

        while (values != NULL){
            fprintf(codegen->file, "list_add(%s, ", node->data.assignment.name);
            gen_box_item(codegen, elem_type, values->node);
            fprintf(codegen->file, "); \n");

            values = values->next;
        }
        return;
    }

    // its assigned some sort of expression
    fprintf(codegen->file, "%s = ", node->data.assignment.name);
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
// <repeat_on>   ::= "repeat" "on" "(" IDENTIFIER "in" IDENTIFIER ")" "start" ":" <statement>* "end"
static void gen_repeat_on(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "{ \n"); // open var initialization block

    int id_num = codegen->temp_var_count++;

    // init length. static to start of loop
    fprintf(codegen->file, "int _len%d = list_length(%s); \n", id_num, node->data.repeat_on.list_name);

    // for loop
    fprintf(codegen->file, "for (int _i%d = 0; ", id_num);
    fprintf(codegen->file, "_i%d < _len%d; ", id_num, id_num);
    fprintf(codegen->file, "_i%d += 1) {\n", id_num); // step always +1

    // init loop variable
    gen_type(codegen, node->type_info);
    fprintf(codegen->file, " %s = ", node->data.repeat_on.var_name);

    char index_name[32]; // build the index name "_i%d" to pass as the index to the gen_unbox function
    snprintf(index_name, sizeof(index_name), "_i%d", id_num);

    gen_unbox_item(codegen, node->type_info->type, node->data.repeat_on.list_name, index_name, NULL);
    fprintf(codegen->file, ";\n");

    // continue for loop
    gen_body(codegen, node->data.repeat_on.body);
    fprintf(codegen->file, "}\n");

    fprintf(codegen->file, "} \n"); // close var initialization block
}

// <say>         ::= "say" <expr> "\n"
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

    //? Not sure if needed
    fprintf(codegen->file, "printf(\"\\n\");\n"); // trailing newline for the whole say statement
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
    fprintf(codegen->file, "break; \n");
}

static void gen_skip(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "continue; \n");
}


/* ----- Expression generation functions ----- */
// TODO: handle built in function calls
// TODO: handle default values. need to keep track of all function declarations and add it in call site.
static void gen_function_call(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "%s(", node->data.function_call.name);

    // arguments
    LinkedASTNode *args = node->data.function_call.params;

    while (args != NULL){
        gen_expression(codegen, args->node);
        
        if (args->next != NULL){
            fprintf(codegen->file, ", ");
        }

        args = args->next;
    }

    fprintf(codegen->file, ")");
}

static void gen_identifier(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "%s", node->data.identifier.name);
}

// TODO:
// TODO: needs to distinguish between strings and other types
static void gen_input(Codegen *codegen, ASTNode *node){
    
}

static void gen_binary_expr(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "(");
    gen_expression(codegen, node->data.expression.left_val);
    fprintf(codegen->file, " %s ", op_to_c_string(node->data.expression.op));
    gen_expression(codegen, node->data.expression.right_val);
    fprintf(codegen->file, ")");
}

// TODO: this
// Note: always prints the op covering the operand in parens because its either needed or not harmful
static void gen_unary(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "%c(", op_to_c_string(node->data.unary.op));
    gen_expression(codegen, node->data.unary.operand);
    fprintf(codegen->file, ")");
}

// generates an expression with an index. like xs[i] 
static void gen_index(Codegen *codegen, ASTNode *node){
    gen_unbox_item(codegen, node->type_info->type, node->data.index.list_name, NULL, node->data.index.index_expr);
}

static void gen_literal(Codegen *codegen, ASTNode *node){
    fprintf(codegen->file, "%s", node->data.literal.value);
}

// TODO: this
static void gen_list_literal(Codegen *codegen, ASTNode *node){

}


/* ----- Builtin generation functions ----- */
// TODO: do all of this... later, way later
//? maybe we can handle this differently? maybe add some identifier in analysis to we wont have to go through the distinguishing process again
static void gen_random_call(Codegen *codegen, ASTNode *node){

}

static void gen_length_call(Codegen *codegen, ASTNode *node){

}

static void gen_add_call(Codegen *codegen, ASTNode *node){

}

static void gen_remove_call(Codegen *codegen, ASTNode *node){

}

static void gen_to_int_call(Codegen *codegen, ASTNode *node){

}

static void gen_to_float_call(Codegen *codegen, ASTNode *node){

}

static void gen_to_char_call(Codegen *codegen, ASTNode *node){

}

static void gen_to_string_call(Codegen *codegen, ASTNode *node){

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

// TODO:
static void gen_type(Codegen *codegen, TypeInfo *type_info){

}

// writes the correct boxing function
static void gen_box_item(Codegen *codegen, Type type, ASTNode *value){
    switch(type){
        case TYPE_INT:
            fprintf(codegen->file, "box_int(");
            break;

        case TYPE_FLOAT:
            fprintf(codegen->file, "box_float(");
            break;

        case TYPE_CHAR:
            fprintf(codegen->file, "box_char(");
            break;

        case TYPE_BOOL:
            fprintf(codegen->file, "box_bool(");
            break;

        case TYPE_STRING:
        case TYPE_LIST:
            return gen_expression(codegen, value);

        default:
            print_error("Codegen: can't box type %d.\n", type);
    }

    // for scalars, continue the call
    gen_expression(codegen, value);
    fprintf(codegen->file, ")");
}

// writes the correct unboxing function or casts to a list if typeinfo's a list.
// both gets the item from the list at the index and unboxes it
// Note: *index_name is used for gen_repeat_on while *index_expr is used for other. only one of them should exist
static void gen_unbox_item(Codegen *codegen, Type type, const char *list_name, const char *index_name, ASTNode *index_expr){
    switch(type){ // call correct function based on type
        case TYPE_INT:    
            fprintf(codegen->file, "unbox_int(");
            break;

        case TYPE_FLOAT:  
            fprintf(codegen->file, "unbox_float(");
            break;

        case TYPE_CHAR:   
            fprintf(codegen->file, "unbox_char(");
            break;

        case TYPE_BOOL:   
            fprintf(codegen->file, "unbox_bool(");
            break;

        case TYPE_STRING: 
            fprintf(codegen->file, "(char *)");
            break;
            
        case TYPE_LIST:   
            fprintf(codegen->file, "(List *)");
            break;

        default: 
            print_error("Codegen: can't unbox type %d.\n", type);
    }

    fprintf(codegen->file, "list_get(%s, ", list_name);

    if (index_name != NULL){
        fprintf(codegen->file, "%s", index_name);
    }
    else if (index_expr != NULL){
        gen_expression(codegen, index_expr);
    }

    fprintf(codegen->file, ")");

    // close the unbox_ paren for scalars (casts have no closing paren)
    if (type != TYPE_STRING && type != TYPE_LIST){
        fprintf(codegen->file, ")");
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