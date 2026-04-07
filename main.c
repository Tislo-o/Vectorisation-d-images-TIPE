#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#include <time.h>

#include "global.h"
#include "r_string.h"
#include "vector.h"
#include "dense_set.h"

//constantes
//4000
#define COL_THRESHOLD 4000.f //distance au carrée maximale entre 2 couleur pour qu'elles soient considérées comme identiques
//8000
#define DEL_TRESHOLD 8000.f //ratio minimal (nb pixels de l'image / nb pixels d'une forme) pour que la forme soit considérée comme trop petite
#define DEL_COL_TRESHOLD 8000.f //distance au carrée minimale entre la couleur d'une forme trop petite et celle de son environnement pour qu'elle soit considérée comme visible

typedef u32 Pixel;
typedef struct{
    u16 x;
    u16 y;
}Vertex; //sommet

typedef enum{
    TOP = 0,
    RIGHT = 1,
    BOTTOM = 2,
    LEFT = 3,
    ENUM_END
}Straight_Dir;
s8_tuple straight_dir[] = { //décalages x/y pour passer de pixel à pixel ou de sommet à sommet
    {0,-1},{1,0},{0,1},{-1,0}
};

typedef enum{
    TOP_RIGHT = 0,
    BOTTOM_RIGHT = 1,
    BOTTOM_LEFT = 2,
    TOP_LEFT = 3,
    ERROR = 4
}Angled_Dir;
s8_tuple dir_VToP[] = { //décalages x/y pour passer de sommet à pixel
    {0,-1},{0,0},{-1,0},{-1,-1}
};
s8_tuple dir_PToV[] = { //décalages x/y pour passer de pixel à sommet
    {1,0},{1,1},{0,1},{0,0}
};
//convertit 2 directions "droites" en une direction en biais
Angled_Dir straighs_to_angled[16] = {
    ERROR, TOP_RIGHT, ERROR, TOP_LEFT,       //(TOP, TOP) (TOP, RIGHT) (TOP, BOTTOM) (TOP, LEFT)
    TOP_RIGHT, ERROR, BOTTOM_RIGHT, ERROR,   //(RIGHT, TOP) (RIGHT, RIGHT) (RIGHT, BOTTOM) (RIGHT, LEFT)
    ERROR, BOTTOM_RIGHT, ERROR, BOTTOM_LEFT, //(BOTTOM, TOP) (BOTTOM, RIGHT) (BOTTOM, BOTTOM) (BOTTOM, LEFT)
    TOP_LEFT, ERROR, BOTTOM_LEFT, ERROR,     //(LEFT, TOP) (LEFT, RIGHT) (LEFT, BOTTOM) (LEFT, LEFT)
};



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
//une forme est une zone de l'image délimitée par son contour, une liste de sommets  et sa couleur
typedef struct{
    vec* contour; //tableau redimmensionable des sommets du contour
    Color color;
}shape;

void shape_destroy(void* p) {
    shape* n = p;
    free_vec(n->contour);
}
//Si le pixel voisin du pixel src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le pixel correspondant
bool PtoP_neighbour(bitmap_img* img, Pixel src, Pixel* dst, Straight_Dir d) {
    u16 w = img->width;
    u16 x = src % w;
    u16 y = src / w;
    int nx = x + straight_dir[d].x;
    int ny = y + straight_dir[d].y;
    if (nx < 0 || nx >= w || ny < 0 || ny >= img->height) return false;
    *dst = ny * w + nx;
    return true;
}
//Un sommet voisin d'un pixel est toujours dans l'image
//Met dans dst le sommet correspondant
void PtoV_neighbour(bitmap_img* img, Pixel src, Vertex* dst, Angled_Dir d) {
    u16 w = img->width;
    u16 x = src % w;
    u16 y = src / w;
    int nx = x + dir_PToV[d].x;
    int ny = y + dir_PToV[d].y;
    assert(nx >= 0 && nx < w+1 && ny >= 0 && ny < img->height+1 && "Je me suis gouré car ce cas est impossible normalement");
    dst->x = nx;
    dst->y = ny;
}
//Si le pixel voisin du sommet src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le pixel correspondant
bool VtoP_neighbour(bitmap_img* img, Vertex src, Pixel* dst, Angled_Dir d) {
    u16 w = img->width;
    int nx = src.x + dir_VToP[d].x;
    int ny = src.y + dir_VToP[d].y;
    if (nx < 0 || nx >= w || ny < 0 || ny >= img->height) return false;
    *dst = ny * w + nx;
    return true;
}
//Si le sommet voisin du sommet src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le sommet correspondant
bool VtoV_neighbour(bitmap_img* img, Vertex src, Vertex* dst, Straight_Dir d) {
    int nx = src.x + straight_dir[d].x;
    int ny = src.y + straight_dir[d].y;
    if (nx < 0 || nx >= img->width+1 || ny < 0 || ny >= img->height+1) return false;
    dst->x = nx;
    dst->y = ny;
    return true;
}

