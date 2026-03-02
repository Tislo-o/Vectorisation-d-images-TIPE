#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "r_string.h"

#define N 3

typedef struct{
    uint16_t x;
    uint16_t y;
}Pixel;

typedef struct{
    int width;
    int height;
    int channels; //nombre de canaux, 3 ou 4 suivant la présence du canal alpha
    uint8_t* data;
}bitmap_img;

typedef struct{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
}Color;

//Renvoie la couleur du pixel p dans l'image bitmap img
Color pixel_color(bitmap_img* img, Pixel* p) {
    assert(p->x >= 0 && p->y >= 0 && p->x < img->width && p->y < img->height);

    int idx = (p->y * img->width + p->x) * img->channels;

    return (Color){img->data[idx], img->data[idx + 1], img->data[idx + 2]};
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

int main() {
    /***  CODE POUR LIRE UNE IMAGE ***/
    // bitmap_img img; 
    // img.data = stbi_load("paysage.jpg", &img.width, &img.height, &img.channels, 0);
    // if (img.data == NULL) {
    //     printf("Erreur chargement\n");
    //     return 1;
    // }
    // for (int i = 0; i < img.height; ++i) {
    //     for (int j = 0; j < img.width; ++j) {
    //     }
    // }
    // stbi_image_free(img.data);  

    create_svg("resultat.svg");
    return 0;
}

