/*
 * Modul programu pro struktury hry a hrace.
 */

#include <string.h>
#include <time.h>
#include "game_structures.h"
#include "config.h"

/*
 * Vytvori strukturu pro novou hru a vrati ji.
 */
game *new_game(player *player1, player *player2, int32_t vertex_count) {
    game *new_game = (game*)malloc(sizeof(game));
    new_game->state = ST_INIT;
    new_game->this_graph = create_graph(vertex_count);
    new_game->player1 = player1;
    new_game->player2 = player2;
    new_game->player1_finished_squares = (int *)calloc(vertex_count, sizeof(int));
    new_game->player2_finished_squares = (int *)calloc(vertex_count, sizeof(int));
    memset(new_game->player1_finished_squares, -1, vertex_count);
    memset(new_game->player2_finished_squares, -1, vertex_count);
    new_game->last_move = time(NULL);
    new_game->player1_score = 0;
    new_game->player2_score = 0;
    return new_game;
}

/*
 * Prida k vytvorenym ctvercum prvniho hrace dalsi ctverce.
 */
void add_new_squares_player1(std::vector<int> new_squares, game *game) {
    int i;
    int j;
    int current_squares_count;

    if (!game) {
        return;
    }

    current_squares_count = game->player1_score;

    if (int(new_squares.size()) + current_squares_count > game->this_graph->vertex_count) {
        return;
    }

    j = 0;
    for (i = current_squares_count; i < current_squares_count + int(new_squares.size()); i++) {
        game->player1_finished_squares[i] = new_squares[j];
        j++;
    }
}

/*
 * Prida k vytvorenym ctvercum druheho hrace dalsi ctverce.
 */
void add_new_squares_player2(std::vector<int> new_squares, game *game) {
    int i;
    int j;
    int current_squares_count;

    if (!game) {
        return;
    }

    current_squares_count = game->player2_score;

    if (int(new_squares.size()) + current_squares_count > game->this_graph->vertex_count) {
        return;
    }

    j = 0;
    for (i = current_squares_count; i < current_squares_count + int(new_squares.size()); i++) {
        game->player2_finished_squares[i] = new_squares[j];
        j++;
    }
}

/*
 * Uvolni strukturu hrace.
 */
void free_player(player *freed) {
    if (!freed) {
        return;
    }

    free(freed);
}

/*
 * Uvolni strukturu hry.
 */
void free_game(game *freed) {
    if (!freed) {
        return;
    }

    if (freed->this_graph) {
        free_graph(freed->this_graph);
    }

    if (freed->player1_finished_squares) {
        free(freed->player1_finished_squares);
    }

    if (freed->player2_finished_squares) {
        free(freed->player2_finished_squares);
    }

    free(freed);
}

/*
 * Vytvori strukturu pro noveho hrace a vrati ji.
 */
player *new_player(int client_socket, char *name) {
    player *new_player = (player*)malloc(sizeof(player));
    new_player->state = ST_LOGGED;
    new_player->socket = client_socket;
    strcpy(new_player->name, name);
    new_player->this_game = NULL;
    new_player->missed_pings = 0;
    new_player->ponged = true;
    return new_player;
}