//Renvoie la couleur du pixel p dans l'image bitmap img
Color pixel_color(bitmap_img* img, Pixel px) {
    u64 idx = px * img->channels;
    return (Color){img->data[idx], img->data[idx + 1], img->data[idx + 2]};
}

//renvoie true si la distance au carré des couleurs c1 et c2 sont <= threshold
bool is_close_color(Color c1, Color c2, float threshold) {
    float dr = (c1.red - c2.red);
    float dg = (c1.green - c2.green);
    float db = (c1.blue - c2.blue);
    float rb = (c1.red + c2.red)/2.f;

    float dif = (2.f + rb/256.f)*dr*dr + 4.f*dg*dg + (2.f + (255.f - rb) / 256.f)*db*db;
    return dif < threshold;
}   

r_string* create_path(bitmap_img* img, vec* v) {
    u32 n_points = v->count;
    u16 w = img->width;

    Vertex* v0 = (Vertex*)vec_get_element(v, 0);
    float x0 = v0->x;
    float y0= v0->y;

    r_string* s = create_r_string("M");

    int i = 0; // indice courant dans la tableau de sommets
    concat_float(s, x0);
    concat_float(s, y0);
    while (i + 2 < n_points) {
        Vertex* vi = (Vertex*)vec_get_element(v, i);
        Vertex* vi1 = (Vertex*)vec_get_element(v, i+1);
        Vertex* vi2 = (Vertex*)vec_get_element(v, i+2);
        float xi = vi->x, yi = vi->y;
        float xi1 = vi1->x, yi1 = vi1->y; 
        float xi2 = vi2->x, yi2 = vi2->y; 

        concat_str(s, " Q");
        concat_float(s, 2.f * xi1 - (xi + xi2) / 2.f);
        concat_float(s, 2.f * yi1 - (yi + yi2) / 2.f);
        concat_float(s, xi2);
        concat_float(s, yi2);
        i += 2;
    }
    if (i + 2 == n_points) { //si le Pixel d'indice i + 1 n'est relié à personne
        concat_str(s, " Q");   //on referme la forme avec une courbe jusqu'au Pixel de départ
        Vertex* vi = (Vertex*)vec_get_element(v, i);
        Vertex* vi1 = (Vertex*)vec_get_element(v, i+1);
        float xi = vi->x, yi = vi->y;
        float xi1 = vi1->x, yi1 = vi1->y;

        concat_float(s, 2.f * xi1 - (xi + x0) / 2.f);
        concat_float(s, 2.f * yi1 - (yi + y0) / 2.f);
        concat_float(s, x0);
        concat_float(s, y0);
    } else { //tous les points ont été reliés
        concat_str(s, " Z"); //on referme la forme par une droite jusqu'au Pixel de départ
    }
    return s;
}
r_string* create_path2(bitmap_img* img, vec* v) {
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
        r_string* path = create_path2(img, s->contour);
        fprintf(f, "<path d=\"%s\" fill=\"rgb(%d, %d, %d)\"/>", path->data, s->color.red, s->color.green, s->color.blue);
        free_r_string(path);
    }
    fprintf(f, "</svg>");
    fclose(f);
}


