#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <limits.h>
#include "ting_runtime.h"

// private function declarations
/* ----- List methods ----- */
static void list_realloc(List *list);
static void check_index(int index, int length);

/* ----- Input helpers ----- */
static char *read_line(void);


// variable declarations
#define INT_STRING_SIZE   16   // enough for any 32-bit int plus sign and null
#define FLOAT_STRING_SIZE 32   // enough for %f output

static int seeded = 0;


/* ----- List methods ----- */
// allocates a new blank list with capacity of 4
List *init_list(void){
    List *new_list = malloc(sizeof(List));

    new_list->capacity = 4;
    new_list->length = 0;
    new_list->data = malloc(sizeof(void *) * new_list->capacity);

    return new_list;
}

// adds the item to the end of the list
void list_add(List *list, void *item){
    if (list->length == list->capacity){ // need to increase capacity before adding
        list_realloc(list);
    }

    list->data[list->length++] = item;
}

// overrides an existing index in the list with the passed item
void list_set(List *list, int index, void *item){
    check_index(index, list->length);

    list->data[index] = item;
}

void *list_get(List *list, int index){
    check_index(index, list->length);

    return list->data[index];
}

// deletes item at the index and shifts everything so itd stay continuous
void list_remove(List *list, int index){
    check_index(index, list->length);

    // shift everything by one slot starting from the index
    for (int i = index; i < (list->length - 1); i++){
        list->data[i] = list->data[i + 1];
    }

    list->length--;
}

int list_length(List *list){
    return list->length;
}

void list_clear(List *list){
    free(list->data); // TODO: need to free the content recursively i think
    
    list->capacity = 4;
    list->length = 0;

    list->data = malloc(sizeof(void *) * list->capacity);
    if (list->data == NULL){
        fprintf(stderr, "Null Pointer Error\n");
        exit(1);
    }
}

// a variadic function that gets the any amount of variables (and the amount itself)
// and returns a list holding them
List *list_of(int length, ...){
    if (length < 0){
        return NULL;
    }

    List *new_list = init_list();

    va_list args;
    va_start(args, length);

    for (int i = 0; i < length; i++){
        list_add(new_list, va_arg(args, void *));
    }

    va_end(args);
    return new_list;
}

static void check_index(int index, int length){
    if (index < 0 || index >= length){
        fprintf(stderr, "index %d out of bounds \n", index);
        exit(1);
    }   
}

// double the size of the list's data
static void list_realloc(List *list){
    list->capacity *= 2;

    list->data = realloc(list->data, sizeof(void *) * list->capacity);
    if (list->data == NULL){
        fprintf(stderr, "Null Pointer Error: Couldn't realloc a list's data. \n");
        exit(1);
    }
}

// TODO: need to free the content recursively i think
void list_free(List *list){
    free(list->data);
    free(list);
}


/* ----- Box helpers ----- */
void *box_int(int value){
    int *p = malloc(sizeof(int));
    *p = value;
    return p;
}

int unbox_int(void *item){
    return *(int *)item;
}

void *box_float(float value){
    float *p = malloc(sizeof(float));
    *p = value;
    return p;
}

float unbox_float(void *item){
    return *(float *)item;
}

void *box_char(char value){
    char *p = malloc(sizeof(char));
    *p = value;
    return p;
}

char unbox_char(void *item){
    return *(char *)item;
}

void *box_bool(int value){
    int *p = malloc(sizeof(int));
    *p = value; // either 0/1
    return p;
}

int unbox_bool(void *item){
    return *(int *)item;
}


/* ----- Input helpers ----- */
int input_int(void){
    char *line = read_line();
    if (line == NULL){
        fprintf(stderr, "Couldn't read input for an int. \n");
        exit(1);
    }

    int value = string_to_int(line);
    free(line);

    return value;
}


float input_float(void){
    char *line = read_line();
    if (line == NULL){
        fprintf(stderr, "Couldn't read input for a float. \n");
        exit(1);
    }

    float value = string_to_float(line);
    free(line);

    return value;
}

