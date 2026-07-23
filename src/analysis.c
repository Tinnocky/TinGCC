#include <stdlib.h>
#include <string.h>
#include "../include/analysis.h"
#include "../include/utils.h"


// private function declarations
/* ----- TypeInfo "Methods" ----- */
static TypeInfo *copy_type_info(TypeInfo *original);

/* ----- ParamInfo "Methods" ----- */
static ParamInfo *init_param_list(LinkedASTNode *params);
static void add_param_info_data(ParamInfo *param_info, ASTNode *param_node);
static void free_param_info(ParamInfo *params);

/* ----- Symbol "Methods" ----- */
static Symbol *init_symbol_var(char *name, TypeInfo *type, int line); 
static Symbol *init_symbol_func(char *name, TypeInfo *return_type, ParamInfo *params, int line);
static void free_symbol(Symbol *symbol);

/* ----- SymbolTable "Methods" ----- */
static SymbolTable *init_symbol_table(void);
static void table_add(SymbolTable *table, Symbol *symbol);
static Symbol *table_lookup(SymbolTable *table, char *name);
static void free_symbol_table(SymbolTable *table);
static unsigned int hash(char *name);

/* ----- Scope "Methods" (only reachable through Context) ----- */
static void push_scope(Context *context);
static void pop_scope(Context *context);
static inline void scope_add(Context *context, Symbol *symbol);
static Symbol *scope_lookup(Context *context, char *name);
static Symbol *expect_symbol(Context *context, char *name, SymbolKind kind, int line);
static void free_scope(Scope *scope);
static void free_scope_stack(Scope *scope);

/* ----- "Main" Analysis functions ----- */
static void register_functions(Context *context, LinkedASTNode *statements_head);
static void analyze_statement(Context *context, ASTNode *statement);
static TypeInfo *analyze_expression(Context *context, ASTNode *expression);
static TypeInfo *analyze_builtin(Context *context, ASTNode *node);

/* ----- Statement analysis functions ----- */
static void analyze_function(Context *context, ASTNode *node);
static void analyze_create_var(Context *context, ASTNode *node);
static void analyze_assignment(Context *context, ASTNode *node);
static void analyze_if(Context *context, ASTNode *node);
static void analyze_else(Context *context, ASTNode *node);
static void analyze_while(Context *context, ASTNode *node);
static void analyze_repeat(Context *context, ASTNode *node);
static void analyze_repeat_on(Context *context, ASTNode *node);
static void analyze_return(Context *context, ASTNode *node);
static void analyze_skip_stop(Context *context, ASTNode *node);
static void analyze_say(Context *context, ASTNode *node);

/* ----- Expression analysis functions ----- */
static TypeInfo *analyze_function_call(Context *context, ASTNode *node);
static TypeInfo *analyze_identifier(Context *context, ASTNode *node);
static TypeInfo *analyze_input(Context *context, ASTNode *node);
static TypeInfo *analyze_arithmetic_expr(Context *context, ASTNode *node);
static TypeInfo *analyze_comparison_expr(Context *context, ASTNode *node);
static TypeInfo *analyze_logical_expr(Context *context, ASTNode *node);
static TypeInfo *analyze_unary(Context *context, ASTNode *node);
static TypeInfo *analyze_index(Context *context, ASTNode *node);
static TypeInfo *analyze_literal(Context *context, ASTNode *node);
static TypeInfo *analyze_list_literal(Context *context, ASTNode *node);

/* ----- Control Flow functions ----- */
static bool always_returns(LinkedASTNode *statements);
static bool if_always_returns(ASTNode *node);

/* ----- Builtin functions ----- */
static TypeInfo *analyze_random_call(Context *context, ASTNode *node);
static TypeInfo *analyze_length_call(Context *context, ASTNode *node);
static TypeInfo *analyze_add_call(Context *context, ASTNode *node);
static TypeInfo *analyze_remove_call(Context *context, ASTNode *node);
static TypeInfo *analyze_to_int_call(Context *context, ASTNode *node);
static TypeInfo *analyze_to_float_call(Context *context, ASTNode *node);
static TypeInfo *analyze_to_char_call(Context *context, ASTNode *node);
static TypeInfo *analyze_to_string_call(Context *context, ASTNode *node);

/* ---- Inner semantics functions ----- */
static void analyze_body(Context *context, LinkedASTNode *linked_node);
static void expect_bool_condition(Context *context, ASTNode *condition, int line);
static inline bool is_type_numeric(Type type);
static bool types_match(TypeInfo *type_info1, TypeInfo *type_info2);
static bool is_built_in(char *name);
static ASTNode *get_arg(LinkedASTNode *args, unsigned int index);


// variable declarations
#define HASH_SEED 5381
#define HASH_MULTIPLIER 33

char *builtin_names[] = { // names of built in functions
    "random", "length", "add", "remove",
    "to_int", "to_float", "to_char", "to_string",
    NULL
};


/* ----- TypeInfo "Methods" ----- */
// copy a all of a type info into a totally new node
// Note: this doesnt exist in parser but here cuz its only used here. Other methods are there
static TypeInfo *copy_type_info(TypeInfo *original){
    if (original == NULL){
        return NULL;
    }

    TypeInfo *copy = init_type_info(original->type);
    copy->inner = copy_type_info(original->inner); // recursive for lists

    return copy;
}


