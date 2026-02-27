#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "r_string.h"

#define N 3
typedef struct point {
    float x;
    float y;
}point_t;

//à partir d'une liste de points, construit un chaîne de caractère dans str
//décrivant le path passant par tous les points
r_string* create_path(const point_t* points, int n_points) {
    assert(n_points >= 3);

    r_string* s = create_r_string("M");

    int i = 0; // indice courant dans la tableau de points

    concat_float(s, points[0].x);
    concat_float(s, points[0].y);
    while (i + 2 < n_points) {
        concat_str(s, " Q");
        concat_float(s, 2*points[i+1].x - points[i].x / 2 - points[i+2].x / 2);
        concat_float(s, 2*points[i+1].y - points[i].y / 2 - points[i+2].y / 2);
        concat_float(s, points[i+2].x);
        concat_float(s, points[i+2].y);
        i += 2;
    }
    if (i + 2 == n_points) { //si le point d'indice i + 1 n'est relié à personne
        concat_str(s, " Q");   //on referme la forme avec une courbe jusqu'au point de départ
        concat_float(s, 2*points[i+1].x - points[i].x / 2 - points[0].x / 2);
        concat_float(s, 2*points[i+1].y - points[i].y / 2 - points[0].y / 2);
        concat_float(s, points[0].x);
        concat_float(s, points[0].y);
    } else { //tous les points ont été reliés
        concat_str(s, " Z"); //on referme la forme par une droite jusqu'au point de départ
    }
}

//crée un fichier svg de nom 'filename' suivant les dimensions 'width' et 'height'
void create_svg(const char* filename, int width, int height) {
    FILE* f = fopen(filename, "w");
    //Ecriture du header
    fprintf(f, " <svg xmlns=\"http://www.w3.org/2000/svg\" ");
    fprintf(f, " width=\"%dpx\" height=\"%dpx\" ", width, height);
    fprintf(f, " viewBox=\"0 0 %d %d\"> ", 50 , 50);

    point_t pt[N] = {{2.f,2.f}, {46.f,16.f}, {25.f, 26.f}};
    r_string* path = create_path(pt, N);

    fprintf(f, "<path d=\"%s\" fill=\"#b8ddf0\"/>", path->data);
    free_r_string(path);

    //optionnel: dessiner les points
    for (int i = 0; i < N; ++i) {
        fprintf(f, "<circle cx=\"%f\" cy=\"%f\" r=\"1\" fill=\"#d400ff\"/>", pt[i].x, pt[i].y);
    }
    fprintf(f, "</svg>");
    fclose(f);
}

int main() {
    /***  CODE POUR LIRE UNE IMAGE ***/
    // int x,y,n;
    // unsigned char *data = stbi_load("paysage.jpg", &x, &y, &n, 0);
    // printf("largeur: %d   hauteur: %d   nombre de composantes: %d", x, y, n);
    // if (data == NULL) {
    //     printf("Erreur chargement\n");
    //     return 1;
    // }
    // stbi_image_free(data);

    create_svg("resultat.svg", 2000, 2000);
    return 0;
}

