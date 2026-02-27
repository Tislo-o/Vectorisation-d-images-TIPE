#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
//structure pour une chaîne de caractère redimensionnable 
typedef struct resizeable_string {
    char* data;
    int length; //longueur de la chaîne sans \0
    int size;  // taille de la mémoire allouée
}r_string;

//convertit une chaîne classique en une chaîne redimensionnable 
r_string* create_r_string(const char* src) {
    r_string* s = malloc(sizeof(r_string));

    s->length = strlen(src);
    s->size = s->length + 1;
    s->data = malloc(sizeof(char) * s->size);
    strcpy(s->data, src);

    return s;
}
//réalloue de la mémoire si nécessaire pour agrandir s
//ne modifie pas l'attribut length de s !
void reserve(r_string* s, int new_length) {
    if (new_length >= s->size) {
        int a = new_length + 1;
        int b = s->size * 2;
        int new_size = a > b ? a : b;

        s->data = realloc(s->data, sizeof(char) * new_size);
        assert(s->data != NULL);

        s->size = new_size;
    }
}
//libère la mémoire d'une chaîne redimensionnable
void free_r_string(r_string* s) {
    free(s->data);
    free(s);
}
//concatène un espace et l'écriture de x à la chaîne redimensionnable dst
void concat_float(r_string* dst, float x) {
    int needed = snprintf(NULL, 0, "%f", x); //nombre de caractère nécessaire dans l'écriture de x (sans \0)
    int new_length = dst->length + needed + 1; //+1 pour l'espace 
    reserve(dst, new_length);

    dst->data[dst->length] = ' '; //on concatène un espace à dst
    sprintf(dst->data + dst->length + 1, "%f", x); //on concatène l'écriture de x à dst
    dst->length = new_length;
}
//concatène la chaîne src à la chaîne redimensionnable dst
void concat_str(r_string* dst, char* src) {
    int n = strlen(src);
    int new_length = dst->length + n; 
    reserve(dst, new_length);
    strcat(dst->data, src);
    dst->length = new_length;
}