/* ----- ParamInfo "Methods" ----- */
// make and return the head of a linked list of ParamInfo's
static ParamInfo *init_param_list(LinkedASTNode *params){
    ParamInfo *param_info_head = NULL;
    ParamInfo *curr_param_info = NULL;

    while (params != NULL){
        // new ParamInfo
        ParamInfo *new_param = malloc(sizeof(ParamInfo));
        check_nullptr(new_param, "Analysis: Malloc for ParamInfo failed. \n");
        
        add_param_info_data(new_param, params->node);
        new_param->next = NULL;
        
        // add it to linked list
        if (param_info_head == NULL){
            param_info_head = new_param;
        } 
        else {
            curr_param_info->next = new_param;
        }

        curr_param_info = new_param;
        params = params->next;
    }

    return param_info_head;
}

// add data from the passed *param_node to the passed *param_info
static void add_param_info_data(ParamInfo *param_info, ASTNode *param_node){
    param_info->name = strdup(param_node->data.function_param.name);
    check_nullptr(param_info->name, "...");

    param_info->type_info = copy_type_info(param_node->data.function_param.type_info);

    if (param_node->data.function_param.default_val != NULL){ // optional
        param_info->has_default = true;
    }
    else {
        param_info->has_default = false;
    }  
}

// free all param infos passed 
static void free_param_info(ParamInfo *params){
    while (params != NULL){
        ParamInfo *next = params->next;

        free(params->name);
        free_type_info(params->type_info);
        free(params);

        params = next;
    }
}


/* ----- Symbol "Methods" ----- */
// initialize a symbol structure as a variable (and its *type)
static Symbol *init_symbol_var(char *name, TypeInfo *type, int line){
    Symbol *new_symbol = malloc(sizeof(Symbol));
    check_nullptr(new_symbol, "Analysis: malloc for Symbol failed.\n");

    new_symbol->kind = SYMBOL_VAR;

    new_symbol->name = strdup(name);
    check_nullptr(new_symbol->name, "...");

    new_symbol->line = line;
    new_symbol->data.var.type_info = copy_type_info(type);

    return new_symbol;
} 

// initialize a symbol structure as a function (and its *return_type, *params and param_count)
static Symbol *init_symbol_func(char *name, TypeInfo *return_type, ParamInfo *params, int line){
    Symbol *new_symbol = malloc(sizeof(Symbol));
    check_nullptr(new_symbol, "Analysis: malloc for Symbol failed.\n");

    new_symbol->kind = SYMBOL_FUNC;

    new_symbol->name = strdup(name);
    check_nullptr(new_symbol->name, "...");

    new_symbol->line = line;
    new_symbol->data.func.params = params;
    new_symbol->data.func.return_type_info = copy_type_info(return_type);

    return new_symbol;
}

// free one symbol and its data
static void free_symbol(Symbol *symbol){
    free(symbol->name);

    if (symbol->kind == SYMBOL_VAR){
        free_type_info(symbol->data.var.type_info);
    }
    else if (symbol->kind == SYMBOL_FUNC){
        free_param_info(symbol->data.func.params);
        free_type_info(symbol->data.func.return_type_info);
    }
    else { // not sure if its needed
        print_error("...");
    }

    free(symbol);
}


/* ----- SymbolTable "Methods" ----- */
// initialize a new symbol table with all of its contents set to null
static SymbolTable *init_symbol_table(void){
    SymbolTable *new_table = calloc(1, sizeof(SymbolTable));
    check_nullptr(new_table, "Analysis: calloc for SymbolTable failed.\n");

    return new_table;
}

// add a new symbol to the symbol table by hashing the symbol's name and storing it at that index
static void table_add(SymbolTable *table, Symbol *symbol){
    unsigned int index = hash(symbol->name);

    LinkedSymbol *new_node = malloc(sizeof(LinkedSymbol));
    check_nullptr(new_node, "Analysis: malloc for LinkedSymbol failed.\n");

    new_node->symbol = symbol;

    new_node->next = table->table[index]; // add it to the start of the existing chain at that index
    table->table[index] = new_node; // connect it to the table
}

// go through the symbol table at the name's index and see if a symbol with that name is there
// return it if its there, if not then return null
static Symbol *table_lookup(SymbolTable *table, char *name){
    unsigned int index = hash(name);

    LinkedSymbol *current_node = table->table[index]; 

    while (current_node != NULL){
        if (strcmp(current_node->symbol->name, name) == 0){
            return current_node->symbol;
        }

        current_node = current_node->next;
    }

    return NULL; // symbol does not yet exist

}

// delete a full symbol table
static void free_symbol_table(SymbolTable *table){
    for (int i = 0; i < SYMBOL_TABLE_LENGTH; i++){
        LinkedSymbol *symbols = table->table[i];

        while (symbols != NULL){
            LinkedSymbol *next = symbols->next;

            free_symbol(symbols->symbol);
            free(symbols);

            symbols = next;
        }
    }

    free(table);
}

// my own hash implementation, used for storing symbols inside a symboltable (hash map)
static unsigned int hash(char *name){
    char *name_dup = name;

    unsigned int result = HASH_SEED;

    while ((*name_dup) != '\0'){
        result = result * HASH_MULTIPLIER + (*name_dup);

        name_dup++;
    }

    return result % SYMBOL_TABLE_LENGTH; // has to be an index in the table
}


/* ----- Scope "methods" (only reachable through Context) ----- */
// add a new scope to the top of *context's scope stack
static void push_scope(Context *context){
    Scope *new_scope = calloc(1, sizeof(Scope));
    check_nullptr(new_scope, "Analysis: calloc for Scope failed.\n");

    new_scope->table = init_symbol_table();

    new_scope->parent = context->current_scope;
    context->current_scope = new_scope;
}

