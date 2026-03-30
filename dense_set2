#pragma once
#include <assert.h>
#include <stdbool.h>
#include "global.h"
#define NONE_U32 ~((u32)0)

//SDA : ensemble d'entier naturels
typedef struct dense_set{
    u32* data;
    u32 count; //nombre d'élément
}dset;

/*Renvoie un pointeur vers un dset = {0, 1, ..., count-1} .*/
dset* full_dset(u32 count) {
    dset* d = malloc(sizeof(dset) * count);
    d->data = malloc(sizeof(u32) * count);
    d->count = count;
    for (int i = 0; i < count; ++i) {
        d->data[i] = 0;
    }
    return d;
}

/*Libère la mémoire de d.*/
void free_dset(dset* d) {
    free(d->data);
    free(d);
}

u32 dset_give_ind_min(u32* data, u32 acc) {
    u32 i = data[acc];
    assert(i != NONE_U32);
    if (i == 0) {
        return acc;
    }else{
        return dset_give_ind_min(data, acc + i);
    }
}

/*Renvoie le min de d.*/
u32 dset_give_element(dset* d) {
    assert(d->count != 0);
    u32 i = dset_give_ind_min(d->data, 0);
    data[0] = i; //modifie le tableau pour que le prochain appel soit plus rapide.
    return i;
}

/*Renvoie true si e est dans d sinon renvoie false.*/
bool is_in_dset(dset* d, u32 e) {
    return d->indices[e] == 0;
}

/*Supprime e de d.*/
void dset_remove_element(dset* d, u32 e) {
    u32* data = d->data;
    if (e == d->count - 1){
        data[e] = NONE_U32;
    }else{
        data[e] = data[e + 1] + 1;
    }
    d->count--;
}
