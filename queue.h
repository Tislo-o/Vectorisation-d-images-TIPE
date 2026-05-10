#pragma once
#include "global.h"
#include <assert.h>
#include <string.h>
#include <stdbool.h>
//queue of u32
typedef struct{
    u32* data; 
    u32 next_out; //the slot where the next element to dequeue is 
    u32 count;
    u32 capacity;
}queue;

queue* empty_queue() {
    queue* q = malloc(sizeof(queue));
    q->capacity = 8;
    q->data = malloc(sizeof(u32) * 8);
    q->count = 0;
    q->next_out = 0;
    return q;
}

void free_queue(queue* q) {
    free(q->data);
    free(q);
}

bool is_queue_empty(queue* q) {
    return q->count == 0;
}

void queue_element(queue* q, u32 x) {
    u32 next_in = (q->next_out + q->count) % q->capacity;
    if (q->count == q->capacity) {
        u32 old_cap = q->capacity;
        q->capacity *= 2;
        q->data = realloc(q->data, q->capacity*sizeof(u32));

        memcpy(q->data + old_cap + q->next_out, //works because size is doubled
               q->data + q->next_out, (old_cap - q->next_out)*sizeof(u32));
        q->next_out = q->next_out + old_cap;  
    }
    q->data[next_in] = x;
    q->count++;
}

u32 dequeue_element(queue* q) {
    assert(q->count > 0);
    u32 res = q->data[q->next_out];
    q->count--;
    q->next_out++;
    if (q->next_out == q->capacity) {
        q->next_out = 0;
    }
    return res;
}