// remove the topmost scope from *context's scope stack, make its parent the current scope
static void pop_scope(Context *context){
    Scope *scope_to_pop = context->current_scope;

    context->current_scope = context->current_scope->parent;

    free_scope(scope_to_pop);
}

// add a new symbol to the table living in this current scope
static inline void scope_add(Context *context, Symbol *symbol){
    table_add(context->current_scope->table, symbol);
}

// go through all of the symbol tables in existing scopes starting from the topmost
// and search for a symbol with the same name
static Symbol *scope_lookup(Context *context, char *name){
    Scope *current_scope = context->current_scope;

    while (current_scope != NULL){
        Symbol *symbol = table_lookup(current_scope->table, name);

        if (symbol != NULL){ // symbol found
            return symbol;
        }

        current_scope = current_scope->parent;
    }

    return NULL; // symbol does not yet exist
}

// check if a symbol with the same name + kind exists in the scope
// if it does, return it
static Symbol *expect_symbol(Context *context, char *name, SymbolKind kind, int line){
    Symbol *symbol = scope_lookup(context, name);

    if (symbol == NULL){
        print_error("Analysis (line %d): Symbol %s does not exist in this scope. \n", line, name);
    }

    if (symbol->kind != kind){
        switch (kind){
            case SYMBOL_VAR:
                print_error("Analysis (line %d): Expected variable %s but got a function. \n", line, name);

            case SYMBOL_FUNC:
                print_error("Analysis (line %d): Expected function %s but got a variable. \n", line, name);
            
            default:
                print_error("Analysis (line %d): Unknown symbol kind. \n", line);
        }
    }

    return symbol;
}

// frees one scope
static void free_scope(Scope *scope){
    free_symbol_table(scope->table);
    free(scope);
}

//free a full scope stack
static void free_scope_stack(Scope *scope){
    while (scope != NULL){
        Scope *parent = scope->parent;

        free_scope(scope);

        scope = parent;
    }   
}


/* ----- Context "Methods" ----- */
// initialize a new context
Context *init_context(void){
    Context *context = calloc(1, sizeof(Context));
    check_nullptr(context, "Analysis: calloc for Context failed.\n");

    context->is_inside_loop = false;
    push_scope(context); // push the global scope

    return context;
}

// frees the context and everything thats in it
// doesnt free context->return_type_info because it should be NULL by the time it runs (oh and its borrowed)
void free_context(Context *context){
    free_scope_stack(context->current_scope);
    free(context);
}


/* ----- "Main" Analysis functions ----- */
// go over the ast starting from ast_root (should be a program node)
// for each node, check its specific semantic checks and move forward if theyre ok.
void run_analysis(Context *context, ASTNode *ast_root){
    register_functions(context, ast_root->data.program.statements); // so functions can be called wherever

    // now will go over all of the ast, checking semantics as we go.
    LinkedASTNode *curr_statement = ast_root->data.program.statements;

    while (curr_statement != NULL){
        analyze_statement(context, curr_statement->node);

        curr_statement = curr_statement->next;
    }
}

// check all functions are declared only once
// check they arent named like one of the builtin functions
// register all top level functions to the global context scope
// Note: all other semantic checks for functions are done in analyze_function
static void register_functions(Context *context, LinkedASTNode *statements_head){
    LinkedASTNode *curr_statement = statements_head;

    while (curr_statement != NULL){
        if (curr_statement->node->node_type == FUNCTION_NODE){
            // check if its the name of a builtin
            if (is_built_in(curr_statement->node->data.function.name)){
                print_error(
                    "Analysis (line %d): Cannot name a function '%s' as it is a built-in function. \n",
                    curr_statement->node->line, curr_statement->node->data.function.name
                );
            }

            // check if function was already declared
            Symbol *function = table_lookup(context->current_scope->table, curr_statement->node->data.function.name);

            if (function != NULL){
                print_error("Analysis (line %d): Two or more functions cannot have the same name. \n", curr_statement->node->line);
            }

            // make the ParamInfo list
            ParamInfo *function_params = init_param_list(curr_statement->node->data.function.params);

            // initialize Symbol with data
            Symbol *function_symbol = init_symbol_func(
                curr_statement->node->data.function.name,
                curr_statement->node->data.function.return_type_info,
                function_params,
                curr_statement->node->line
            );
 
            // add it to the global (program) scope
            scope_add(context, function_symbol);
        }

        curr_statement = curr_statement->next;
    }
}

// analyze one full statement, basically redirect to the specific functions
static void analyze_statement(Context *context, ASTNode *statement){
    switch(statement->node_type){
        case FUNCTION_NODE:
            return analyze_function(context, statement);
        
        case FUNCTION_CALL_NODE:
            analyze_function_call(context, statement);
            return; // because analyze_function_call returns a *TypeInfo

        case CREATE_VAR_NODE:
            return analyze_create_var(context, statement);

        case ASSIGNMENT_NODE:
            return analyze_assignment(context, statement);

        case IF_NODE:
            return analyze_if(context, statement); // else statements are handled here too

        case WHILE_NODE:
            return analyze_while(context, statement);

        case REPEAT_NODE:
            return analyze_repeat(context, statement);

        case REPEAT_ON_NODE:
            return analyze_repeat_on(context, statement);

        case RETURN_NODE:
            return analyze_return(context, statement);

        case SKIP_NODE:
        case STOP_NODE:
            return analyze_skip_stop(context, statement);

        case SAY_NODE:
            return analyze_say(context, statement);

        default:
            print_error("Analysis (line %d): Expected a full statement AST node but got %d", statement->line, statement->node_type);
    }

    return;
}

