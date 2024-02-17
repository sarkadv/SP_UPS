/*
 * Struktura pro graf propojenych bodu.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include "graph.h"

/*
 * Vytvori a vrati novou strukturu grafu.
 */
graph *create_graph(int vertex_count) {
    if (vertex_count <= 0) {
        return NULL;
    }

    int i;

    graph *graph_struct = (graph*)malloc(sizeof(graph));
    graph_struct->vertex_count = vertex_count;
    graph_struct->adj_matrix = (int**)calloc(vertex_count, sizeof(int*));

    for(i = 0; i < vertex_count; i++) {
        graph_struct->adj_matrix[i] = (int*)calloc(vertex_count, sizeof(int));
    }

    return graph_struct;
}

/*
 * Najde vsechny propojene vrcholy.
 * Vrati seznam, ve kterem se nachazi za sebou dvojice propojenych vrcholu.
 */
std::vector<int> find_all_connected_vertices(graph *graph) {
    std::vector<int> connected_vertices;
    int i, j;

    if (!graph) {
        return connected_vertices;
    }

    for (i = 0; i < graph->vertex_count; i++) {
        for (j = i + 1; j < graph->vertex_count; j++) {
            if (graph->adj_matrix[i][j]) {
                connected_vertices.push_back(i);
                connected_vertices.push_back(j);
            }
        }
    }

    return connected_vertices;
}

/*
 * Uvolni strukturu grafu.
 */
void free_graph(graph *graph) {
    int i;

    if (!graph) {
        return;
    }

    for(i = 0; i < graph->vertex_count; i++) {
        free(graph->adj_matrix[i]);
    }

    free(graph->adj_matrix);
    free(graph);
}

/*
 * Prida do grafu novou hranu.
 */
int add_edge(graph *graph, int x1, int x2) {
    if (!graph) {
        return 0;
    }

    if (x1 < 0 || x2 < 0) {
        return 0;
    }

    if (x1 > graph->vertex_count - 1 || x2 > graph->vertex_count - 1) {
        return 0;
    }

    graph->adj_matrix[x1][x2] = 1;
    graph->adj_matrix[x2][x1] = 1;
    return 1;
}

/*
 * Zjisti, zda jsou dva vrcholy propojene.
 */
int is_neighbor(graph *graph, int x1, int x2) {
    if (!graph) {
        return 0;
    }

    if (x1 < 0 || x2 < 0) {
        return 0;
    }

    if (x1 > graph->vertex_count - 1 || x2 > graph->vertex_count - 1) {
        return 0;
    }

    return graph->adj_matrix[x1][x2];
}

/*
 * Zjisti a ve vektoru vrati nove vytvorene ctverce.
 */
std::vector<int> find_new_cycle_vertices(graph *graph, int* current_vertices1, int* current_vertices2, int player1_score, int player2_score) {
    std::vector<int> result;
    std::vector<int> current_vertices1_copy;
    std::vector<int> current_vertices2_copy;
    int i;
    int vertex_count_sqrt;

    if (!graph) {
        return result;
    }

    vertex_count_sqrt = int(sqrt(graph->vertex_count));

    for (i = 0; i < player1_score; i++) {
        current_vertices1_copy.push_back(current_vertices1[i]);
    }

    for (i = 0; i < player2_score; i++) {
        current_vertices2_copy.push_back(current_vertices2[i]);
    }

    for (i = 0; i < graph->vertex_count - vertex_count_sqrt; i++){
        if ((i + 1) % vertex_count_sqrt == 0) {
            continue;
        }
        if (is_neighbor(graph, i, i + 1)) {
            if (is_neighbor(graph, i + 1, i + 1 + vertex_count_sqrt)) {
                if (is_neighbor(graph, i + 1 + vertex_count_sqrt, i + vertex_count_sqrt)) {
                    if (is_neighbor(graph, i + vertex_count_sqrt, i)) {
                        if (std::find(current_vertices1_copy.begin(), current_vertices1_copy.end(), i) == current_vertices1_copy.end()) {
                            if (std::find(current_vertices2_copy.begin(), current_vertices2_copy.end(), i) == current_vertices2_copy.end()) {
                                // ani jeden z vektoru neobsahuje i
                                result.push_back(i);
                            }
                        }
                    }
                }
            }
        }
    }

    return result;
}

/*
 * Zjisti, zda jsou vrcholy propojitelne, tedy zda jsou umistene vedle sebe.
 */
int are_vertices_adjacent(graph *graph, int x1, int x2) {
    int vertex_count_sqrt;

    if (!graph) {
        return 0;
    }

    vertex_count_sqrt = int(sqrt(graph->vertex_count));

    if (x1 < 0 || x2 < 0) {
        return 0;
    }

    if (x1 > graph->vertex_count - 1 || x2 > graph->vertex_count - 1) {
        return 0;
    }

    if (abs(x1 - x2) == 1) {    // vedle sebe nalevo nebo napravo
        return 1;
    }

    if (abs(x1 - x2) == vertex_count_sqrt) {    // vedle sebe nahore nebo dole
        return 1;
    }

    return 0;
}


