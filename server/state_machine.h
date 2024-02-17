/*
 * Modul pro stavy hrace a hry.
 */

#ifndef DOTSANDBOXES_STATE_MACHINE_H
#define DOTSANDBOXES_STATE_MACHINE_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "graph.h"

typedef enum e_game_state{
    ST_NOT_ALLOWED_GAME = 0,
    ST_INIT,
    ST_PLAYING_1,
    ST_PLAYING_2,
    ST_WIN,
} game_state;

typedef enum e_game_event {
    EV_CHOOSE_1,
    EV_CHOOSE_2,
    EV_PLAY_1_NO_SQUARES,
    EV_PLAY_2_NO_SQUARES,
    EV_PLAY_1_NEW_SQUARES,
    EV_PLAY_2_NEW_SQUARES,
    EV_PLAYGROUND_FULL,
} game_event;

typedef enum e_player_state {
    ST_NOT_ALLOWED_PLAYER = 0,
    ST_LOGGED,
    ST_QUEUED,
    ST_INGAME,
} player_state;

typedef enum e_player_event {
    EV_QUEUE,
    EV_START_GAME,
    EV_LEAVE_QUEUE,
    EV_LEAVE_GAME,
    EV_OPPONENT_LEFT,
} player_event;

int allowed_game_transition(game_state state, game_event event);
game_state get_game_transition(game_state state, game_event event);

int allowed_player_transition(player_state state, player_event event);
player_state get_player_transition(player_state state, player_event event);

#endif
