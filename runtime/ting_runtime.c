#include <stdio.h>
#include <stdlib.h>
#include "ting_runtime.h"


static void list_realloc(List *list);
static void check_index(int index, int length);


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

// double the size of the list's data
static void list_realloc(List *list){
    list->capacity *= 2;

    list->data = realloc(list->data, sizeof(void *) * list->capacity);
    if (list->data == NULL){
        fprintf(stderr, "Null Pointer Error: Couldn't realloc a list's data. \n");
        exit(1);
    }
}

static void check_index(int index, int length){
    if (index < 0 || index >= length){
        fprintf(stderr, "index %d out of bounds \n", index);
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