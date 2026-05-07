#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "vector.h"

#define NO_ARC 65535 //2^16 - 1 valeur maximale d'un u16

/* graphe 
Les valeurs des size noeuds sont [|0, size|] .
Les informations du noeud n dans un tableau sont à l'indice n.
2 noeuds sont reliés par un unique arc. */
typedef struct{
    int size; //number of nodes
    vec* liste; //liste d'adjacence [[voisin ... ], ... ]
    vec* len; //len[n] = longueur de liste[n] 
    vec* mat; //matrice d'adjacence [[poids, ... ], ...]
}graph_t;

// void graph_destroy(void* p){
//     graph_t* n = p;
//     for(int i=0; i<n->data->count; i++){
//         free(vec_get_element(n->data, i));
//     }
//     free_vec(n->data);
// }

void int_tab_destroy(void* p){
    int* n = p;
    free(n);
}

/* Return a heap pointer to an empty graph. */
graph_t* create_graph(){
    graph_t* g;
    g->size = 0;
    g->liste = empty_vec(sizeof(vec*), free_vec);
    g->len = empty_vec(sizeof(int), NULL);
    g->mat = empty_vec(sizeof(vec*), free_vec);
    return g;
}

/* free g */
void free_graph(graph_t* g){
    free_vec(g->len);
    for (int i=0; i<g->size; i++){
        free_vec(g->liste[i]);
        free_vec(g->mat[i]);
    }
    free_vec(g->liste);
    free_vec(g->mat);
    free(g);
}

/* Add a node to g. */
void graph_add_node(graph_t* g){
    g->size ++;
    vec_push(g->len);
    vec_add(g->liste, empty_vec());

    vec* new_col = empty_vec
    vec_add(g->mat, );
    for (int i=0; i<g->size; i++){
    }
}

/* Set the arc n1-n2 of g to have weight w.
Adds nothing if w=0. */
void graph_set_arc(graph_t* g, int n1, int n2, float w){
}

/* Delete a node from g. */
void graph_add_node(graph_t* g){
}

/* Delete an arc from g. */
void graph_del_arc(graph_t* g){
}

/* Return the vec of the neighbours of node n. */
vec* graph_get_nei(graph_t* g, int n){
    return g->liste[n];
}