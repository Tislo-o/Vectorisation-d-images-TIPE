#pragma once
#include <assert.h>
#include <stdbool.h>
#include "global.h"
#define NONE ~((u32)0)
#define REMOVED (NONE-1)

//array of distinct elements which provides these operations in O(1) time:
//-know if an element is in the array
//-remove an element by value
//-get the minimum element of the array
typedef struct dense_set{
    u32 count;  //number of elements remaining
    u32 min;    //minimum of the set
    u32 allocated; //number of elements allocated
    u32* next;  //next[i] can equal... 
                //k: the element after i is k
                //NONE:    no element after i, i is the maximum

    u32* prev;  //prev[i] can equal... 
                //k: the element before i is k
                //NONE:    no element before i, i is the minimum
                //REMOVED:    i was removed from the set
}dset;
//return a dset with containing all the integers between 0 and count-1
dset* full_dset(u32 count) {
    dset* d = malloc(sizeof(dset));
    d->prev = malloc(sizeof(u32) * count);
    d->next = malloc(sizeof(u32) * count);
    d->count = count;
    d->allocated = count;
    d->min = 0;
    for (int i = 0; i < count; ++i) {
        d->prev[i] = i-1;
        d->next[i] = i+1;
    }
    d->prev[0] = NONE;
    d->next[count-1] = NONE;

    return d;
}
void free_dset(dset* d) {
    free(d->prev);
    free(d->next);
    free(d);
}

u32 dset_give_min_element(dset* d) {
    assert(d->count != 0);
    return d->min;
}
//return if e is in the dset, e must be 
bool is_in_dset(dset* d, u32 e) {
    assert(e < d->allocated);
    return d->prev[e] != REMOVED;
}
//remove an element that must be in the dset
void dset_remove_element(dset* d, u32 e) {
    assert(e < d->allocated);

    u32 p = d->prev[e];
    u32 n = d->next[e];

    assert(p != REMOVED);

    if (p != NONE) { 
        d->next[p] = n;
    } else {        //e is the minimum
        d->min = n;
    }
    if (n != NONE) {
        d->prev[n] = p;
    }

    d->prev[e] = REMOVED;
    d->count--;
}