// analyze an expression ast node, redirect to other functions.
// returns the type of the expression (thats the whole process)
static TypeInfo *analyze_expression(Context *context, ASTNode *expression){
    switch(expression->node_type){
        case FUNCTION_CALL_NODE: // also counts as an expression
            return analyze_function_call(context, expression);

        case IDENTIFIER_NODE:
            return analyze_identifier(context, expression);

        case INPUT_NODE:
            return analyze_input(context, expression);

        case ARITHMETIC_EXPR_NODE:
            return analyze_arithmetic_expr(context, expression);

        case COMPARISON_EXPR_NODE:
            return analyze_comparison_expr(context, expression);

        case LOGICAL_EXPR_NODE:
            return analyze_logical_expr(context, expression);

        case UNARY_NODE:
            return analyze_unary(context, expression);

        case INDEX_NODE:
            return analyze_index(context, expression);

        case LITERAL_NODE:
            return analyze_literal(context, expression);

        case LIST_LITERAL_NODE:
            return analyze_list_literal(context, expression);

        default:
            print_error("Analysis (line %d): Expected an expression AST node but got %d", expression->line, expression->node_type);
    }
        
    return NULL;
}

// checks if the function is a builtin
// checks if the right arguments were passed, basically redirect to the specific functions
static TypeInfo *analyze_builtin(Context *context, ASTNode *node){
    char *function_name = node->data.function_call.name;

    if (strcmp(function_name, "random") == 0){
        return analyze_random_call(context, node);
    }
    if (strcmp(function_name, "length") == 0){
        return analyze_length_call(context, node);
    }
    if (strcmp(function_name, "add") == 0){
        return analyze_add_call(context, node);
    }
    if (strcmp(function_name, "remove") == 0){
        return analyze_remove_call(context, node);
    }
    if (strcmp(function_name, "to_int") == 0){
        return analyze_to_int_call(context, node);
    }
    if (strcmp(function_name, "to_float") == 0){
        return analyze_to_float_call(context, node);
    }
    if (strcmp(function_name, "to_char") == 0){
        return analyze_to_char_call(context, node);
    }
    if (strcmp(function_name, "to_string") == 0){
        return analyze_to_string_call(context, node);
    }

    // shouldnt get here since analyze_builtin is only called when function is known to be a builtin
    print_error("Analysis (line %d): unhandled builtin function. \n", node->line);
    return NULL;
}


/* ----- Statement analysis functions ----- */
// check if already inside a function
// check default parameters appear only after all non-default parameters
// go over the body and check all paths return the expected return type (control flow)
static void analyze_function(Context *context, ASTNode *node){
    if (context->current_return_type_info != NULL){
        print_error("Analysis (line %d): Nested function definitions are not allowed. \n", node->line);
    }

    LinkedASTNode *params = node->data.function.params;
    
    bool is_defaults = false; // seen a default param, from now on only default params should appear
    while (params != NULL){
        if (!is_defaults && params->node->data.function_param.default_val != NULL){
            is_defaults = true;
        }

        if (is_defaults && params->node->data.function_param.default_val == NULL){
            print_error(
                "Analysis (line %d): Non-default parameter cannot appear after a default parameter in function '%s'. \n",
                node->line, node->data.function.name
            );
        }

        params = params->next;
    }

    context->current_return_type_info = node->data.function.return_type_info;
    push_scope(context);

    // save parameters in scope
    params = node->data.function.params;

    while (params != NULL){
        char *param_name = params->node->data.function_param.name;
        Symbol *parameter = table_lookup(context->current_scope->table, param_name);
        if (parameter != NULL){
            print_error("Analysis (line %d): '%s' is already declared in this scope. \n", node->line, param_name);
        }

        Symbol *new_parameter = init_symbol_var(
            param_name,
            params->node->data.function_param.type_info,
            params->node->line
        );

        scope_add(context, new_parameter);

        params = params->next;
    }

    analyze_body(context, node->data.function.body);

    if (node->data.function.return_type_info->type != TYPE_VOID &&
        !always_returns(node->data.function.body)){
            print_error("Analysis (line %d): Function '%s' may not return on all paths. \n",
            node->line, node->data.function.name);
    }

    pop_scope(context);
    context->current_return_type_info = NULL;

}

// check if name doesnt already exist in this specific scope (cuz it shadows other scopes)
// check if its assigned expression (if it has one) matches its type
// add variable symbol to table
static void analyze_create_var(Context *context, ASTNode *node){
    Symbol *variable = table_lookup(context->current_scope->table, node->data.create_var.name);

    if (variable != NULL){
        print_error(
            "Analysis (line %d): '%s' is already declared in this scope. \n", node->line, node->data.create_var.name
        );
    }

    bool has_value = node->data.create_var.value != NULL;

    if (has_value){
        TypeInfo *value_type_info = analyze_expression(context, node->data.create_var.value);

        if (!types_match(value_type_info, node->data.create_var.type_info)){
            // error only if not: value_type = int, and variable_type = float
            if (node->data.create_var.type_info->type != TYPE_FLOAT || value_type_info->type != TYPE_INT){
                print_error(
                    "Analysis (line %d): Cannot assign value of wrong type to variable '%s'. \n",
                    node->line, node->data.create_var.name
                );
            }

        }
    }

    Symbol *new_var = init_symbol_var(node->data.create_var.name, node->data.create_var.type_info, node->line);
        
    scope_add(context, new_var);
}

