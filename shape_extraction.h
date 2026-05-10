#pragma once 
#include <time.h>
#include "global.h"
#include "r_string.h"
#include "vector.h"
#include "dense_set.h"


#define COL_THRESHOLD 4000.f //distance au carrée maximale entre 2 couleurs pour qu'elles soient considérées comme identiques
#define DEL_THRESHOLD 1000.f //ratio minimal (nb pixels de l'image / nb pixels d'une forme) pour que la forme soit considérée comme trop petite

#define NO_SHAPE 65535 //2^16 - 1 valeur maximale d'un u16


typedef enum{
    TOP = 0,
    RIGHT = 1,
    BOTTOM = 2,
    LEFT = 3,
    ENUM_END
}Straight_Dir;
static const s8_tuple straight_dir[] = { //décalages x/y pour passer de pixel à pixel ou de sommet à sommet
    {0,-1},{1,0},{0,1},{-1,0}
};

typedef enum{
    TOP_RIGHT = 0,
    BOTTOM_RIGHT = 1,
    BOTTOM_LEFT = 2,
    TOP_LEFT = 3,
    ERROR = 4
}Angled_Dir;
static const s8_tuple dir_VToP[] = { //décalages x/y pour passer de sommet à pixel
    {0,-1},{0,0},{-1,0},{-1,-1}
};
static const s8_tuple dir_PToV[] = { //décalages x/y pour passer de pixel à sommet
    {1,0},{1,1},{0,1},{0,0}
};
//convertit 2 directions "droites" en une direction en biais
Angled_Dir straights_to_angled[16] = {
    ERROR, TOP_RIGHT, ERROR, TOP_LEFT,       //(TOP, TOP) (TOP, RIGHT) (TOP, BOTTOM) (TOP, LEFT)
    TOP_RIGHT, ERROR, BOTTOM_RIGHT, ERROR,   //(RIGHT, TOP) (RIGHT, RIGHT) (RIGHT, BOTTOM) (RIGHT, LEFT)
    ERROR, BOTTOM_RIGHT, ERROR, BOTTOM_LEFT, //(BOTTOM, TOP) (BOTTOM, RIGHT) (BOTTOM, BOTTOM) (BOTTOM, LEFT)
    TOP_LEFT,  ERROR, BOTTOM_LEFT, ERROR,     //(LEFT, TOP) (LEFT, RIGHT) (LEFT, BOTTOM) (LEFT, LEFT)
};





