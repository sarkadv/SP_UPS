/*
 * Modul pro stavy hrace a hry.
 */

#include "state_machine.h"

/*
 * Zjisti, zda je povolen prechod hry z aktualniho stavu s danou udalosti.
 */
int allowed_game_transition(game_state state, game_event event) {
    if (state == ST_INIT && event == EV_CHOOSE_1) {
        return 1;
    }
    else if (state == ST_INIT && event == EV_CHOOSE_2) {
        return 1;
    }
    else if (state == ST_PLAYING_1 && event == EV_PLAY_1_NO_SQUARES) {
        return 1;
    }
    else if (state == ST_PLAYING_2 && event == EV_PLAY_2_NO_SQUARES) {
        return 1;
    }
    else if (state == ST_PLAYING_1 && event == EV_PLAY_1_NEW_SQUARES) {
        return 1;
    }
    else if (state == ST_PLAYING_2 && event == EV_PLAY_2_NEW_SQUARES) {
        return 1;
    }
    else if (state == ST_PLAYING_1 && event == EV_PLAYGROUND_FULL) {
        return 1;
    }
    else if (state == ST_PLAYING_2 && event == EV_PLAYGROUND_FULL) {
        return 1;
    }
    else {
        return 0;
    }
}

/*
 * Vrati vysledny stav hry pri prechodu s aktualni udalosti.
 */
game_state get_game_transition(game_state state, game_event event) {
    if (state == ST_INIT && event == EV_CHOOSE_1) {
        return ST_PLAYING_1;
    }
    else if (state == ST_INIT && event == EV_CHOOSE_2) {
        return ST_PLAYING_2;
    }
    else if (state == ST_PLAYING_1 && event == EV_PLAY_1_NO_SQUARES) {
        return ST_PLAYING_2;
    }
    else if (state == ST_PLAYING_2 && event == EV_PLAY_2_NO_SQUARES) {
        return ST_PLAYING_1;
    }
    else if (state == ST_PLAYING_1 && event == EV_PLAY_1_NEW_SQUARES) {
        return ST_PLAYING_1;
    }
    else if (state == ST_PLAYING_2 && event == EV_PLAY_2_NEW_SQUARES) {
        return ST_PLAYING_2;
    }
    else if (state == ST_PLAYING_1 && event == EV_PLAYGROUND_FULL) {
        return ST_WIN;
    }
    else if (state == ST_PLAYING_2 && event == EV_PLAYGROUND_FULL) {
        return ST_WIN;
    }
    else {
        return ST_NOT_ALLOWED_GAME;
    }
}

/*
 * Zjisti, zda je povolen prechod hrace z aktualniho stavu s danou udalosti.
 */
int allowed_player_transition(player_state state, player_event event) {
    if (state == ST_LOGGED && event == EV_QUEUE) {
        return 1;
    }
    else if (state == ST_QUEUED && event == EV_START_GAME) {
        return 1;
    }
    else if (state == ST_QUEUED && event == EV_LEAVE_QUEUE) {
        return 1;
    }
    else if (state == ST_INGAME && event == EV_LEAVE_GAME) {
        return 1;
    }
    else if (state == ST_INGAME && event == EV_OPPONENT_LEFT) {
        return 1;
    }
    else {
        return 0;
    }
}

/*
 * Vrati vysledny stav hrace pri prechodu s aktualni udalosti.
 */
player_state get_player_transition(player_state state, player_event event) {
    if (state == ST_LOGGED && event == EV_QUEUE) {
        return ST_QUEUED;
    }
    else if (state == ST_QUEUED && event == EV_START_GAME) {
        return ST_INGAME;
    }
    else if (state == ST_QUEUED && event == EV_LEAVE_QUEUE) {
        return ST_LOGGED;
    }
    else if (state == ST_INGAME && event == EV_LEAVE_GAME) {
        return ST_LOGGED;
    }
    else if (state == ST_INGAME && event == EV_OPPONENT_LEFT) {
        return ST_QUEUED;
    }
    else {
        return ST_NOT_ALLOWED_PLAYER;
    }
}
