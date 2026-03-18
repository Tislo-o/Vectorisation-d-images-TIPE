#pragma once
#include <assert.h>
#include <stdbool.h>
#include "global.h"
#define NONE_U32 ~((u32)0)

typedef struct dense_set{
    u32* data;
    u32* indices;
    u32 count;
}dset;

dset* full_dset(u32 count) {
    dset* d = malloc(sizeof(dset) * count);
    d->data = malloc(sizeof(u32) * count);
    d->indices = malloc(sizeof(u32) * count);
    d->count = count;
    for (int i = 0; i < count; ++i) {
        d->data[i] = i;
        d->indices[i] = i;
    }
    return d;
}
void free_dset(dset* d) {
    free(d->data);
    free(d->indices);
    free(d);
}

u32 dset_give_element(dset* d) {
    assert(d->count != 0);
    return d->data[0];
}
bool is_in_dset(dset* d, u32 e) {
    return d->indices[e] != NONE_U32;
}
void dset_remove_element(dset* d, u32 e) {
    d->data[d->indices[e]] = d->data[d->count - 1];
    d->indices[d->data[d->count - 1]] = d->indices[e];
    d->indices[e] = NONE_U32;
    d->count--;
}