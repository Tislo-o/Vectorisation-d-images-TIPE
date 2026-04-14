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
    unsigned char* temp_buffer;
    void (*destroy)(void*); //destroy function of the elements
}vec;
//return an empty vec of elements of stride bytes
vec* empty_vec(int stride, void (*destroy)(void*)) {
    vec* v = malloc(sizeof(vec));
    v->destroy = destroy;
    v->count = 0;
    v->stride = stride;
    v->capacity = 10;
    v->temp_buffer = malloc(stride);
    v->data = malloc(v->capacity * stride);
    return v;
}
//free the vector v
void free_vec(vec* v) {
    if (v->destroy != NULL) {
        for (int i = 0; i < v->count; ++i) {
            v->destroy(v->data + i * v->stride);
        }
    }
    free(v->temp_buffer);
    free(v->data);
    free(v);
}
//return a pointer in the v->data array to the ith element of v (first is for i = 0)
static inline void* vec_get_element(vec* v, int i) {
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
//if x, containg v->stride bytes, is in v, return its index, else -1
int vec_search (vec* v, const void* x) {
    int s = v->stride;
    for (int i = 0; i < v->count; ++i) {
        if (!memcmp(v->data + i * s, x, s)) {
            return i;
        }
    }
    return -1;
}
//remove the first occurence of the element x containing v->stride bytes from the vector v
//if x isn't in v, raises an error
void vec_remove_by_value(vec* v, const void* x) {
    int idx = vec_search(v, x);
    assert(idx != -1 && "Tried to remove an element the wasn't in the vector");
    v->count--;
    memcpy(v->data + idx*v->stride, v->data + v->count * v->stride, v->stride); //place the last elements in the new empty slot
}
//remove the element at index i
void vec_remove_by_index(vec* v, int i) {
    void* p = vec_get_element(v, i);
    v->count--;
    v->destroy(v->data + i*v->stride);
    memcpy(p, v->data + v->count * v->stride, v->stride); //place the last elements in the new empty slot

}
//change the ith element (starting at 0) of v with new_element of v->stride bytes
void vec_modify(vec* v, int i, void* new_element) {
    assert(new_element != NULL && "The new_element to add is NULL");
    void* dest = vec_get_element(v, i);
    memcpy(dest, new_element, v->stride);    
    memcpy(dest, new_element, v->stride);
}
//swap the elements i and j in a vec
void vec_swap(vec* v, int i, int j) {
    assert(0 <= i && i < v->count && 0 <= j && j < v->count);
    memcpy(v->temp_buffer, v->data + i * v->stride, v->stride);
    memcpy(v->data + i * v->stride, v->data + j * v->stride, v->stride);
    memcpy(v->data + j * v->stride, v->temp_buffer, v->stride);
}

//replace the content of v between the elements of indices start (included) and end (included)
//with a chunk of chunk_size elements
void vec_replace_chunk(vec* v, int start, int end, void* new_chunk, int chunk_size) {
    assert(start >= 0 && end < v->count && start < end);
    int old_chunk_size = end - start + 1;
    int new_count = chunk_size - old_chunk_size;
    if (new_count > v->capacity) { 
        v->capacity = new_count;
        v->data = realloc(v->data, v->capacity * v->stride);
    }
    //put the part just after end, to its new place
    memcpy(v->data + v->stride * (start + chunk_size), v->data + v->stride * (end+1), v->count - end);
    //put the new_chunk
    memcpy(v->data + v->stride * start, new_chunk, chunk_size);

    v->count = new_count;
}   
//ajoute src à la fin de dst
void vec_concatenate(vec* dst, vec* src) {
    assert(dst->stride == src->stride);
    int new_count = dst->count + src->count;
    if (new_count > dst->capacity) {
        dst->capacity = new_count;
        dst->data = realloc(dst->data, dst->capacity * dst->stride);
    }
    memcpy(dst->data + dst->count * dst->stride, src->data, src->count * src->stride);
    dst->count = new_count;
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