// check if variable does not exist in scope
// check the assigned value and the type match, except for if the value is int and the type is float
static void analyze_assignment(Context *context, ASTNode *node){
    NodeType target_type = node->data.assignment.target->node_type;
    if (target_type != IDENTIFIER_NODE && target_type != INDEX_NODE){
        print_error("Analysis (line %d): Cannot assign to this kind of expression. \n", node->line);
    }

    TypeInfo *target_type_info = analyze_expression(context, node->data.assignment.target); // also checks if it exists in scope
    TypeInfo *value_type_info = analyze_expression(context, node->data.assignment.value);

    if (!types_match(value_type_info, target_type_info)){
        // error only if not: value_type = int, and variable_type = float
        if (value_type_info->type != TYPE_INT || target_type_info->type != TYPE_FLOAT){
            print_error("Analysis (line %d): Cannot assign value of incompatible type. \n", node->line);
        }
    }
}

// check condition expression is a boolean
// go over the body
// go over the else branch
static void analyze_if(Context *context, ASTNode *node){
    expect_bool_condition(context, node->data.if_statement.condition, node->line);

    push_scope(context);
    analyze_body(context, node->data.if_statement.body);
    pop_scope(context);

    LinkedASTNode *curr_else = node->data.if_statement.else_branch;
    while (curr_else != NULL){
        analyze_else(context, curr_else->node);

        curr_else = curr_else->next;
    }
}

// check condition expression is a boolean or null (for no expression)
// go over the body
// Note: only called through analyze_if
static void analyze_else(Context *context, ASTNode *node){
    if (node->data.else_statement.condition != NULL){
        expect_bool_condition(context, node->data.else_statement.condition, node->line);
    }

    push_scope(context);
    analyze_body(context, node->data.else_statement.body);
    pop_scope(context);
}

// check condition expression is a boolean
// go over the body 
static void analyze_while(Context *context, ASTNode *node){
    expect_bool_condition(context, node->data.while_loop.condition, node->line);

    bool prev_is_inside_loop = context->is_inside_loop;
    context->is_inside_loop = true;
    push_scope(context);

    analyze_body(context, node->data.while_loop.body);

    pop_scope(context);
    context->is_inside_loop = prev_is_inside_loop;
}

// check from and to variables are int
// check step is an int or NULL
// add the loop variable to scope as an int
// go over the body
static void analyze_repeat(Context *context, ASTNode *node){
    TypeInfo *from_type_info = analyze_expression(context, node->data.repeat.from);
    TypeInfo *to_type_info = analyze_expression(context, node->data.repeat.to);

    if (from_type_info->type != TYPE_INT || to_type_info->type != TYPE_INT){
        print_error("Analysis (line %d): Repeat loop range values must be integers. \n", node->line);
    }

    if (node->data.repeat.step != NULL){
        TypeInfo *step_type_info = analyze_expression(context, node->data.repeat.step);

        if (step_type_info->type != TYPE_INT){
            print_error("Analysis (line %d): Repeat loop step value must be an integer. \n", node->line);
        }
    }

    bool prev_is_inside_loop = context->is_inside_loop;
    context->is_inside_loop = true;
    push_scope(context);

    TypeInfo *loop_var_type_info = init_type_info(TYPE_INT);

    Symbol *loop_variable = init_symbol_var(
        node->data.repeat.var_name,
        loop_var_type_info,
        node->line
    );
    scope_add(context, loop_variable);

    free_type_info(loop_var_type_info); // since its getting copied in init_symbol_var we dont need this one

    analyze_body(context, node->data.repeat.body);

    pop_scope(context);
    context->is_inside_loop = prev_is_inside_loop;
}

// check list exists in scope
// add the loop variable to scope
// go over the body
static void analyze_repeat_on(Context *context, ASTNode *node){
    TypeInfo *target_type_info = analyze_expression(context, node->data.repeat_on.target);

    if (target_type_info->type != TYPE_LIST){
        print_error("Analysis (line %d): Can only iterate over a list. \n", node->line);
    }

    node->type_info = copy_type_info(target_type_info->inner); // keep element type for codegen

    bool prev_is_inside_loop = context->is_inside_loop;
    context->is_inside_loop = true;
    push_scope(context);

    Symbol *loop_variable = init_symbol_var(
        node->data.repeat_on.var_name,
        target_type_info->inner,
        node->line
    );
    scope_add(context, loop_variable);

    analyze_body(context, node->data.repeat_on.body);

    pop_scope(context);
    context->is_inside_loop = prev_is_inside_loop;
}

// check if were inside a function
// check if the function's return type and the return expression match
static void analyze_return(Context *context, ASTNode *node){
    if (context->current_return_type_info == NULL){
        print_error("Analysis (line %d): Return cannot be called outside of a function. \n", node->line);
    }

    // for void returns, the value is null cuz they return nothing
    // specific check for that because we shouldnt run it in analyze_expression
    if (node->data.return_statement.value == NULL){
        if (context->current_return_type_info->type != TYPE_VOID){
            print_error("Analysis (line %d): Does not return the same type as the function. \n", node->line);     
        }
        
        return; // its a void return returning nothing, all good
    }

    TypeInfo *value_type_info = analyze_expression(context, node->data.return_statement.value);
    if (!types_match(context->current_return_type_info, value_type_info)){
        print_error("Analysis (line %d): Does not return the same type as the function. \n", node->line);
    }
}

