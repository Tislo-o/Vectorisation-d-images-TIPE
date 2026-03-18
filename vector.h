#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//vec = resizeable array
typedef struct {
    unsigned char* data;
    int stride; //number of bytes per element pointed by data
    int count;   //number of elements
    int capacity; //number of elements allocated
    void (*destroy)(void*); //destroy function of the elements
}vec;
//return an empty vec of elements of stride bytes
vec* empty_vec(int stride, void (*destroy)(void*)) {
    vec* v = malloc(sizeof(vec));
    v->destroy = destroy;
    v->count = 0;
    v->stride = stride;
    v->capacity = 5;
    v->data = malloc(v->capacity * stride);
    return v;
}
//free the vector v
void free_vec(vec* v) {
    for (int i = 0; i < v->count; ++i) {
        v->destroy(v->data + i * v->stride);
    }
    free(v->data);
    free(v);
}
//return a pointer in the v->data array to the ith element of v (first is for i = 0)
void* vec_get_element(vec* v, int i) {
    if (i >= v->count || i < 0) assert(0 && "Access out of bounds of a vector");
    return v->data + i * v->stride;
}

//add the element x containing v->stride bytes to the vec v
void vec_add(vec* v, const void* x) {
    if (v->count == v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, v->capacity * v->stride);
    }
    memcpy(v->data + v->count * v->stride, x, v->stride);
    v->count++;
}
//enlarge the array to contain a new element and return a pointer to it
void* vec_push(vec* v) {
    if (v->count == v->capacity) {
        v->capacity *= 2;
        v->data = realloc(v->data, v->capacity * v->stride);
    }
    void* slot = v->data + v->count * v->stride;
    v->count++;
    return slot;
}
//if x, containg v->stride bytes, is in v, return the pointer to it, else NULL
void* vec_search (vec* v, const void* x) {
    int s = v->stride;
    for (int i = 0; i < v->count; ++i) {
        if (!memcmp(v->data + i * s, x, s)) {
            return v->data + i * s;
        }
    }
    return NULL;
}
//remove the first occurence of the element x containing v->stride bytes from the vector v
//if x isn't in v, raises an error
void vec_remove_by_value(vec* v, const void* x) {
    void* p = vec_search(v, x);
    assert(p != NULL && "Tried to remove an element the wasn't in the vector");
    v->count--;
    memcpy(p, v->data + v->count * v->stride, v->stride); //place the last elements in the new empty slot
}
//remove the element at index i
void vec_remove_by_index(vec* v, int i) {
    void* p = vec_get_element(v, i);
    v->count--;
    memcpy(p, v->data + v->count * v->stride, v->stride); //place the last elements in the new empty slot

}
//change the ith element (starting at 0) of v with new_element containing v->stride bytes
void vec_modify(vec* v, int i, void* new_element) {
    assert(new_element != NULL && "The new_element to add is NULL");
    void* dest = vec_get_element(v, i);
    memcpy(dest, new_element, v->stride);
}

//print the elements of the vector v in hexedecimal
void vec_print_data(vec* v) {
    for (int i = 0; i < v->count; ++i) {
        printf("Element %d | Data ", i);
        for (int k = 0; k < v->stride; ++k) {
            printf("%02X ", v->data[i * v->stride + k]);
        }
        printf("\n");
    }
}



