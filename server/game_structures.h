/*
 * Modul programu pro struktury hry a hrace.
 */

#ifndef SP_GAME_STRUCTURES_H
#define SP_GAME_STRUCTURES_H

#include "state_machine.h"

#define NAME_LENGTH 20

struct player;
struct game;

struct player {
    player_state state;
    int socket;
    char name[NAME_LENGTH];
    game *this_game;
    bool ponged;
    int missed_pings;
};

struct game {
    game_state state;
    graph *this_graph;
    player *player1;
    player *player2;
    int *player1_finished_squares;
    int *player2_finished_squares;
    int player1_score;
    int player2_score;
    time_t last_move;
};

game *new_game(player *player1, player *player2, int32_t vertex_count);
void free_game(game *freed);
void add_new_squares_player1(std::vector<int> new_squares, game *game);
void add_new_squares_player2(std::vector<int> new_squares, game *game);

player *new_player(int client_socket, char *name);
void free_player(player *freed);

#endif //SP_GAME_STRUCTURES_H
