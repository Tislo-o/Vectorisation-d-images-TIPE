#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"

#include <time.h>
#include "shape_extraction.h"
#include "polygons.h"

r_string* create_path(bitmap_img* img, vec* v) {
    u32 n_points = v->count;

    u16 w = img->width;

    Vertex* v0 = (Vertex*)vec_get_element(v, 0);
    float x0 = v0->x;
    float y0= v0->y;

    r_string* s = create_r_string("M");

    concat_float(s, x0);
    concat_float(s, y0);
    for (int i = 1; i < n_points; ++i) {
        concat_str(s, " L");

        Vertex* vi = (Vertex*)vec_get_element(v, i);
        concat_float(s, (float)vi->x);
        concat_float(s, (float)vi->y);
    }

    return s;
}

//crée un fichier svg de nom 'filename' 
void create_svg(const char* filename, bitmap_img* img, vec* shapes) {
    FILE* f = fopen(filename, "w");
    u16 width, height;
    if (img->width > img->height) {
        width = 2000;
        height = (img->height * 2000) / img->width;
    } else {
        height = 2000;
        width = (img->width * 2000) / img->height;
    }
    //Ecriture du header
    fprintf(f, " <svg xmlns=\"http://www.w3.org/2000/svg\" ");
    fprintf(f, " width=\"%dpx\" height=\"%dpx\" ", width, height);
    fprintf(f, " viewBox=\"0 0 %d %d\"> ", img->width, img->height);

    for (int i = 0; i < shapes->count; ++i) {
        shape* s = vec_get_element(shapes, i);
        r_string* path = create_path(img, s->outline);
        fprintf(f, "<path d=\"%s\" fill=\"rgb(%d, %d, %d)\"/>", path->data, s->color.red, s->color.green, s->color.blue);
        free_r_string(path);
    }
    fprintf(f, "</svg>");
    fclose(f);
}

void transform_into_polygons(vec* shapes, bitmap_img* img) {
    for (int i = 0; i < shapes->count; ++i) {
        shape* s = vec_get_element(shapes, i);
        
        graph* g = outline_to_graph(s->outline);

        vec* polygon = shortest_cycle(g); //indices in the outline array that form the shortest polygon
        vec* old_outline = s->outline;
        s->outline = empty_vec(sizeof(Vertex), NULL);
        for (int j = 0; j < polygon->count; ++j) {
            vec_add(s->outline, vec_get_element(old_outline, *(u32*)vec_get_element(polygon, j)));
        }
        free_graph(g);
        free_vec(polygon);
        free_vec(old_outline);
    }
}

int main(int argc, char** argv) {
    if (argc < 3){
        printf("Donnez le nom de l'image en argument et le nom de l'image  à générer.\n");
        return 1;
    }
    
    bitmap_img img; 
    img.data = stbi_load(argv[1], (int*)&img.width, (int*)&img.height, &img.channels, 0);
    
    if (img.data == NULL) {
        printf("Erreur de chargement.\n");
        return 1;
    }
    if (img.height * img.width > (~((u32)0))) {
        printf("L'image contient trop de pixels.");
        return 1;
    }

    printf("Donner la tolérance de différence de couleur pour la création des formes,\n");
    do {
        printf("entre 1 000 et 1 000 000 000 (4000 recommandé): ");
        scanf("%f", &COL_THRESHOLD);
    }while (COL_THRESHOLD < 1000.f || COL_THRESHOLD > 1000000000.f);

    printf("\nDonner le ratio minimal nb_pixels_image/ nb_pixels_forme pour qu'une forme soit considérée comme trop petite, donc à fusionner,\n");
    do {
        printf("entre 10 et 1 000 000 (10 000 recommandé):");
        scanf("%f", &DEL_THRESHOLD);
    }while (DEL_THRESHOLD < 10.f || DEL_THRESHOLD > 1000000.f);

    printf("\nDonner distance maximal entre un segment (i,j) et un pixel sur le contour entre i et j,\n");
    do {
        printf("entre 0 et 100 (3 recommandé):");
        scanf("%f", &DISTANCE_THRESHOLD);
    }while (DISTANCE_THRESHOLD < 0.f || DISTANCE_THRESHOLD > 100.f);

    printf("\n");
    vec* shapes = get_shapes(&img);
    clock_t start = clock();
    transform_into_polygons(shapes, &img);
    clock_t end = clock();
    float ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;

    printf("Transformation en polygones: %fms\n", ms);


    // Affichage des pixels des formes, utilisé pour debugger
    // for (int i = 0; i < shapes->count; ++i) {
    //     shape* s = vec_get_element(shapes, i);
    //     printf("Color %d %d %d:\n", s->color.red, s->color.green, s->color.blue);
    //     printf("Pixel count: %d|\n", s->pixels->count);
    //     for (int k = 0; k < s->pixels->count; ++k) {
    //         Pixel* p = vec_get_element(s->pixels, k);
    //         printf("x:%d y:%d\n", *p % img.width, *p / img.width);
    //     }
    // }

    start = clock();

    create_svg(argv[2], &img, shapes);

    end = clock();
    ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("Ecriture de %s: %fms\n", argv[2], ms);

    free_vec(shapes);
    stbi_image_free(img.data);
    return 0;
}