// check if its called inside a loop
static void analyze_skip_stop(Context *context, ASTNode *node){
    if (!context->is_inside_loop){
        print_error("Analysis (line %d): SKIP/STOP cannot be called outside of a loop. \n", node->line);
    }
}

// checks if all expressions are valid
static void analyze_say(Context *context, ASTNode *node){
    LinkedASTNode *values = node->data.say.values;

    while (values != NULL){
        ASTNode *value = values->node;

        TypeInfo *value_type_info = analyze_expression(context, value);

        if (value_type_info->type == TYPE_VOID){
            print_error("Analysis (line %d): SAY cannot print a void expression. \n", node->line);
        }

        values = values->next;
    }
}


/* ----- Expression analysis functions ----- */
// check if function is one of the built-in functions
// check if function exists in scope
// check if all of the arguments match the function's paramters
static TypeInfo *analyze_function_call(Context *context, ASTNode *node){
    if (is_built_in(node->data.function_call.name)){
        return analyze_builtin(context, node);
    }

    Symbol *function = expect_symbol(context, node->data.function_call.name, SYMBOL_FUNC, node->line);

    ParamInfo *curr_parameter = function->data.func.params;
    LinkedASTNode *curr_argument = node->data.function_call.args;
    int arg_index = 1; // used for the error message

    // check if all parameters and arguments match, none are missed
    while (curr_parameter != NULL){
        if (curr_argument == NULL){ 
            if (!curr_parameter->has_default){
                print_error("Analysis (line %d): Function '%s' call has too few arguments. \n", node->line, function->name);
            }

            // its a default parameter so its okay if no argument is passed
            curr_parameter = curr_parameter->next;
            continue; // either another parameter with default value or loop ends
        }

        TypeInfo *arg_type_info = analyze_expression(context, curr_argument->node);
        if (!types_match(curr_parameter->type_info, arg_type_info)){
            print_error("Analysis (line %d): Argument number %d in call to '%s' does not match expected parameter type '%s'. \n",
                node->line, arg_index, function->name, curr_parameter->name);
        }

        curr_parameter = curr_parameter->next;
        curr_argument = curr_argument->next;
        arg_index++;
    }

    // function parameters over, check if there are more arguments
    if (curr_argument != NULL){
        print_error("Analysis (line %d): Function '%s' call has too many arguments. \n", node->line, function->name);
    }

    node->type_info = copy_type_info(function->data.func.return_type_info);
    return node->type_info;
}

// check if a variable with that name exists
// Note: this is used to check existing identifier's type, called in expressions.
// Note: Other instances of using identifiers dont go through here
static TypeInfo *analyze_identifier(Context *context, ASTNode *node){
    Symbol *identifier = expect_symbol(context, node->data.identifier.name, SYMBOL_VAR, node->line);

    node->type_info = copy_type_info(identifier->data.var.type_info);
    return node->type_info;
}

// nothing to check, just assign val
static TypeInfo *analyze_input(Context *context, ASTNode *node){
    node->type_info = init_type_info(node->data.input.type);
    return node->type_info;
}

// check if both types are int or float
// decide what type is the result (only int = int, has float = float)
static TypeInfo *analyze_arithmetic_expr(Context *context, ASTNode *node){
    TypeInfo *left_type_info = analyze_expression(context, node->data.expression.left_val);
    TypeInfo *right_type_info = analyze_expression(context, node->data.expression.right_val);

    if (!is_type_numeric(left_type_info->type) || !is_type_numeric(right_type_info->type)){
        print_error("Analysis (line %d): Both sides of an arithmetic expression must be either int or float. \n", node->line);
    }

    if (left_type_info->type == TYPE_FLOAT || right_type_info->type == TYPE_FLOAT){
        node->type_info = init_type_info(TYPE_FLOAT);
    }
    else {
        node->type_info = init_type_info(TYPE_INT);
    }

    return node->type_info;
}

// check if both expressions are the same type
static TypeInfo *analyze_comparison_expr(Context *context, ASTNode *node){
    TypeInfo *left_type_info = analyze_expression(context, node->data.expression.left_val);
    TypeInfo *right_type_info = analyze_expression(context, node->data.expression.right_val);

    if (!types_match(left_type_info, right_type_info)){
        print_error("Analysis (line %d): Both sides of a comparison must be the same type. \n", node->line);   
    }

    node->type_info = init_type_info(TYPE_BOOL); // comparison expressions are boolean
    return node->type_info;
}

// check if both expressions are boolean
static TypeInfo *analyze_logical_expr(Context *context, ASTNode *node){
    TypeInfo *left_type_info = analyze_expression(context, node->data.expression.left_val);
    TypeInfo *right_type_info = analyze_expression(context, node->data.expression.right_val);

    if (left_type_info->type != TYPE_BOOL || right_type_info->type != TYPE_BOOL){
        print_error("Analysis (line %d): Both sides of a logical expression must be boolean. \n", node->line);   
    }

    node->type_info = init_type_info(TYPE_BOOL);
    return node->type_info;
}

// check for if the operator is NOT then the operand is boolean
// check for if the operator is MINUS then the operand is numeric
static TypeInfo *analyze_unary(Context *context, ASTNode *node){
    TypeInfo *operand_type_info = analyze_expression(context, node->data.unary.operand);

    switch(node->data.unary.op){
        case NOT_TOKEN:
            if (operand_type_info->type != TYPE_BOOL){
                print_error("Analysis (line %d): The NOT operator can only be applied to a boolean expression. \n", node->line);    
            } 

            node->type_info = init_type_info(TYPE_BOOL);
            break;

        case MINUS_TOKEN:
            if (!is_type_numeric(operand_type_info->type)){
                print_error("Analysis (line %d): The unary minus operator can only be applied to either int or float. \n", node->line);    
            }

            node->type_info = init_type_info(operand_type_info->type);
            break;

        default: // honestly, should never happen as its checked way back lexing
            print_error("Analysis (line %d): Expected an unary operator but got token %d. \n", node->line, node->data.unary.op);  
    }

    return node->type_info;
}