//ajoute une forme a la liste de formes shapes
//visited: tableau de n 'false'
void add_shape(bitmap_img* img, vec* shapes, dset* remaining, bool* visited) {
    //----------Parcourir tous les pixels de la forme----------//
    Pixel starting_px = dset_get_min_element(remaining);    //pixel le plus en haut à gauche disponible
    Color c_first = pixel_color(img, starting_px);

    u32* stack = malloc(remaining->count * sizeof(u32)); //pile de pixels en attente d'être visité
    u32 stack_size = 1;
    stack[0] = starting_px;

    dset_remove_element(remaining, starting_px);
    visited[starting_px] = true;

    u32 s_nb_px = 1; //nombre de pixels de la forme
    u64 red = c_first.red, green = c_first.green, blue = c_first.blue;
    Pixel px_out = starting_px; //dernier pixel hors de la forme trouvé
    Pixel last_starting_px = starting_px; //pixel de départ du précédent agrandissement
    Pixel neighbour;
    bool neighbour_in_s;
    bool is_big_enough; //si la forme est assez grande
    bool s_finished = false;
    while (!s_finished) {
        u32 px = stack[stack_size-1]; //on récupère le premier pixel de la pile
        stack_size--;

        for (Straight_Dir d = TOP; d < ENUM_END; ++d) {
            neighbour_in_s = PtoP_neighbour(img, px, &neighbour, d);
            if (neighbour_in_s && !visited[neighbour]) {
                Color c_neighbour = pixel_color(img, neighbour);
                if (is_in_dset(remaining, neighbour)) {
                    if (is_close_color(c_first, c_neighbour, COL_THRESHOLD)) {
                        
                        dset_remove_element(remaining, neighbour);
                        stack_size++;
                        stack[stack_size-1] = neighbour;
                        visited[neighbour] = true;

                        red += c_neighbour.red;
                        green += c_neighbour.green;
                        blue += c_neighbour.blue;
                        s_nb_px ++;
                    }else{
                        px_out = neighbour;
                    }
                }
            }
        }
        if (stack_size == 0){
            is_big_enough = img->width * img->height / (float)s_nb_px <= DEL_TRESHOLD;
            if (!is_big_enough && px_out != last_starting_px){
                last_starting_px = px_out;
                stack_size++;
                stack[stack_size-1] = px_out;
                visited[px_out] = true;
            }else{
                s_finished = true;
            }
        }    
    }
    free(stack);

    Color c_shape =(Color){red / s_nb_px, green / s_nb_px, blue / s_nb_px}; //couleur de la forme
    //----------Construire le contour----------//
    //fonction(visited, c_shape, starting_px, shapes)
    //bool is_different_enough = !is_close_color(, , DEL_COL_TRESHOLD);//temp
    shape* s = vec_push(shapes);
    s->color = c_shape;
    s->contour = empty_vec(sizeof(Vertex), NULL);

    //On ajoute le sommet en haut à gauche
    Vertex starting_vertex;
    PtoV_neighbour(img, starting_px, &starting_vertex, TOP_LEFT);
    vec_add(s->contour, &starting_vertex);

    Straight_Dir d_mov = RIGHT; //direction dans le sens du déplacement
    Straight_Dir d_cen = BOTTOM; //direction vers le centre de la forme
    Straight_Dir tmp;
    Vertex cur;                  //le sommet courant
    PtoV_neighbour(img, starting_px, &cur, TOP_RIGHT);
    while (starting_vertex.x != cur.x || starting_vertex.y != cur.y) {
        vec_add(s->contour, &cur);
        
        Angled_Dir d1 = straighs_to_angled[d_mov*4+d_cen];              //direction résultante de d_mov et d_cen
        Angled_Dir d2 = straighs_to_angled[d_mov*4+((d_cen + 2) & 3)]; //direction résultante de d_mov et l'opposé de d_cen
        assert(d1 != ERROR && d2 != ERROR);
        Pixel p1;      
        Pixel p2;      
        if (!VtoP_neighbour(img, cur, &p1, d1) || !visited[p1]) { //si p1 est hors de la forme
            tmp = d_mov;
            d_mov = d_cen;            //direction du mouvement est maintenant celle du centre
            d_cen = (tmp + 2) & 3;    //direction du centre prend la direction opposée du mouvement 
        } else if (VtoP_neighbour(img, cur, &p2, d2) && visited[p2]) { //si p2 est dans la forme
                tmp = d_cen;
                d_cen = d_mov;            //direction du centre est maintenant celle du mouvement
                d_mov = (tmp + 2) & 3;    //direction du mouvement prend la direction opposée du centre
        }
        assert(VtoV_neighbour(img, cur, &cur, d_mov));
    }
} 

//renvoie un vec de toutes les formes composant l'image
vec* get_shapes(bitmap_img* img) {

    vec* shapes = empty_vec(sizeof(shape), shape_destroy); //liste des formes
    u32 n = img->height * img->width;
    dset* remaining = full_dset(n); //liste des indices des pixels disponibles

    bool* blank = malloc(n * sizeof(bool));  //tableau de n 'false'
    bool* in_shape_visited = malloc(n * sizeof(bool)); //pixels dans la forme mais déjà visités
    for (int i = 0; i < n; ++i) {
        blank[i] = false;
    }

    while (remaining->count > 0) {
        memcpy(in_shape_visited, blank, n * sizeof(bool)); //on réinitialise les pixels visités à 'aucuns'
        add_shape(img, shapes, remaining, in_shape_visited);
        // shape* s = vec_get_element(shapes, shapes->count - 1);
        // if () {
        //     vec_remove(s);
        // }
    }
    free_dset(remaining);
    free(blank);
    free(in_shape_visited);

    return shapes;
}

int main(int argc, char** argv) {
    if (argc < 3){
        printf("Donnez le nom de l'image en argument et le nom de l'image générée.\n");
        return 0;
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
    clock_t start = clock();

    vec* shapes = get_shapes(&img);

    clock_t end = clock();
    float ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("image crée en %fms\n", ms);
    printf("Nombre de formes : %d\n", shapes->count);


    // for (int i = 0; i < shapes->count; ++i) {
    //     shape* s = vec_get_element(shapes, i);
    //     printf("Color %d %d %d:\n", s->color.red, s->color.green, s->color.blue);

    //     for (int k = 0; k < s->contour->count; ++k) {
    //         Vertex* v = vec_get_element(s->contour, k);
    //         printf("x:%d y:%d\n", v->x, v->y);
    //     }
    // }
    start = clock();

    create_svg(argv[2], &img, shapes);

    end = clock();
    ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("Ecriture du fichier svg: %fms\n", ms);
    free_vec(shapes);
    stbi_image_free(img.data);
    return 0;
}
