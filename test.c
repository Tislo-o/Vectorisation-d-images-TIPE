#include "vector.h"
#include "polygons.h"

int main() {
    vec* v = empty_vec(sizeof(Vertex), NULL);
    Vertex* ver = vec_push(v);
    (*ver) = (Vertex){0, 0};
    ver = vec_push(v);
    (*ver) = (Vertex){0, 1};
    ver = vec_push(v);
    (*ver) = (Vertex){1, 1};
    ver = vec_push(v);
    (*ver) = (Vertex){2, 1};
    ver = vec_push(v);
    (*ver) = (Vertex){2, 2};
    ver = vec_push(v);
    (*ver) = (Vertex){3, 2};
    ver = vec_push(v);
    (*ver) = (Vertex){3, 0};
    //      _ 
    //  _ _| |
    // |_ _ _|
    // (0,0) en bas à gauche
    graph* g = outline_to_graph(v);
    print_graph(g);
    vec* cycle = shortest_cycle(g);
    vec_print_data(cycle);
  

    free_vec(cycle);
    free_vec(v);
    free_graph(g);
    return 0;
}