//Si le pixel voisin du pixel src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le pixel correspondant
static inline bool PtoP_neighbour(bitmap_img* img, Pixel src, Pixel* dst, Straight_Dir d) {
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
static inline void PtoV_neighbour(bitmap_img* img, Pixel src, Vertex* dst, Angled_Dir d) {
    u16 w = img->width;
    u16 x = src % w;
    u16 y = src / w;
    int nx = x + dir_PToV[d].x;
    int ny = y + dir_PToV[d].y;
    assert(nx >= 0 && nx < w+1 && ny >= 0 && ny < img->height+1 && "Ce cas est impossible avec un pixel dans l'image");
    dst->x = nx;
    dst->y = ny;
}
//Si le pixel voisin du sommet src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le pixel correspondant
static inline bool VtoP_neighbour(bitmap_img* img, Vertex src, Pixel* dst, Angled_Dir d) {
    u16 w = img->width;
    int nx = src.x + dir_VToP[d].x;
    int ny = src.y + dir_VToP[d].y;
    if (nx < 0 || nx >= w || ny < 0 || ny >= img->height) return false;
    *dst = ny * w + nx;
    return true;
}
//Si le sommet voisin du sommet src est hors de l'image, retourne false
//Sinon retourne true et met dans *dst le sommet correspondant
static inline bool VtoV_neighbour(bitmap_img* img, Vertex src, Vertex* dst, Straight_Dir d) {
    int nx = src.x + straight_dir[d].x;
    int ny = src.y + straight_dir[d].y;
    if (nx < 0 || nx >= img->width+1 || ny < 0 || ny >= img->height+1) return false;
    dst->x = nx;
    dst->y = ny;
    return true;
}

//Renvoie la couleur du pixel p dans l'image bitmap img
static inline Color pixel_color(bitmap_img* img, Pixel px) {
    u64 idx = px * img->channels;
    return (Color){img->data[idx], img->data[idx + 1], img->data[idx + 2]};
}

//renvoie la distance au carré des couleurs c1 et c2 
static inline float color_distance(Color c1, Color c2) {
    float dr = (c1.red - c2.red);
    float dg = (c1.green - c2.green);
    float db = (c1.blue - c2.blue);
    float rb = (c1.red + c2.red)/2.f;

    return (2.f + rb/256.f)*dr*dr + 4.f*dg*dg + (2.f + (255.f - rb) / 256.f)*db*db;
}   

//ajoute une forme a la liste de formes shapes
//visited: tableau de n 'false'
void add_shape(bitmap_img* img, vec* shapes, dset* remaining, u16* shape_on_px) {
    assert(shapes->count <= 65535 && "Nombre de formes excède 65536!");

    shape* s = vec_push(shapes);
    s->ID = shapes->count - 1;
    s->pixels = empty_vec(sizeof(Pixel), NULL);
    s->outline = NULL;

     //----------Parcourir tous les pixels de la forme----------//
    Pixel starting_px = dset_get_min_element(remaining);    //pixel le plus en haut à gauche disponible
    Color c_first = pixel_color(img, starting_px);

    Pixel* stack = malloc(remaining->count * sizeof(Pixel)); //pile de pixels en attente d'être visité
    u32 stack_size = 1;
    stack[0] = starting_px;

    dset_remove_element(remaining, starting_px);
    vec_add(s->pixels, &starting_px);
    shape_on_px[starting_px] = s->ID;

    u32 s_nb_px = 1; //nombre de pixels de la forme
    u64 red = c_first.red, green = c_first.green, blue = c_first.blue;
    while (stack_size > 0) {
        Pixel px = stack[stack_size-1]; //on récupère le premier pixel de la pile
        stack_size--;

        for (Straight_Dir d = TOP; d < ENUM_END; ++d) {
            Pixel neighbour;
            if (PtoP_neighbour(img, px, &neighbour, d) && shape_on_px[neighbour] == NO_SHAPE) {
                Color c_neighbour = pixel_color(img, neighbour);

                if (color_distance(c_first, c_neighbour) <= COL_THRESHOLD) {
                    dset_remove_element(remaining, neighbour);
                    vec_add(s->pixels, &neighbour);
                    shape_on_px[neighbour] = s->ID;

                    stack_size++;
                    stack[stack_size-1] = neighbour;

                    red += c_neighbour.red;
                    green += c_neighbour.green;
                    blue += c_neighbour.blue;
                    s_nb_px ++;
                }
            }
        }    
    }
    free(stack);

    s->color = (Color){red / s_nb_px, green / s_nb_px, blue / s_nb_px}; 
} 

//renvoie un vec de toutes les formes composant l'image
vec* get_shapes(bitmap_img* img) {
    //---------Création des formes en parcourant l'image--------//
    clock_t start = clock();

    vec* shapes = empty_vec(sizeof(shape), shape_destroy); //liste des formes
    u32 n = img->height * img->width;           //nombre de pixels

    dset* remaining = full_dset(n);
    u16* shape_on_px = malloc(sizeof(u64) * n); //...[i] = ID de la forme au pixel i
    for (int i = 0; i < n; ++i) {
        shape_on_px[i] = NO_SHAPE;
    }
    while (remaining->count > 0) {
        add_shape(img, shapes, remaining, shape_on_px);
    }
    free_dset(remaining);

    clock_t end = clock();
    float ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("%d formes créées: %f ms\n", shapes->count, ms);

    //-------Fusion des petites formes avec d'autres adjacentes-------//
    start = clock();
    u16 total = shapes->count;

    u16* ID_to_idx = malloc(sizeof(u16) * shapes->count);
    for (int i = 0; i < total; ++i) {
        ID_to_idx[i] = i;
    }
    u16 next_slot = 0;  //case où placer la prochaine forme dans shape
    for (int i = 0; i < total; ++i) {
        shape* s = vec_get_element(shapes, i);

        if (s->pixels->count <= (n / DEL_THRESHOLD)) { //si s est trop petite

            //1---Trouver une forme voisine, la plus proche en couleur
            Pixel starting_px = *(Pixel*)vec_get_element(s->pixels, 0);
            Color s_color = s->color;
            u16 nei_ID = NO_SHAPE;
            float best_dist = 10e30;
            

             //On ajoute le sommet en haut à gauche
            Vertex starting_vertex;
            PtoV_neighbour(img, starting_px, &starting_vertex, TOP_LEFT);

            Straight_Dir d_mov = RIGHT; //direction dans le sens du déplacement
            Straight_Dir d_cen = BOTTOM; //direction vers le centre de la forme
            Straight_Dir tmp;
            Vertex cur;                  //le sommet courant
            PtoV_neighbour(img, starting_px, &cur, TOP_RIGHT);
            while (starting_vertex.x != cur.x || starting_vertex.y != cur.y) {

                Angled_Dir d1 = straights_to_angled[d_mov*4+d_cen];              //direction résultante de d_mov et d_cen
                Angled_Dir d2 = straights_to_angled[d_mov*4+((d_cen + 2) & 3)]; //direction résultante de d_mov et l'opposé de d_cen
                assert(d1 != ERROR && d2 != ERROR);
                Pixel p1, p2;      
                bool p1_in_image = VtoP_neighbour(img, cur, &p1, d1);
                bool p2_in_image = VtoP_neighbour(img, cur, &p2, d2);
                if (!p1_in_image || shape_on_px[p1] != s->ID) { //si p1 est hors de la forme
                    tmp = d_mov;
                    d_mov = d_cen;            //direction du mouvement est maintenant celle du centre
                    d_cen = (tmp + 2) & 3;    //direction du centre prend la direction opposée du mouvement 
                    if (p1_in_image) {
                        float dist = color_distance(pixel_color(img, p1), s_color);
                        if (dist < best_dist) {
                            best_dist = dist;
                            nei_ID = shape_on_px[p1];
                        }
                    }
                    
                } else if (p2_in_image && shape_on_px[p2] == s->ID) { //si p2 est dans la forme
                        tmp = d_cen;
                        d_cen = d_mov;            //direction du centre est maintenant celle du mouvement
                        d_mov = (tmp + 2) & 3;    //direction du mouvement prend la direction opposée du centre
                } else {
                    if (p2_in_image) {
                        float dist = color_distance(pixel_color(img, p2), s_color);
                        if (dist < best_dist) {
                            best_dist = dist;
                            nei_ID = shape_on_px[p2];
                        }
                    }  
                }
                VtoV_neighbour(img, cur, &cur, d_mov);
            }
            assert(nei_ID != NO_SHAPE);
            assert(ID_to_idx[nei_ID] != NO_SHAPE);


            shape* s_nei = vec_get_element(shapes, ID_to_idx[nei_ID]);

        //     float t = s_nei->pixels->count / (float)(s_nei->pixels->count + s->pixels->count);
            
        //   if (t < 0.8f) printf("%.2f|", t);
        //     Color c1 = (Color){(u8)((float)s_nei->color.red * t + (float)s_color.red * (1.f - t)),
        //                             (u8)((float)s_nei->color.green * t + (float)s_color.green * (1.f - t)),
        //                             (u8)((float)s_nei->color.blue * t + (float)s_color.blue * (1.f - t))};

            s_nei->color = s_nei->pixels->count > s->pixels->count ? s_nei->color : s->color;

            //2---Changer la forme voisine et le tableau global shape_on_px
            Pixel p_first_nei = *(Pixel*)vec_get_element(s_nei->pixels, 0);
            u32 old_nei_px_count = s_nei->pixels->count;

            vec_concatenate(s_nei->pixels, s->pixels);

            if (starting_px < p_first_nei) {  //remettre le pixel les plus en haut à gauche en première position
                vec_swap(s_nei->pixels, 0, old_nei_px_count);
            }

            for (int idx = 0; idx < s->pixels->count; ++idx) {
                shape_on_px[*(Pixel*)vec_get_element(s->pixels, idx)] = nei_ID;
            }
            
            //3---Supprimer la forme trop petite
            shape_destroy(s);
            ID_to_idx[i] = NO_SHAPE;
        } else {
            memcpy(vec_get_element(shapes, next_slot), s, shapes->stride);
            ID_to_idx[i] = next_slot;
            next_slot++;
        }
    }
    shapes->count = next_slot;
    shapes->data = realloc(shapes->data, shapes->stride * shapes->count);
    free(ID_to_idx);

    end = clock();
    ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("Fusion des plus petites formes pour arriver à %d formes: %f ms\n", shapes->count, ms);

    //-------Création du contour pour chaque forme-------//
    start = clock();

    for (int i = 0; i < shapes->count; ++i) {
        shape* s = vec_get_element(shapes, i);
        s->outline = empty_vec(sizeof(Vertex), NULL);
        
        Pixel starting_px = *(Pixel*)vec_get_element(s->pixels, 0);

        //On ajoute le sommet en haut à gauche
        Vertex starting_vertex;
        PtoV_neighbour(img, starting_px, &starting_vertex, TOP_LEFT);
        vec_add(s->outline, &starting_vertex);

        Straight_Dir d_mov = RIGHT; //direction dans le sens du déplacement
        Straight_Dir d_cen = BOTTOM; //direction vers le centre de la forme
        Straight_Dir tmp;
        Straight_Dir prev_d_mov = RIGHT; //état précédent de d_mov, sert à ne pas ajouter de sommets inutiles sur une ligne droite
        Vertex cur;           
        PtoV_neighbour(img, starting_px, &cur, TOP_RIGHT);
        Vertex prev_cur = cur;
        while (starting_vertex.x != prev_cur.x || starting_vertex.y != prev_cur.y) {
            // printf("prev: %d cur: %d\n", prev_cur, cur);
            if (d_mov != prev_d_mov) vec_add(s->outline, &prev_cur); //si on a changé de direction, on ajoute le sommet précédent
            prev_d_mov = d_mov;

            Angled_Dir d1 = straights_to_angled[d_mov*4+d_cen];              //direction résultante de d_mov et d_cen
            Angled_Dir d2 = straights_to_angled[d_mov*4+((d_cen + 2) & 3)]; //direction résultante de d_mov et l'opposé de d_cen
            assert(d1 != ERROR && d2 != ERROR);
            Pixel p1, p2;      
            if (!VtoP_neighbour(img, cur, &p1, d1) || shape_on_px[p1] != s->ID) { //si p1 est hors de la forme
                if (VtoP_neighbour(img, cur, &p2, d2) && shape_on_px[p2] == s->ID) {//si p2 est dans la forme
                    tmp = d_cen;
                    d_cen = d_mov;          //direction du centre prend la direction du mouvement
                    d_mov = (tmp + 2) & 3;  //direction du mouvement prend l'opposée du centre
                } else {
                    tmp = d_mov;
                    d_mov = d_cen;            //direction du mouvement est maintenant celle du centre
                    d_cen = (tmp + 2) & 3;    //direction du centre prend la direction opposée du mouvement 
                }

            } else if (VtoP_neighbour(img, cur, &p2, d2) && shape_on_px[p2] == s->ID) { //si p2 est dans la forme
                    tmp = d_cen;
                    d_cen = d_mov;            //direction du centre est maintenant celle du mouvement
                    d_mov = (tmp + 2) & 3;    //direction du mouvement prend la direction opposée du centre
            }
            // if (prev_cur.x == cur.x && prev_cur.y == cur.y) {
            //     printf("ERROR %d %d|", cur.x, cur.y);
            //     fflush(stdout);
            //     assert(0);
            // }
            prev_cur = cur;
            VtoV_neighbour(img, cur, &cur, d_mov);
        }
        free_vec(s->pixels); //on n'a plus besoin du tableau des pixels à l'intérieur de la forme
        s->pixels = NULL;
    }
    free(shape_on_px);

    end = clock();
    ms = ((float)(end - start) / CLOCKS_PER_SEC) * 1000;
    printf("Création du contour de chaque forme: %fms\n", ms);

    return shapes;
}
