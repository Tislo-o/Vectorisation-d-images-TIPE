#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "global.h"
#include "r_string.h"
#include "vector.h"
#include "dense_set.h"

#define N 3

s8_tuple directions[] = {
    {0,1},{0,-1},{1,0},{-1,0}
};
typedef enum Direction {
    BOTTOM = 0,
    TOP = 1,
    RIGHT = 2,
    LEFT = 3,
    ENUM_END
}Direction;
typedef u32 Pixel;



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
//une forme est une zone de l'image délimitée par son contour, une liste de pixels et sa couleur
typedef struct{
    vec* contour; //tableau redimmensionable des pixels du contour
    Color color;
}shape;

void shape_destroy(void* p) {
    shape* n = p;
    free_vec(n->contour);
}
//Si le pixel est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le pixel correspondant
bool pixel_neighbour(bitmap_img* img, Pixel src, Pixel* dst, Direction d) {
    u16 w = img->width;
    u16 x = src % img->width;
    u16 y = src / img->width;
    int nx = x + directions[d].x;
    int ny = y + directions[d].y;
    if (nx < 0 || nx >= img->width || ny < 0 || ny >= img->height) return false;
    *dst = ny * w + nx;
}

//Renvoie la couleur du pixel p dans l'image bitmap img
Color pixel_color(bitmap_img* img, Pixel px) {
    u64 idx = px * img->channels;
    return (Color){img->data[idx], img->data[idx + 1], img->data[idx + 2]};
}
//renvoie si le pixel px est assez proche en couleur de celle de shape
bool is_close_color(bitmap_img* img, shape* s, Pixel px) {
    //Le pixel est assez proche en couleur
    Color c = pixel_color(img, px);
    int dif = (s->color.blue - c.blue)*(s->color.blue - c.blue) + 
              (s->color.green - c.green)*(s->color.green - c.green) + 
              (s->color.red - c.red)*(s->color.red - c.red);
    return dif < 40;
}
//à partir d'une liste de points, construit un chaîne de caractère dans str
//décrivant le path passant par tous les points
r_string* create_path(bitmap_img* img, const Pixel* pixels, int n_points) {
    assert(n_points >= 3);
    u16 w = img->width;

    float x0 = (float)(pixels[0] % w);
    float y0 = (float)(pixels[0] / w);
    float xi, yi, xi1, yi1, xi2, yi2;

    r_string* s = create_r_string("M");

    int i = 0; // indice courant dans la tableau de points
    
    concat_float(s, x0);
    concat_float(s, y0);
    while (i + 2 < n_points) {
        xi = (float)(pixels[i] % w);
        yi = (float)(pixels[i] / w);
        xi1 = (float)(pixels[i+1] % w);
        yi1 = (float)(pixels[i+1] / w);
        xi2 = (float)(pixels[i+2] % w);
        yi2 = (float)(pixels[i+2] / w);
        concat_str(s, " Q");
        concat_float(s, 2.f * xi1 - (xi + xi2) / 2.f);
        concat_float(s, 2.f * yi1 - (yi + yi2) / 2.f);
        concat_float(s, xi2);
        concat_float(s, yi2);
        i += 2;
    }
    if (i + 2 == n_points) { //si le Pixel d'indice i + 1 n'est relié à personne
        concat_str(s, " Q");   //on referme la forme avec une courbe jusqu'au Pixel de départ
        xi = (float)(pixels[i] % w);
        yi = (float)(pixels[i] / w);
        xi1 = (float)(pixels[i+1] % w);
        yi1 = (float)(pixels[i+1] / w);
        concat_float(s, 2.f * (xi1 - (xi + x0) / 2.f));
        concat_float(s, 2.f * (yi1 - (yi + y0) / 2.f));
        concat_float(s, x0);
        concat_float(s, y0);
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

    //fprintf(f, "<path d=\"%s\" fill=\"#b8ddf0\"/>", path);

    fprintf(f, "</svg>");
    fclose(f);
}


//ajoute une forme a la liste de formes shapes
//visited: tableau de n 'false'
void add_shape(bitmap_img* img, vec* shapes, dset* remaining, bool* visited) {
    shape* s = vec_push(shapes);
    s->contour = empty_vec(sizeof(Pixel), NULL);

    u32* stack = malloc(remaining->count * sizeof(u32)); //pile de pixels en attente d'être visité
    stack[0] = dset_give_element(remaining);               //premier pixel disponible
    dset_remove_element(remaining, stack[0]);
    s->color = pixel_color(img, stack[0]);
    u32 stack_size = 1;

    printf("New shape started with px: %d\n", stack[0]);
    while (stack_size > 0) {
        u32 px = stack[stack_size-1]; //on récupère le premier pixel de la pile
        stack_size--;
        printf("stack_size: %d\n", stack_size);
        //Si le pixel a déjà été visité, on l'ignore
        if (visited[px] == true) {
            continue;
        }

        bool is_contour = false;
        for (Direction d = BOTTOM; d < ENUM_END; ++d) {
            Pixel neighbour;

            if (pixel_neighbour(img, px, &neighbour, d) && 
                is_in_dset(remaining, neighbour)  &&
                is_close_color(img, s, neighbour)) {
            
                dset_remove_element(remaining, neighbour);
                printf("FOUND  ");
                stack_size++;
                stack[stack_size-1] = neighbour;
            } else {
                is_contour = true;
            }
        }
        if (is_contour) {
            vec_add(s->contour, &px);
        }      
    }
    free(stack);
}

//renvoie un vec de toutes les formes composant l'image
vec* get_shapes(bitmap_img* img) {

    vec* shapes = empty_vec(sizeof(shape), shape_destroy);
    u32 n = img->height * img->width;
    dset* remaining = full_dset(n);

    bool* blank = malloc(n * sizeof(bool));  //tableau de n 'false' 
    bool* in_shape_visited = malloc(n * sizeof(bool));
    for (int i = 0; i < n; ++i) {
        blank[i] = false;
    } 

    while (remaining->count > 0) {
        //printf("remaining: %d", remaining->count);
        memcpy(in_shape_visited, blank, n * sizeof(bool)); //on réinitialise les pixels visités à 'aucuns'
        add_shape(img, shapes, remaining, in_shape_visited);
        shape* ps = vec_get_element(shapes, shapes->count-1);
        int qsf = 0;
    }
    free_dset(remaining);
    free(blank);
    free(in_shape_visited);

    return shapes;
}
int main() {
    bitmap_img img; 
    img.data = stbi_load("simple.png", (int*)&img.width, (int*)&img.height, &img.channels, 0);
    if (img.data == NULL) {
        printf("Erreur chargement\n");
        return 1;
    }
    vec* shapes = get_shapes(&img);

    for (int i = 0; i < shapes->count; ++i) {
        printf("SHAPE %i:\n");
        shape* s = vec_get_element(shapes, i);
        for (int k = 0; k < s->contour->count; ++k) {
            Pixel* p = vec_get_element(s->contour, k);
            printf("x:%d y:%d\n", *p % img.width, *p / img.width);
        }
    }
    free_vec(shapes);

    stbi_image_free(img.data);  

    //create_svg("resultat.svg");
    return 0;
}

