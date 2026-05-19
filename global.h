#pragma once
#include <stdlib.h>
#include <stdint.h> 
#include "vector.h"

float COL_THRESHOLD  = 4000.f; //distance au carrée maximale entre 2 couleurs pour qu'elles soient considérées comme identiques
float DEL_THRESHOLD  = 10000.f; //-ratio minimal (nb pixels de l'image / nb pixels d'une forme) pour que la forme 
                              //soit considérée comme trop petite lors de la fusion
                              //-est une majoration du nombre de forme total après fusion
float DISTANCE_THRESHOLD = 10;//tolérance pour l'approximation en polygones



typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef struct  {
    s8 x;
    s8 y;
}s8_tuple;

typedef u32 Pixel;
typedef struct{
    u16 x;
    u16 y;
}Vertex; //sommet



typedef struct{
    u16 width;
    u16 height;
    int channels; //nombre de canaux, 3 ou 4 suivant la présence du canal alpha
    u8* data;
}bitmap_img;

typedef struct{
    u8 red;
    u8 green;
    u8 blue;
}Color;
//une forme est une zone de l'image délimitée par son contour, une liste de sommets ou de pixels, et sa couleur
typedef struct{
    vec* pixels; //tableau redimmensionable des pixels à l'intérieur de la forme
    vec* outline; //tableau redimmensionable des sommets du contour
    Color color;
    u16 ID;   
}shape;
void shape_destroy(void* p) {
    shape* n = p;
    if (n->pixels != NULL) free_vec(n->pixels);
    if (n->outline != NULL) free_vec(n->outline);
}