char input_char(void){
    char *line = read_line();
    if (line == NULL){
        fprintf(stderr, "Couldn't read input for a char. \n");
        exit(1);
    }

    char value = line[0];
    free(line);

    return value;
}

char *input_string(void){
    char *line = read_line();
    if (line == NULL){
        fprintf(stderr, "Couldn't read input for a string. \n");
        exit(1);
    }

    return line;
}

// reads the whole line from stdin and returns it
static char *read_line(void){
    char *line = NULL;
    size_t cap = 0;

    ssize_t length = getline(&line, &cap, stdin);
    if (length == -1){
        free(line);
        return NULL;
    }

    // strip the trailing newline if present
    if (length > 0 && line[length - 1] == '\n'){
        line[length - 1] = '\0';
    }

    return line;
}


/* ----- Built in functions ----- */
int ting_random(int min, int max){
    if (min > max){
        fprintf(stderr, "ting_random(int min, int max): min <= max only. \n");
        exit(1);
    }

    if (!seeded){
        srand(time(NULL));
        seeded = 1;
    }

    return min + rand() % (max - min + 1);
}

// Note: casts that dont appear down here are probably simple and done inline through codegen.c

int string_to_int(char *value){
    if (value == NULL || value[0] == '\0'){
        fprintf(stderr, "string_to_int: cannot convert an empty string. \n");
        exit(1);
    }

    char *end;
    errno = 0;
    long result = strtol(value, &end, 10); // base 10

    if (end == value){ // nothing was parsed at all
        fprintf(stderr, "string_to_int: '%s' is not a number. \n", value);
        exit(1);
    }

    if (*end != '\0'){ // there's trailing junk after the number
        fprintf(stderr, "string_to_int: '%s' has trailing characters. \n", value);
        exit(1);
    }

    if (errno == ERANGE || result > INT_MAX || result < INT_MIN){
        fprintf(stderr, "string_to_int: '%s' is out of int range. \n", value);
        exit(1);
    }

    return (int)result;
}

float string_to_float(char *value){
    if (value == NULL || value[0] == '\0'){
        fprintf(stderr, "string_to_float: cannot convert an empty string. \n");
        exit(1);
    }

    char *end;
    errno = 0;
    float result = strtof(value, &end);

    if (end == value){ // nothing was parsed at all
        fprintf(stderr, "string_to_float: '%s' is not a number. \n", value);
        exit(1);
    }

    if (*end != '\0'){ // there's trailing junk after the number
        fprintf(stderr, "string_to_float: '%s' has trailing characters. \n", value);
        exit(1);
    }

    if (errno == ERANGE){ // overflow or underflow
        fprintf(stderr, "string_to_float: '%s' is out of float range. \n", value);
        exit(1);
    }

    return result;
}

// just gives the first character of the string
char string_to_char(char *value){
    if (value == NULL || value[0] == '\0'){
        fprintf(stderr, "string_to_char(): Cannot convert an empty string. \n");
        exit(1);
    }

    return value[0];
}

char *int_to_string(int value){
    char *result = malloc(INT_STRING_SIZE);
    if (result == NULL){
        fprintf(stderr, "int_to_string: malloc failed. \n");
        exit(1);
    }

    snprintf(result, INT_STRING_SIZE, "%d", value);
    return result;
}

char *float_to_string(float value){
    char *result = malloc(FLOAT_STRING_SIZE);
    if (result == NULL){
        fprintf(stderr, "float_to_string: malloc failed. \n");
        exit(1);
    }

    snprintf(result, FLOAT_STRING_SIZE, "%f", value);
    return result;
}

char *char_to_string(char value){
    char *result = malloc(2); // the char + null terminator
    if (result == NULL){
        fprintf(stderr, "char_to_string: malloc failed. \n");
        exit(1);
    }

    result[0] = value;
    result[1] = '\0';
    return result;
}

char *bool_to_string(int value){
    // strdup so the caller can free it like any other converted string
    char *result = strdup(value ? "true" : "false");
    if (result == NULL){
        fprintf(stderr, "bool_to_string: strdup failed. \n");
        exit(1);
    }

    return result;
}