#pragma once
#include <assert.h>
#include <stdbool.h>
#include "global.h"

#define REMOVED ~((u32)0)

//array of distinct elements which provides these operations in O(1) time:
//-know if an element is in the array
//-remove an element by value
//-get the minimum element of the array
typedef struct dense_set{
    u32 count;  //number of elements remaining
    u32 N_bot;      //number of elements at the creation of the set, also serves as the end of start of the set
    u32 N_top;     //serves as the end of the set
    u32* next;  //next[i] can equal... 
                //k: the element after i is k
                //N_top: i is the max

    u32* prev;  //prev[i] can equal... 
                //k: the element before i is k
                //N_bot:    no element before i, i is the minimum
                //REMOVED:    i was removed from the set
}dset;
//return a dset with containing all the integers between 0 and count-1
dset* full_dset(u32 count) {
    dset* d = malloc(sizeof(dset));
    d->prev = malloc(sizeof(u32) * (count+2));
    d->next = malloc(sizeof(u32) * (count+2));
    d->count = count;
    d->N_bot = count;
    d->N_top = count+1;
    for (int i = 0; i < count; ++i) {
        d->prev[i] = i-1;
        d->next[i] = i+1;
    }
    d->prev[0] = d->N_bot;
    d->next[d->N_bot] = 0;

    d->prev[d->N_top] = count-1;
    d->next[count-1] = d->N_top;
    return d;
}
void free_dset(dset* d) {
    free(d->prev);
    free(d->next);
    free(d);
}

u32 dset_get_min_element(dset* d) {
    assert(d->count != 0);

    return d->next[d->N_bot];
}
//return if e is in the dset, e must be 
bool is_in_dset(dset* d, u32 e) {
    assert(e < d->N_bot);
    return d->prev[e] != REMOVED;
}
//remove an element that must be in the dset
void dset_remove_element(dset* d, u32 e) {
    assert(e < d->N_bot);

    u32 p = d->prev[e];
    u32 n = d->next[e];

    assert(p != REMOVED);

    d->next[p] = n;
    d->prev[n] = p;

    d->prev[e] = REMOVED;
    d->count--;
}
