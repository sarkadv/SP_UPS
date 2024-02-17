/*
 * Struktura pro graf propojenych bodu.
 */

#ifndef DOTSANDBOXES_GRAPH_H
#define DOTSANDBOXES_GRAPH_H

typedef struct {
    int vertex_count;
    int **adj_matrix;
} graph;

graph *create_graph(int vertex_count);
void free_graph(graph *graph);
int add_edge(graph *graph, int x1, int x2);
int is_neighbor(graph *graph, int x1, int x2);
std::vector<int> find_new_cycle_vertices(graph *graph, int* current_vertices1, int* current_vertices2, int player1_score, int player2_score);
int are_vertices_adjacent(graph *graph, int x1, int x2);
std::vector<int> find_all_connected_vertices(graph *graph);

#endif //DOTSANDBOXES_GRAPH_H
