#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "r_string.h"
#include "vector.h"
#include "global.h"

#define N 3
#define NONE_U32 ~((u32)0)

s8_tuple directions[] = {
    {0,1},{0,-1},{1,0},{-1,0}
};
typedef enum Direction {
    BOTTOM = 0,
    TOP = 1,
    RIGHT = 2,
    LEFT = 8,
    ENUM_END
}Direction;



typedef struct{
    u16 x;
    u16 y;
}Pixel;

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
//une forme est une zone de l'image délimitée par son contour, une liste de pixels
typedef struct{
    vec* contour; //tableau redimmensionable des pixels du contour
    Color color;
}shape;

void shape_destroy(void* p) {
    shape* n = p;
    free_vec(n->contour);
}

//Renvoie la couleur du pixel p dans l'image bitmap img
Color pixel_color(bitmap_img* img, int idx) {
    return (Color){img->data[idx], img->data[idx + 1], img->data[idx + 2]};
}


bool pixel_in_shape(bitmap_img* img, shape* s, Pixel* px, Direction d, u32* ind) {
    //Pixel dans l'image
    int x = (int)px->x + directions[d].x, y = (int)px->y + directions[d].y;
    
    if (x < 0 || x >= img->width || y < 0 || y >= img->height) return false;

    //Le pixel est disponible(pas dans une autre forme)
    int idx = (y * img->width + x) * img->channels;
    if (ind[idx] == NONE_U32) return false;

    //Le pixel est assez proche en couleur
    Color c = pixel_color(img, idx);
    int dif = (s->color.blue - c.blue)*(s->color.blue - c.blue) + 
              (s->color.green - c.green)*(s->color.green - c.green) + 
              (s->color.red - c.red)*(s->color.red - c.red);
    return dif < 40;
}
//à partir d'une liste de points, construit un chaîne de caractère dans str
//décrivant le path passant par tous les points
r_string* create_path(const Pixel* pixels, int n_points) {
    assert(n_points >= 3);

    r_string* s = create_r_string("M");

    int i = 0; // indice courant dans la tableau de points

    concat_float(s, (float)pixels[0].x);
    concat_float(s, (float)pixels[0].y);
    while (i + 2 < n_points) {
        concat_str(s, " Q");
        concat_float(s, 2.f * (float)pixels[i+1].x - (float)(pixels[i].x + pixels[i+2].x) / 2.f);
        concat_float(s, 2.f * (float)pixels[i+1].y - (float)(pixels[i].y + pixels[i+2].y) / 2.f);
        concat_float(s, (float)pixels[i+2].x);
        concat_float(s, (float)pixels[i+2].y);
        i += 2;
    }
    if (i + 2 == n_points) { //si le Pixel d'indice i + 1 n'est relié à personne
        concat_str(s, " Q");   //on referme la forme avec une courbe jusqu'au Pixel de départ
        concat_float(s, 2.f * (float)pixels[i+1].x - (float)(pixels[i].x + pixels[0].x) / 2.f);
        concat_float(s, 2.f * (float)pixels[i+1].y - (float)(pixels[i].y + pixels[0].y) / 2.f);
        concat_float(s, (float)pixels[0].x);
        concat_float(s, (float)pixels[0].y);
    } else { //tous les points ont été reliés
        concat_str(s, " Z"); //on referme la forme par une droite jusqu'au Pixel de départ
    }
}

//crée un fichier svg de nom 'filename' suivant les dimensions 'width' et 'height'
void create_svg(const char* filename) {
    FILE* f = fopen(filename, "w");
    //Ecriture du header
    fprintf(f, " <svg xmlns=\"http://www.w3.org/2000/svg\" ");
    fprintf(f, " width=\"%dpx\" height=\"%dpx\" ", 2000, 2000);
    fprintf(f, " viewBox=\"0 0 %d %d\"> ", 50 , 50);

    Pixel pt[N] = {{2.f,2.f}, {46.f,16.f}, {25.f, 26.f}};
    r_string* path = create_path(pt, N);

    fprintf(f, "<path d=\"%s\" fill=\"#b8ddf0\"/>", path->data);
    free_r_string(path);

    //optionnel: dessiner les points
    for (int i = 0; i < N; ++i) {
        fprintf(f, "<circle cx=\"%d\" cy=\"%d\" r=\"1\" fill=\"#d400ff\"/>", pt[i].x, pt[i].y);
    }
    fprintf(f, "</svg>");
    fclose(f);
}


//ajoute une forme a la liste de formes shapes
//*rem: nombre de pixels restants
//visited: tableau de n 'false'
void add_shape(bitmap_img* img, vec* shapes, u32* av, u32* ind, u32* rem, bool* visited) {
    shape* s = vec_push(shapes);
    s->contour = empty_vec(sizeof(Pixel), NULL);

    u32* stack = malloc(*rem * sizeof(u32)); //pile de pixels en attente d'être visité
    stack[0] = av[0];                        //premier pixel disponible
    s->color = pixel_color(img, av[0]);
    u32 stack_size = 1;


    while (stack_size > 0) {
        u32 idx = stack[stack_size-1];
        Pixel px = (Pixel){idx % img->width, idx / img->width};

        for (Direction d = TOP; d < ENUM_END; ++d) {
            
            if (pixel_in_shape(img, s, &px, d, ind)) {
                av[ind[idx]] = av[*rem - 1];
                ind[av[*rem - 1]] = ind[idx];
                ind[idx] = NONE_U32;
                (*rem)--;

                stack[stack_size];
            } else {
                vec_add(s->contour, idx);
            }
        }
        
    }



    free(stack);
}

//renvoie un vec de toutes les formes composant l'image
vec* get_shapes(bitmap_img* img) {

    vec* shapes = empty_vec(sizeof(shape), shape_destroy);

    u32 n = img->height * img->width;

    u32* available = malloc(n * sizeof(u32)); //pixels restants de l'image
    u32* indices =  malloc(n * sizeof(u32)); //indice des pixels dans le tableau précédent
    bool* blank = malloc(n * sizeof(bool));  //tableau de n 'false' 
    bool* in_shape_visited = malloc(n * sizeof(bool));
    for (int i = 0; i < n; ++i) {
        blank[i] = false;
        available[i] = i;
        indices[i] = i;
    } 
    u32 remaining = n;

    while (remaining > 0) {
        memcpy(in_shape_visited, blank, n * sizeof(bool));
        add_shape(img, shapes, available, indices, &remaining, in_shape_visited);
    }
    free(indices);
    free(available);

    return shapes;
}
int main() {
    bitmap_img img; 
    img.data = stbi_load("dirt.png", (int*)&img.width, (int*)&img.height, &img.channels, 0);
    if (img.data == NULL) {
        printf("Erreur chargement\n");
        return 1;
    }
    //vec* shapes = get_shapes(&img);
    //free_vec(shapes);

    stbi_image_free(img.data);  

    //create_svg("resultat.svg");
    return 0;
}