// check if list exists in scope
// check the index expression is an int
static TypeInfo *analyze_index(Context *context, ASTNode *node){
    TypeInfo *target_type_info = analyze_expression(context, node->data.index.target); // this also checks if it exists in scope

    if (target_type_info->type != TYPE_LIST){
        print_error("Analysis (line %d): Can only index on a list variable. \n", node->line);
    }

    TypeInfo *index_type_info = analyze_expression(context, node->data.index.index_expr);
    if (index_type_info->type != TYPE_INT){
        print_error("Analysis (line %d): Index expression should be an integer. \n", node->line);
    }

    node->type_info = copy_type_info(target_type_info->inner); // hold the type of element the index gets
    return node->type_info;
}

static TypeInfo *analyze_literal(Context *context, ASTNode *node){
    node->type_info = init_type_info(node->data.literal.type);
    return node->type_info;
}

// check if list literal holds anything (it has to hold something so we know its type)
// check if all values inside it are the same type
// Note: Although sometimes int expressions turn to float, we cant create a float list like [1, 2.0, 2.5, 3].
// Note: Perhaps ill add that in the future...
static TypeInfo *analyze_list_literal(Context *context, ASTNode *node){
    if (node->data.list_literal.values == NULL){
        print_error("Analysis (line %d): A list literal has to hold atleast one value. \n", node->line);
    }

    TypeInfo *first_value_type_info = analyze_expression(context, node->data.list_literal.values->node); // will check for this type
    LinkedASTNode *linked_value = node->data.list_literal.values->next; // loop from the 2nd value (the first was just checked)
    while (linked_value != NULL){
        TypeInfo *value_type_info = analyze_expression(context, linked_value->node);
        if (!types_match(value_type_info, first_value_type_info)){
            print_error("Analysis (line %d): All values in a list literal must be the same type. \n", node->line);
        }

        linked_value = linked_value->next;
    }

    node->type_info = init_type_info(TYPE_LIST);
    node->type_info->inner = copy_type_info(first_value_type_info);
    return node->type_info;
}


/* ----- Control Flow functions ----- */
// analyze the control flow of a function, meaning if everything returns what it needed to.
// return bool for if the function definitely returns or no.
static bool always_returns(LinkedASTNode *statements){

    while (statements != NULL){
        if (statements->node->node_type == RETURN_NODE){
            return true;
        }

        if (statements->node->node_type == IF_NODE){
            if (if_always_returns(statements->node)){ // if it definitely returns, then return true. if not, keep looking.
                return true;
            }
        }

        statements = statements->next;
    }

    return false;
}

// check if an if statement and its else branch definitely returns.
// for it to definitely return it needs: 1) all branches return, 2) end with an "else" without any condition
static bool if_always_returns(ASTNode *node){
    if (node->data.if_statement.else_branch == NULL){
        return false;
    }

    if (!always_returns(node->data.if_statement.body)){
            return false;
    }

    // go over else branch
    LinkedASTNode *else_node = node->data.if_statement.else_branch;
    bool seen_else_without_condition = false;

    while (else_node != NULL){
        if (!seen_else_without_condition && else_node->node->data.else_statement.condition == NULL){
            seen_else_without_condition = true;
        }

        if (!always_returns(else_node->node->data.else_statement.body)){
            return false;
        }

        else_node = else_node->next;
    }

    return seen_else_without_condition;
}


/* ----- Builtin functions ----- */
// random(int min, int max)
static TypeInfo *analyze_random_call(Context *context, ASTNode *node){
    LinkedASTNode *args = node->data.function_call.args;

    if (get_arg(args, 0) == NULL || get_arg(args, 1) == NULL){
        print_error("Analysis (line %d): random() requires 2 arguments. \n", node->line);
    }

    TypeInfo *min_type_info = analyze_expression(context, get_arg(args, 0));
    TypeInfo *max_type_info = analyze_expression(context, get_arg(args, 1));

    if (min_type_info->type != TYPE_INT || max_type_info->type != TYPE_INT){
        print_error("Analysis (line %d): random() arguments must be integers. \n", node->line);
    }

    node->type_info = init_type_info(TYPE_INT);
    return node->type_info;
}

// length(list or string x)
static TypeInfo *analyze_length_call(Context *context, ASTNode *node){
    LinkedASTNode *args = node->data.function_call.args;

    if (get_arg(args, 0) == NULL){
        print_error("Analysis (line %d): length() requires 1 argument. \n", node->line);
    }

    TypeInfo *arg_type_info = analyze_expression(context, get_arg(args, 0));

    if (arg_type_info->type != TYPE_LIST && arg_type_info->type != TYPE_STRING){
        print_error("Analysis (line %d): length() argument must be a list or string. \n", node->line);
    }

    node->type_info = init_type_info(TYPE_INT);
    return node->type_info;
}

