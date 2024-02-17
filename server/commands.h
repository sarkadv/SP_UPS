/*
 * Modul pro vykonavani prikazu podle prichozich klientskych zprav.
 */

#ifndef DOTSANDBOXES_COMMANDS_H
#define DOTSANDBOXES_COMMANDS_H

#include "game_structures.h"
#include "config.h"
#include <queue>

#define MESSAGE_LENGTH 450

// klient -> server
#define LOGIN "login"
#define QUEUE "queue"
#define LINE_COORDS "line_coords"
#define LEAVE_QUEUE "leave_queue"
#define LEAVE_GAME "leave_game"
#define PONG "pong"

// server -> klient
#define PING "ping"
#define LOGGED "logged"
#define DISCONNECTED_SUSPICIOUS_ACTIVITY "disconnected_suspicious_activity"
#define DISCONNECTED_INACTIVITY "disconnected_inactivity"
#define QUEUED "queued"
#define START_GAME "start_game"
#define PLAY "play"
#define UNKNOWN_COMMAND "unknown_command"
#define DRAW_COORDS "draw_coords"
#define INVALID_COORDS "invalid_coords"
#define DRAW_SQUARES "draw_squares"
#define WINNER "winner"
#define YOU_LEFT_GAME "you_left_game"
#define OPPONENT_LEFT_GAME "opponent_left_game"
#define OPPONENT_CONNECTION "opponent_connection"
#define LEFT_QUEUE "left_queue"
#define SERVER_SHUTDOWN "server_shutdown"
#define RESUME_GAME "resume_game"

int try_running_command(char *command, std::vector<char*> params, int client_socket);
void send_message(int socket, char *base, std::vector<char*> params);
player *find_player(int client_socket);
void remove_player(int client_socket);
void disconnect_suspicious_client(int client_socket);
int create_game(player *player1, player *player2, int32_t vertex_count);
void remove_unfinished_game(int game_index);
void remove_finished_game(int game_index);
void send_connection_state_to_opponent(int client_socket, config *config);

#endif //DOTSANDBOXES_COMMANDS_H
