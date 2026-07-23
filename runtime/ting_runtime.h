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
int string_to_int(char *value);
float string_to_float(char *value);
char string_to_char(char *value);
char *int_to_string(int value);
char *float_to_string(float value);
char *char_to_string(char value);
char *bool_to_string(int value);


#endif