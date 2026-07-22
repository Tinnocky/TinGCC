#ifndef TING_RUNTIME_H
#define TING_RUNTIME_H


typedef struct {
    void **data; // an array of void pointers. each pointer is an index
    int length; // how many elemens are there right now
    int capacity; // how many elements can be until we need to realloc 
} List;


/* ----- List methods ----- */
List *init_list(void);
void list_add(List *list, void *item);
void list_set(List *list, int index, void *item);
void *list_get(List *list, int index);
void list_remove(List *list, int index);
int list_length(List *list);
void list_clear(List *list);
List *list_of(int length, ...);
void list_free(List *list);

/* ----- Box helpers ----- */
void *box_int(int value);
void *box_float(float value);
void *box_char(char value);
void *box_bool(int value);
int   unbox_int(void *item);
float unbox_float(void *item);
char  unbox_char(void *item);
int   unbox_bool(void *item);

/* ----- Input helpers ----- */
int input_int(void);
float input_float(void);
char input_char(void);
char *input_string(void);

/* ----- Built in functions ----- */
int ting_random(int min, int max);

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
char string_to_char(char *value);
char *int_to_string(int value);
char *float_to_string(float value);
char *char_to_string(char value);
char *bool_to_string(int value);


#endif