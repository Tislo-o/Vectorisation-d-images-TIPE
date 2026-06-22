#pragma once
#include "global.h"
#include "vector.h"
#include "queue.h"
#include <stdbool.h>
#include <math.h>

#define MAX_U32 0b11111111111111111111111111111111ULL

typedef struct {
    float dif; //poids du lien
    u32 v_idx; //indice du sommet d'arrivée
}Link;

typedef struct {
    vec* links;
}Node;
void Node_destroy(void* p) {
    Node* n = p;
    free_vec(n->links);
}
typedef struct {
    vec* nodes;
}graph;
//return a graph with n isolated nodes numbered from 0 to n-1
graph* basic_graph(int n) {
    graph* g = malloc(sizeof(graph));
    g->nodes = empty_vec(sizeof(Node), Node_destroy);
    for (int i = 0; i < n; ++i) {
        Node* ni = (Node*)vec_push(g->nodes);
        ni->links = empty_vec(sizeof(Link),NULL);
    }
    return g;
}
void free_graph(graph* g) {
    free_vec(g->nodes);
    free(g);
}

void print_graph(graph* g) {
    int n = g->nodes->count;
    for (int i = 0; i < n; ++i) {
        Node* ni = vec_get_element(g->nodes, i);
        printf("%d:", i);
        int ki = ni->links->count;
        for (int j = 0; j < ki; ++j) {
            printf("%d,", ((Link*)vec_get_element(ni->links, j))->v_idx);
        }
        printf("\n");
    }
}

//Precondition: i < j  0 <= i <= n-1   1 <= j <= n + (n-2)
//retourne si le segment i j approxime suffisament bien les sommets entre i+1 et j-1
//si oui retourne dans dif la différence moyenne entre le segment et les points
bool is_straight(vec* outline, int i, int j, int n, float* dif) {
    Vertex pi = *(Vertex*)vec_get_element(outline, i);
    Vertex pj = *(Vertex*)vec_get_element(outline, j % n);
    
    float d_ij = sqrt((pi.x - pj.x)*(pi.x - pj.x) + (pi.y - pj.y)*(pi.y - pj.y));
    assert(d_ij != 0.f);
    float sum = 0.f;
    for (int k = i + 1; k < j; ++k) {
        Vertex pk = *(Vertex*)vec_get_element(outline, k % n);
        float d = ((pi.y - pj.y)*pk.x - (pi.x - pj.x)*pk.y + pi.x*pj.y - pi.y*pj.x) / d_ij;
        if (d > DISTANCE_THRESHOLD || d < 0) { //si le pixel intermédiaire est trop loin du segment
            return false;                        //ou qu'il est à l'exterieur du segment (donc que le segment 
                                                 //passe par l'interieur de la forme)
        }
        sum += d;
    }
    if (i+1 != j) sum /= j - i - 1; //diviser par le nombre de points intermédiaires
    *dif = sum;
    return true;
}

graph* outline_to_graph(vec* outline) {
    int n = outline->count;
    graph* g = basic_graph(n);

    for (int i = 0; i < n; ++i) {
        Node* ni = vec_get_element(g->nodes, i);

        for (int j = i+1; j < n+i; ++j) {
            float dif;
            if (is_straight(outline, i, j, n, &dif)) {
                Link* l = vec_push(ni->links);
                l->dif = dif;
                l->v_idx = j % n;
            }
        }
    }
    return g;
}

vec* shortest_cycle(graph* g) {
    u32 n = g->nodes->count;
    u32 bytes_needed = n * sizeof(u32);

    vec* best_cycle = empty_vec(sizeof(u32), NULL); //best cycle found
    u32 best_length = MAX_U32; //best cycle length found
    float best_total_dif = INFINITY;//best total dif (sum of the dif of each link) found

    u32* initial_dist = malloc(bytes_needed);
    u32* dist = malloc(bytes_needed); //will hold the distance from i to each n_idx in the graph
    for (int i = 0; i < n; ++i) {
        initial_dist[i] = MAX_U32;
    }
    u32* parents = malloc(sizeof(u32) * n);//to reconstruct the cycle once found, 
    float* difs = malloc(sizeof(float) * n); //for each node i, dif of the link between i and parents[i]

    for (u32 s = 0; s < n; ++s) { //for each posssible starting vertex
        //printf("Starting: %d\n", s);
        memcpy(dist, initial_dist, bytes_needed);//initialize the dist array
        dist[s] = 0;

        queue* q = empty_queue();
        queue_element(q, s);

        while (!is_queue_empty(q)) {
            u32 n_idx = dequeue_element(q);
            //printf("dequeued: %d ", n_idx);
            Node* node = vec_get_element(g->nodes, n_idx);
            //printf("links count: %d\n",node->links->count);

            assert(node->links->count > 0);
            for (int j = node->links->count-1; j >= 0; --j) {
                Link* l = (Link*)vec_get_element(node->links, j);
                u32 child_idx = l->v_idx;
                //printf("child: %d", child_idx);

                if (child_idx == s) {   //if a cycle is found
                    //printf("starting from %d length: %d\n", s, dist[n_idx] + 1);
                    
                    u32 cycle_length = dist[n_idx] + 1;
                    if (cycle_length < 3) {
                        printf("Warning: cycle of length %d found\n",cycle_length);
                        continue;
                    }
                    if (cycle_length > best_length) continue;

                    vec* new_cycle = empty_vec(sizeof(u32), NULL);
                    vec_add(new_cycle, &s);
                    float total_dif = l->dif;

                    u32 cur = n_idx;
                    while (cur != s) {
                        //printf("%d,", cur);
                        vec_add(new_cycle, &cur);
                        total_dif += difs[cur];
                        cur = parents[cur];
                    }
                    vec* worst_cycle = new_cycle;
                    if (cycle_length < best_length || total_dif < best_total_dif) { //if it is a better cycle
                        //printf("BETTER CYCLE:");
                        if (cycle_length == best_length && total_dif < best_total_dif) {
       //                     printf("OPTIMIZED   ");
                        }
                        worst_cycle = best_cycle;
                        best_cycle = new_cycle;
                        best_length = cycle_length;
                        best_total_dif = total_dif;
                    }
                    free_vec(worst_cycle);
                } 
                else if (dist[child_idx] == MAX_U32) { //if the child is visited for the first time
                    //printf("visited for the first time");
                    dist[child_idx] = dist[n_idx] + 1;
                    parents[child_idx] = n_idx;
                    difs[child_idx] = l->dif;

                    if (dist[child_idx] < best_length) {
                        queue_element(q, child_idx);
                        //printf(" and queued");
                    }
                } 
                //printf("\n");
            }
        }
        free_queue(q);
    }
    free(initial_dist);
    free(dist);
    free(parents);
    free(difs);
    assert(best_length != MAX_U32 && "there was no cycle in the graph!");
    assert(best_length != 0);
    assert(best_length != 1);
    assert(best_length != 2);
    return best_cycle;
}