// add(list, list->inner val)
static TypeInfo *analyze_add_call(Context *context, ASTNode *node){
    LinkedASTNode *args = node->data.function_call.args;

    if (get_arg(args, 0) == NULL || get_arg(args, 1) == NULL){
        print_error("Analysis (line %d): add() requires 2 arguments. \n", node->line);
    }

    TypeInfo *list_type_info = analyze_expression(context, get_arg(args, 0));
    TypeInfo *val_type_info = analyze_expression(context, get_arg(args, 1));

    if (list_type_info->type != TYPE_LIST){
        print_error("Analysis (line %d): add() first argument must be a list. \n", node->line);
    }

    if (!types_match(list_type_info->inner, val_type_info)){
        print_error("Analysis (line %d): add() value type does not match list element type. \n", node->line);
    }

    node->type_info = init_type_info(TYPE_VOID);
    return node->type_info;
}

// remove(list, int index)
static TypeInfo *analyze_remove_call(Context *context, ASTNode *node){
    LinkedASTNode *args = node->data.function_call.args;

    if (get_arg(args, 0) == NULL || get_arg(args, 1) == NULL){
        print_error("Analysis (line %d): remove() requires 2 arguments. \n", node->line);
    }

    TypeInfo *list_type_info = analyze_expression(context, get_arg(args, 0));
    TypeInfo *index_type_info = analyze_expression(context, get_arg(args, 1));

    if (list_type_info->type != TYPE_LIST){
        print_error("Analysis (line %d): remove() first argument must be a list. \n", node->line);
    }

    if (index_type_info->type != TYPE_INT){
        print_error("Analysis (line %d): remove() index must be an integer. \n", node->line);
    }

    node->type_info = init_type_info(TYPE_VOID);
    return node->type_info;
}

// to_int(x)
static TypeInfo *analyze_to_int_call(Context *context, ASTNode *node){
    LinkedASTNode *args = node->data.function_call.args;

    if (get_arg(args, 0) == NULL){
        print_error("Analysis (line %d): to_int() requires 1 argument. \n", node->line);
    }

    TypeInfo *arg_type_info = analyze_expression(context, get_arg(args, 0));

    if (!is_type_numeric(arg_type_info->type) && arg_type_info->type != TYPE_STRING){
        print_error("Analysis (line %d): to_int() argument must be numeric or string. \n", node->line);
    }

    node->type_info = init_type_info(TYPE_INT);
    return node->type_info;
}

// to_float(x)
static TypeInfo *analyze_to_float_call(Context *context, ASTNode *node){
    LinkedASTNode *args = node->data.function_call.args;

    if (get_arg(args, 0) == NULL){
        print_error("Analysis (line %d): to_float() requires 1 argument. \n", node->line);
    }

    TypeInfo *arg_type_info = analyze_expression(context, get_arg(args, 0));

    if (!is_type_numeric(arg_type_info->type) && arg_type_info->type != TYPE_STRING){
        print_error("Analysis (line %d): to_float() argument must be numeric or string. \n", node->line);
    }

    node->type_info = init_type_info(TYPE_FLOAT);
    return node->type_info;
}

// to_char(x)
static TypeInfo *analyze_to_char_call(Context *context, ASTNode *node){
    LinkedASTNode *args = node->data.function_call.args;

    if (get_arg(args, 0) == NULL){
        print_error("Analysis (line %d): to_char() requires 1 argument. \n", node->line);
    }

    TypeInfo *arg_type_info = analyze_expression(context, get_arg(args, 0));

    if (arg_type_info->type != TYPE_INT && arg_type_info->type != TYPE_STRING){
        print_error("Analysis (line %d): to_char() argument must be int or string. \n", node->line);
    }

    node->type_info = init_type_info(TYPE_CHAR);
    return node->type_info;
}

// to_string(x)
static TypeInfo *analyze_to_string_call(Context *context, ASTNode *node){
    LinkedASTNode *args = node->data.function_call.args;

    if (get_arg(args, 0) == NULL){
        print_error("Analysis (line %d): to_string() requires 1 argument. \n", node->line);
    }

    analyze_expression(context, get_arg(args, 0)); // any type is fine

    node->type_info = init_type_info(TYPE_STRING);
    return node->type_info;
}


/* ----- Inner semantics functions ----- */
static void analyze_body(Context *context, LinkedASTNode *linked_node){
    while (linked_node != NULL){
        analyze_statement(context, linked_node->node);

        linked_node = linked_node->next;
    }
}

static void expect_bool_condition(Context *context, ASTNode *condition, int line){
    TypeInfo *condition_type_info = analyze_expression(context, condition);

    if (condition_type_info->type != TYPE_BOOL){
        print_error("Analysis (line %d): Condition must be a boolean expression. \n", line);
    }
}

// Note: true if both nodes null, false if only one is.
static inline bool is_type_numeric(Type type){
    return type == TYPE_INT || type == TYPE_FLOAT;
}

// recursively, check if the type info's types match exactly.
static bool types_match(TypeInfo *type_info1, TypeInfo *type_info2){
    if (type_info1 == NULL && type_info2 == NULL){
        return true;
    }
    else if (type_info1 == NULL || type_info2 == NULL){ // only one node is null
        return false;
    }

    if (type_info1->type != type_info2->type){
        return false;
    }

    if (type_info1->type == TYPE_LIST){ // check if inner types match
        return types_match(type_info1->inner, type_info2->inner);
    }

    return true;
}

// check if a function with that name is one of the built-in functions
static bool is_built_in(char *name){
    int builtin_index = 0;

    while (builtin_names[builtin_index] != NULL){
        if (strcmp(builtin_names[builtin_index], name) == 0){
            return true;
        }

        builtin_index++;
    }
    return false;
}


// get the arg in the passed index. used for builtin functions.
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