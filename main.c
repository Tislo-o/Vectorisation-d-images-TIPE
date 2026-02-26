#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#include <stdio.h>

//crée un fichier svg de nom filename suivant les dimensions with et height
void creer_svg(const char* filename, const int width, const int height) {
    FILE* f = fopen(filename, "w");
    //Ecriture du header
    fprintf(f, " <svg xmlns=\"http://www.w3.org/2000/svg\" ");
    fprintf(f, " width=\"%dpx\" height=\"%dpx\" ", width, height);
    fprintf(f, " viewBox=\"0 0 %d %d\"> ", 20 , 20);

    //Contenu
    fprintf(f, "<path d=\"M 4 8 L 10 1 L 13 0 L 12 3 L 5 9 C 6 10 6 11 7 10 C 7 11 8 12 7 12 A 1.42 1.42 0 0 1 6 13 A 5 5 0 0 0 4 10 Q 3.5 9.9 3.5 10.5 T 2 11.8 T 1.2 11 T 2.5 9.5 T 3 9 A 5 5 90 0 0 0 7 A 1.42 1.42 0 0 1 1 6 C 1 5 2 6 3 6 C 2 7 3 7 4 8 M 10 1 L 10 3 L 12 3 L 10.2 2.8 L 10 1\" fill=\"#b8ddf0\"/>");

    fprintf(f, "</svg>");
}

int main() {
    int x,y,n;
    unsigned char *data = stbi_load("paysage.jpg", &x, &y, &n, 0);
    printf("largeur: %d   hauteur: %d   nombre de composantes: %d", x, y, n);
    if (data == NULL) {
        printf("Erreur chargement\n");
        return 1;
    }
    stbi_image_free(data);

    creer_svg("resultat.svg", 2000, 2000);
    return 0;
}

