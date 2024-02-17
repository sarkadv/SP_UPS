/*
 * Modul pro vykonavani prikazu podle prichozich klientskych zprav.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <csignal>
#include <sys/socket.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include "parser.h"
#include "commands.h"
#include "state_machine.h"
#include "server.h"
#include "graph.h"
#include "logger.h"

/*
 * Vytvori a vrati zpravu podle zakladniho prikazu base a parametru.
 */
char* create_message(char *base, std::vector<char*> params) {
    char *message;
    int i;

    message = (char*)calloc(MESSAGE_LENGTH, sizeof(char));

    strcpy(message, base);

    for (i = 0; i < int(params.size()); i++) {
        strcat(message, "|");
        strcat(message, params[i]);
    }

    strcat(message, "\n");

    return message;
}

/*
 * Posle zpravu na urceny socket.
 */
void send_message(int socket, char *base, std::vector<char*> params) {
    char message_buffer[MESSAGE_LENGTH];
    char *created_message;
    char *line;
    player *player;
    signal(SIGPIPE, SIG_IGN);
    ssize_t rc;

    if (!base) {
        return;
    }

    player = find_player(socket);

    memset(message_buffer, 0, sizeof(message_buffer));
    line = (char*)calloc(LINE_LENGTH, sizeof(char));

    created_message = create_message(base, params);
    strncpy(message_buffer, created_message, sizeof(message_buffer));

    rc = send(socket, message_buffer, sizeof(message_buffer), 0);

    if (rc == -1) { // nepodarilo se odeslat zpravu
        if (errno == EPIPE) {
            // broken pipe
        }
    }
    else {
        if (player != NULL) {
            snprintf(line, LINE_LENGTH, "Sending message to socket %d (player %s): %s", socket, player->name, created_message);
        }
        else {
            snprintf(line, LINE_LENGTH, "Sending message to socket %d: %s", socket, created_message);
        }
        printf("%s", line);
        log_line(global::log, line);
    }

    free(created_message);
    free(line);
}

/*
 * Najde a vrati prvniho hrace s timto socketem v seznamu hracu.
 */
player *find_player(int client_socket) {
    player *found = NULL;
    int i;

    for (i = 0; i < int(global::players.size()); i++) {
        if (global::players[i]->socket == client_socket) {
            return global::players[i];
        }
    }

    return found;
}

/*
 * Najde a vrati prvniho hrace s timto jmenem v seznamu hracu.
 */
player *find_player_by_name(char *name) {
    player *found = NULL;
    int i;

    for (i = 0; i < int(global::players.size()); i++) {
        if (!strcmp(global::players[i]->name, name)) {
            return global::players[i];
        }
    }

    return found;
}

/*
 * Najde a vrati index hrace v seznamu vsech hracu.
 */
int find_player_index(int client_socket) {
    int found = -1;
    int i;

    for (i = 0; i < int(global::players.size()); i++) {
        if (global::players[i]->socket == client_socket) {
            return i;
        }
    }

    return found;
}

/*
 * Najde a vrati index hrace v seznamu vsech hracu.
 */
int find_player_queue_index(int client_socket) {
    int found = -1;
    int i;

    for (i = 0; i < int(global::player_queue.size()); i++) {
        if (global::player_queue[i]->socket == client_socket) {
            return i;
        }
    }

    return found;
}

/*
 * Zaradi hrace do fronty, pokud to jeho stav dovoluje.
 */
int command_queue(int client_socket) {
    std::vector<char*> message_params;
    player *player;

    player = find_player(client_socket);

    if (!player) {
        message_params = {(char*)"error"};
        send_message(client_socket, (char*)QUEUED, message_params);
        return 0;
    }

    if (allowed_player_transition(player->state, EV_QUEUE)) {
        player->state = get_player_transition(player->state, EV_QUEUE);

        global::player_queue.push_back(player);

        printf("QUEUED: %s\n", player->name);

        message_params = {(char*)"ok"};
        send_message(player->socket, (char*)QUEUED, message_params);

        return 1;

    }
    else if (allowed_player_transition(player->state, EV_OPPONENT_LEFT)) {
        player->state = get_player_transition(player->state, EV_OPPONENT_LEFT);

        if (player->this_game == NULL) {
            global::player_queue.push_back(player);

            printf("QUEUED: %s\n", player->name);

            message_params = {(char*)"ok"};
            send_message(player->socket, (char*)QUEUED, message_params);

            return 1;
        }
        else if (!player->this_game->player1 || !player->this_game->player2) {
            global::player_queue.push_back(player);

            printf("QUEUED: %s\n", player->name);

            message_params = {(char*)"ok"};
            send_message(player->socket, (char*)QUEUED, message_params);

            return 1;
        }
    }

    message_params = {(char*)"error"};
    send_message(player->socket, (char*)QUEUED, message_params);

    return 0;
}

/*
 * Vytvori strukturu noveho hrace a zaradi ji do seznamu vsech hracu.
 */
int create_player(int client_socket, char *name) {
    player *created_player;
    std::vector<char*> message_params;

    created_player = new_player(client_socket, name);

    if (!created_player) {
        return 0;
    }

    global::players.push_back(created_player);

    return 1;
}

/*
 * Odstrani strukturu hrace z fronty, pokud se v ni nachazi.
 */
void remove_player_from_queue(int client_socket) {
    player *player;
    int queue_index = find_player_queue_index(client_socket);
    char *line;

    player = find_player(client_socket);

    if (!player) {
        return;
    }

    line = (char*)calloc(LINE_LENGTH, sizeof(char));

    if (queue_index != -1) {
        global::player_queue.erase(global::player_queue.begin() + queue_index);

        snprintf(line, LINE_LENGTH, "Player %s on socket %d was removed from the queue.\n", player->name, player->socket);
        printf("%s", line);
        log_line(global::log, line);
    }

    free(line);
}

/*
 * Odstrani strukturu hrace ze seznamu vsech hracu, pripadne i z fronty.
 */
void remove_player(int client_socket) {
    player *player;
    int player_index = find_player_index(client_socket);
    char *line;

    if (player_index == -1) {
        return;
    }

    line = (char*)calloc(LINE_LENGTH, sizeof(char));

    player = global::players[player_index];

    if (player->this_game != NULL) {
        if (player == player->this_game->player1) {
            player->this_game->player1 = NULL;
        }
        else if (player == player->this_game->player2) {
            player->this_game->player2 = NULL;
        }
    }

    snprintf(line, LINE_LENGTH, "Player %s on socket %d was removed.\n", player->name, player->socket);
    printf("%s", line);
    log_line(global::log, line);

    remove_player_from_queue(client_socket);
    global::players.erase(global::players.begin() + player_index);

    free_player(player);
    free(line);
}

/*
 * Odpoji klienta, ktery poslal nevalidni zpravu.
 */
void disconnect_suspicious_client(int client_socket) {
    std::vector<char*> message_params;

    send_message(client_socket, (char*)DISCONNECTED_SUSPICIOUS_ACTIVITY, message_params);
    close(client_socket);
    remove_player(client_socket);
}

/*
 * Posle znovu pripojenemu klientovi informace potrebne k pokracovani hry.
 */
int resume_game(int client_socket) {
    player *me_player;
    player *opponent;
    std::vector<char*> message_params;
    char player_number[2];
    char active_player[2];
    char my_score[3];
    char opponent_score[3];
    char opponent_name[NAME_LENGTH];
    char **my_square_coordinates;
    char **opponent_square_coordinates;
    char **line_coordinates;
    std::vector<int> connected_vertices;
    int i;

    me_player = find_player(client_socket);

    if (!me_player) {
        message_params = {(char *const)"error"};
        send_message(client_socket, (char *)RESUME_GAME, message_params);
        return 0;
    }

    if (me_player == me_player->this_game->player1) {
        opponent = find_player(me_player->this_game->player2->socket);
    }
    else if (me_player == me_player->this_game->player2) {
        opponent = find_player(me_player->this_game->player1->socket);
    }
    else {
        message_params = {(char *const)"error"};
        send_message(client_socket, (char *)RESUME_GAME, message_params);
        return 0;
    }

    if (!opponent) {
        message_params = {(char *const)"error"};
        send_message(client_socket, (char *)RESUME_GAME, message_params);
        return 0;
    }

    if (me_player->state == ST_INGAME && me_player->this_game != NULL) {
        connected_vertices = find_all_connected_vertices(me_player->this_game->this_graph);
        line_coordinates = (char**)calloc(connected_vertices.size(), sizeof(char*));

        if (me_player == me_player->this_game->player1) {
            my_square_coordinates = (char**)calloc(me_player->this_game->player1_score, sizeof(char*));
            opponent_square_coordinates = (char**)calloc(me_player->this_game->player2_score, sizeof(char*));

            snprintf(player_number, 2, "%d", 1);
            message_params = {(char*)player_number};

            strncpy(opponent_name, opponent->name, NAME_LENGTH);
            message_params.push_back(opponent_name);

            if (me_player->this_game->state == ST_PLAYING_1) {
                snprintf(active_player, 2, "%d", 1);
                message_params.push_back(active_player);
            }
            else if (me_player->this_game->state == ST_PLAYING_2) {
                snprintf(active_player, 2, "%d", 2);
                message_params.push_back(active_player);
            }
            else {
                message_params = {(char *const)"error"};
                send_message(client_socket, (char *)RESUME_GAME, message_params);
                return 0;
            }

            snprintf(my_score, 3, "%d", me_player->this_game->player1_score);
            message_params.push_back(my_score);
            snprintf(opponent_score, 3, "%d", me_player->this_game->player2_score);
            message_params.push_back(opponent_score);

            // moje ctverce
            for (i = 0; i < me_player->this_game->player1_score; i++) {
                my_square_coordinates[i] = (char*)calloc(5, sizeof(char));
                snprintf(my_square_coordinates[i], 5, "%d", me_player->this_game->player1_finished_squares[i]);
                message_params.push_back(my_square_coordinates[i]);
            }

            // oponentovy ctverce
            for (i = 0; i < me_player->this_game->player2_score; i++) {
                opponent_square_coordinates[i] = (char*)calloc(5, sizeof(char));
                snprintf(opponent_square_coordinates[i], 5, "%d", me_player->this_game->player2_finished_squares[i]);
                message_params.push_back(opponent_square_coordinates[i]);
            }

            // propojene vrcholy
            for (i = 0; i < int(connected_vertices.size()); i++) {
                line_coordinates[i] = (char*)calloc(5, sizeof(char));
                snprintf(line_coordinates[i], 5, "%d", connected_vertices[i]);
                message_params.push_back(line_coordinates[i]);
            }

            send_message(client_socket, (char *)RESUME_GAME, message_params);

            for (i = 0; i < me_player->this_game->player1_score; i++) {
                free(my_square_coordinates[i]);
            }
            free(my_square_coordinates);

            for (i = 0; i < me_player->this_game->player2_score; i++) {
                free(opponent_square_coordinates[i]);
            }
            free(opponent_square_coordinates);
        }
        else if (me_player == me_player->this_game->player2) {
            my_square_coordinates = (char**)calloc(me_player->this_game->player2_score, sizeof(char*));
            opponent_square_coordinates = (char**)calloc(me_player->this_game->player1_score, sizeof(char*));

            snprintf(player_number, 2, "%d", 2);
            message_params = {(char*)player_number};

            strncpy(opponent_name, opponent->name, NAME_LENGTH);
            message_params.push_back(opponent_name);

            if (me_player->this_game->state == ST_PLAYING_1) {
                snprintf(active_player, 2, "%d", 1);
                message_params.push_back(active_player);
            }
            else if (me_player->this_game->state == ST_PLAYING_2) {
                snprintf(active_player, 2, "%d", 2);
                message_params.push_back(active_player);
            }
            else {
                message_params = {(char *const)"error"};
                send_message(client_socket, (char *)RESUME_GAME, message_params);
                return 0;
            }

            snprintf(my_score, 3, "%d", me_player->this_game->player2_score);
            message_params.push_back(my_score);
            snprintf(opponent_score, 3, "%d", me_player->this_game->player1_score);
            message_params.push_back(opponent_score);

            // moje ctverce
            for (i = 0; i < me_player->this_game->player2_score; i++) {
                my_square_coordinates[i] = (char*)calloc(5, sizeof(char));
                snprintf(my_square_coordinates[i], 5, "%d", me_player->this_game->player2_finished_squares[i]);
                message_params.push_back(my_square_coordinates[i]);
            }

            // oponentovy ctverce
            for (i = 0; i < me_player->this_game->player1_score; i++) {
                opponent_square_coordinates[i] = (char*)calloc(5, sizeof(char));
                snprintf(opponent_square_coordinates[i], 5, "%d", me_player->this_game->player1_finished_squares[i]);
                message_params.push_back(opponent_square_coordinates[i]);
            }

            // propojene vrcholy
            for (i = 0; i < int(connected_vertices.size()); i++) {
                line_coordinates[i] = (char*)calloc(5, sizeof(char));
                snprintf(line_coordinates[i], 5, "%d", connected_vertices[i]);
                message_params.push_back(line_coordinates[i]);
            }

            send_message(client_socket, (char *)RESUME_GAME, message_params);

            for (i = 0; i < me_player->this_game->player2_score; i++) {
                free(my_square_coordinates[i]);
            }
            free(my_square_coordinates);

            for (i = 0; i < me_player->this_game->player1_score; i++) {
                free(opponent_square_coordinates[i]);
            }
            free(opponent_square_coordinates);
        }
        else {
            message_params = {(char *const)"error"};
            send_message(client_socket, (char *)RESUME_GAME, message_params);
            return 0;
        }

        for (i = 0; i < int(connected_vertices.size()); i++) {
            free(line_coordinates[i]);
        }
        free(line_coordinates);
        return 1;
    }
    else {
        message_params = {(char *const)"error"};
        send_message(client_socket, (char *)RESUME_GAME, message_params);
        return 0;
    }
}

/*
 * Rozpozna, zda jde o vracejiciho se hrace ci noveho.
 * Vrati vracejiciho hrace zpet do hry, pokud v ni byl.
 * Uplne novy hrac je pouze pridan do seznamu hracu.
 */
int command_login(char *name, int client_socket) {
    player *existing_player;
    std::vector<char*> message_params;
    char *trimmed_name;
    player *existing_socket;    // aby se nemohl logovat hrac znovu pod stejnym socketem

    if (!name) {
        return 0;
    }

    existing_socket = find_player(client_socket);

    if (existing_socket) {
        message_params = {(char *const)"error"};
        send_message(client_socket, (char *)LOGGED, message_params);
        return 0;
    }

    if (strlen(name) > NAME_LENGTH) {
        message_params = {(char *const)"error"};
        send_message(client_socket, (char *)LOGGED, message_params);
        return 0;
    }

    trimmed_name = (char*)calloc(NAME_LENGTH, sizeof(char));
    remove_spaces(trimmed_name, name);

    if (strlen(trimmed_name) == 0) {
        message_params = {(char *const)"error"};
        send_message(client_socket, (char *)LOGGED, message_params);
        return 0;
    }

    existing_player = find_player_by_name(name);

    if (existing_player) {     // novy hrac je vracejici se hrac
        // novy socket
        existing_player->socket = client_socket;
        existing_player->ponged = true;
        existing_player->missed_pings = 0;

        // poslat zpravy o stavu hry
        if (existing_player->state == ST_INGAME && (existing_player->this_game->state == ST_PLAYING_1 || existing_player->this_game->state == ST_PLAYING_2)) {
            message_params = {(char*)"ok"};
            send_message(existing_player->socket, (char*)LOGGED, message_params);
            if (!resume_game(client_socket)) {
                existing_player->state = ST_LOGGED;
                return 1;
            }
            return 1;
        }
        else {
            existing_player->state = ST_LOGGED;
            remove_player_from_queue(existing_player->socket);
            message_params = {(char*)"ok"};
            send_message(existing_player->socket, (char*)LOGGED, message_params);
            return 1;
        }
    }
    else {      // uplne novy hrac
        create_player(client_socket, name);
        message_params = {(char*)"ok"};
        send_message(client_socket, (char*)LOGGED, message_params);
        return 1;
    }

    return 0;
}

/*
 * Overi validitu zadanych spojovanych souradnic.
 * Cisla musi byt v urcitem rozsahu a tlacitka vedle sebe.
 */
int check_line_coords(int x1, int x2, graph *graph) {
    if (!graph) {
        return 0;
    }

    if (x1 < 0 || x2 < 0) {
        return 0;
    }

    if (x1 > graph->vertex_count - 1 || x2 > graph->vertex_count - 1) {
        return 0;
    }

    if (!are_vertices_adjacent(graph, x1, x2)) {    // nejsou sousedni vrcholy
        return 0;
    }

    if (is_neighbor(graph, x1, x2)) {   // uz jsou propojene
        return 0;
    }

    return 1;
}

/*
 * Zvoli viteze dokoncene hry.
 */
void pick_winner(game *game) {
    int player1_score;
    int player2_score;
    char winner_score_string[5];
    std::vector<char*> message_params;

    if (!game) {
        return;
    }

    memset(winner_score_string, 0, sizeof(winner_score_string));

    if (game->state == ST_PLAYING_1 || game->state == ST_PLAYING_2) {
        if (allowed_game_transition(game->state, EV_PLAYGROUND_FULL)) {
            game->state = get_game_transition(game->state, EV_PLAYGROUND_FULL);

            player1_score = game->player1_score;
            player2_score = game->player2_score;

            if (player1_score > player2_score) {    // vyhral hrac 1
                snprintf(winner_score_string, sizeof(winner_score_string), "%d", player1_score);
                message_params = {(char *const)"1", (char *const)winner_score_string};
                send_message(game->player1->socket, (char*)WINNER, message_params);
                send_message(game->player2->socket, (char*)WINNER, message_params);
            }
            else if (player1_score < player2_score) {   // vyhral hrac 2
                snprintf(winner_score_string, sizeof(winner_score_string), "%d", player2_score);
                message_params = {(char *const)"2", (char *const)winner_score_string};
                send_message(game->player1->socket, (char*)WINNER, message_params);
                send_message(game->player2->socket, (char*)WINNER, message_params);
            }
            else {  // remiza
                message_params = {(char *const)"0", (char *const)"0"};
                send_message(game->player1->socket, (char*)WINNER, message_params);
                send_message(game->player2->socket, (char*)WINNER, message_params);
            }
        }
    }
}

/*
 * Zjisti, zda uz je cela herni plocha zaplnena.
 * Vrati 1 - ano, 0 - ne.
 */
int is_full(graph *graph, int player1_score, int player2_score) {
    int vertex_count_sqrt;

    if (!graph) {
        return 0;
    }

    if (player1_score < 0 || player2_score < 0) {
        return 0;
    }

    vertex_count_sqrt = int(sqrt(graph->vertex_count));

    if (player1_score + player2_score == (vertex_count_sqrt - 1) * (vertex_count_sqrt - 1)) {
        return 1;
    }

    return 0;
}

/*
 * Posle klientovi pokyn k vykresleni usecky.
 */
void draw_coords(int x1, int x2, game *game) {
    char coordinate1[5];
    char coordinate2[5];
    std::vector<char*> message_params;

    if (!game) {
        return;
    }

    memset(coordinate1, 0, sizeof(coordinate1));
    memset(coordinate2, 0, sizeof(coordinate2));

    snprintf(coordinate1, sizeof(coordinate1), "%d", x1);
    snprintf(coordinate2, sizeof(coordinate2), "%d", x2);

    message_params = {(char *const)coordinate1, (char *const)coordinate2};
    send_message(game->player1->socket, (char*)DRAW_COORDS, message_params);
    send_message(game->player2->socket, (char*)DRAW_COORDS, message_params);
}

/*
 * Posle klientovi pokyn k vykresleni ctvercu.
 */
void draw_new_squares(std::vector<int> squares, game *game, int active_player) {
    char player_number[2];
    char **coordinates;
    int i;
    std::vector<char*> message_params;

    if (!game) {
        return;
    }

    if (squares.size() == 0) {
        return;
    }

    if (active_player < 1 || active_player > 2) {
        return;
    }

    coordinates = (char**)calloc(squares.size(), sizeof(char*));

    snprintf(player_number, 2, "%d", active_player);
    message_params = {(char*)player_number};

    for (i = 0; i < int(squares.size()); i++) {
        coordinates[i] = (char*)calloc(5, sizeof(char));
        snprintf(coordinates[i], 5, "%d", squares[i]);
        message_params.push_back(coordinates[i]);
    }

    send_message(game->player1->socket, (char*)DRAW_SQUARES, message_params);
    send_message(game->player2->socket, (char*)DRAW_SQUARES, message_params);

    for (i = 0; i < int(squares.size()); i++) {
        free(coordinates[i]);
    }

    free(coordinates);
}

/*
 * Ziska od klienta souradnice a validuje je.
 * Pokud jsou validni, jsou poslany zpravy o vykresleni usecek.
 * Pripadne jsou poslany i zpravy o vykresleni ctvercu.
 * Pokud vznikl ctverec, je prohozen aktivni hrac.
 */
int command_line_coords(int x1, int x2, int client_socket) {
    std::vector<int> new_squares;
    player *player = find_player(client_socket);
    game *game;
    std::vector<char*> message_params;

    if (!player) {
        send_message(client_socket, (char*)INVALID_COORDS, message_params);
        return 0;
    }

    game = player->this_game;

    if (!game) {
        send_message(client_socket, (char*)INVALID_COORDS, message_params);
        return 0;
    }

    if (!check_line_coords(x1, x2, game->this_graph)) {  // nevalidni souradnice vrcholu
        if (game->player1 == player && game->state == ST_PLAYING_1) {
            send_message(game->player1->socket, (char*)INVALID_COORDS, message_params);
        }
        else if (game->player1 == player && game->state == ST_PLAYING_1) {
            send_message(game->player2->socket, (char*)INVALID_COORDS, message_params);
        }
        send_message(client_socket, (char*)INVALID_COORDS, message_params);
        return 0;
    }

    if (game->player1 == player && game->state == ST_PLAYING_1) {
        add_edge(player->this_game->this_graph, x1, x2);
        draw_coords(x1, x2, game);
        new_squares = find_new_cycle_vertices(game->this_graph, game->player1_finished_squares, game->player2_finished_squares, game->player1_score, game->player2_score);

        if (new_squares.size() == 0) {  // zadne nove ctverce, hraje druhy hrac
            game->state = get_game_transition(game->state, EV_PLAY_1_NO_SQUARES);

            message_params = {(char *const)"2"};
            send_message(game->player1->socket, (char*)PLAY, message_params);
            send_message(game->player2->socket, (char*)PLAY, message_params);
        }
        else {  // vytvoreny nove ctverce
            add_new_squares_player1(new_squares, game);
            game->player1_score += new_squares.size();
            draw_new_squares(new_squares, game, 1);

            if (is_full(game->this_graph, game->player1_score, game->player2_score)) {
                // vsechny ctverce vytvorene, existuje vitez
                pick_winner(game);
            }
            else {  // jeste chybi vytvorit ctverce
                game->state = get_game_transition(game->state, EV_PLAY_1_NEW_SQUARES);

                message_params = {(char *const)"1"};
                send_message(game->player1->socket, (char*)PLAY, message_params);
                send_message(game->player2->socket, (char*)PLAY, message_params);
            }
        }
    }
    else if (game->player2 == player && game->state == ST_PLAYING_2) {
        add_edge(player->this_game->this_graph, x1, x2);
        draw_coords(x1, x2, game);
        new_squares = find_new_cycle_vertices(game->this_graph, game->player1_finished_squares, game->player2_finished_squares, game->player1_score, game->player2_score);

        if (new_squares.size() == 0) {  // zadne nove ctverce, hraje druhy hrac
            game->state = get_game_transition(game->state, EV_PLAY_2_NO_SQUARES);

            message_params = {(char *const)"1"};
            send_message(game->player1->socket, (char*)PLAY, message_params);
            send_message(game->player2->socket, (char*)PLAY, message_params);
        }
        else {  // nove ctverce
            add_new_squares_player2(new_squares, game);
            game->player2_score += new_squares.size();
            draw_new_squares(new_squares, game, 2);

            if (is_full(game->this_graph, game->player1_score, game->player2_score)) {
                // vsechny ctverce vytvorene, existuje vitez
                pick_winner(game);
            }
            else {  // jeste chybi vytvorit ctverce
                game->state = get_game_transition(game->state, EV_PLAY_2_NEW_SQUARES);

                message_params = {(char *const)"2"};
                send_message(game->player1->socket, (char*)PLAY, message_params);
                send_message(game->player2->socket, (char*)PLAY, message_params);
            }
        }
    }
    else {
        send_message(client_socket, (char*)INVALID_COORDS, message_params);
        return 0;
    }

    game->last_move = time(NULL);

    return 1;
}

/*
 * Hrac je odstranen z fronty, pokud se v ni nachazi.
 */
int command_leave_queue(int client_socket) {
    player *player = find_player(client_socket);
    int queue_index = find_player_queue_index(client_socket);
    std::vector<char*> message_params;

    if (!player) {
        message_params = {(char*)"error"};
        send_message(client_socket, (char*)LEFT_QUEUE, message_params);
        return 0;
    }

    if (queue_index == -1) {
        message_params = {(char*)"error"};
        send_message(client_socket, (char*)LEFT_QUEUE, message_params);
        return 0;
    }

    if (player->state == ST_QUEUED) {
        if (allowed_player_transition(player->state, EV_LEAVE_QUEUE)) {
            player->state = get_player_transition(player->state, EV_LEAVE_QUEUE);
            global::player_queue.erase(global::player_queue.begin() + queue_index);

            message_params = {(char*)"ok"};
            send_message(client_socket, (char*)LEFT_QUEUE, message_params);
        }
        else {
            message_params = {(char*)"error"};
            send_message(client_socket, (char*)LEFT_QUEUE, message_params);
            return 0;
        }
    }
    else {
        message_params = {(char*)"error"};
        send_message(client_socket, (char*)LEFT_QUEUE, message_params);
        return 0;
    }

    return 1;
}

/*
 * Hrac odejde ze hry, pokud se v ni nachazi.
 */
int command_leave_game(int client_socket) {
    player *player = find_player(client_socket);
    std::vector<char*> message_params;

    if (!player) {
        message_params = {(char*)"error"};
        send_message(client_socket, (char*)YOU_LEFT_GAME, message_params);
        return 0;
    }

    if (player->state == ST_INGAME) {
        if (allowed_player_transition(player->state, EV_LEAVE_GAME)) {
            player->state = get_player_transition(player->state, EV_LEAVE_GAME);

            if (player == player->this_game->player1) {
                player->this_game->player1 = NULL;
            }
            else if (player == player->this_game->player2) {
                player->this_game->player2 = NULL;
            }

            player->this_game = NULL;

            // posleme zpravu odchazejicimu hraci
            message_params = {(char*)"ok"};
            send_message(client_socket, (char*)YOU_LEFT_GAME, message_params);
        }
        else {
            message_params = {(char*)"error"};
            send_message(client_socket, (char*)YOU_LEFT_GAME, message_params);
            return 0;
        }
    }
    else {
        message_params = {(char*)"error"};
        send_message(client_socket, (char*)YOU_LEFT_GAME, message_params);
        return 0;
    }

    return 1;
}

/*
 * Overi odpoved na zpravu PING.
 */
int command_pong(int client_socket) {
    player *player = find_player(client_socket);

    if (!player) {
        return 0;
    }

    player->ponged = true;

    return 1;
}

/*
 * Posle zpravu klientovi, ktery zadal neznamy prikaz.
 */
int command_unknown(int client_socket) {
    std::vector<char*> message_params;

    send_message(client_socket, (char*)UNKNOWN_COMMAND, message_params);

    return 0;
}

/*
 * Podle prikazu zjisti, jakou funkci zavolat.
 * Funkci jsou predany parametry zaslane klientem.
 */
int try_running_command(char *command, std::vector<char*> params, int client_socket) {
    if (!command || !strcmp(command, "")) {
        return 0;
    }

    if (!strcmp(command, LOGIN)) {
        if (params.size() == 0 || !strcmp(params[0], "") || params.size() > 1) {
            return 0;
        }

        return command_login(params[0], client_socket);
    }
    else if (!strcmp(command, QUEUE)) {
        if (params.size() > 0) {
            return 0;
        }
        return command_queue(client_socket);
    }
    else if (!strcmp(command, LINE_COORDS)) {
        if (params.size() < 2 || !strcmp(params[0], "") || !strcmp(params[1], "") || params.size() > 2) {
            return 0;
        }

        if (!is_number(params[0]) || !is_number(params[1])) {
            return 0;
        }

        return command_line_coords(atoi(params[0]), atoi(params[1]), client_socket);
    }
    else if (!strcmp(command, LEAVE_QUEUE)) {
        if (params.size() > 0) {
            return 0;
        }
        return command_leave_queue(client_socket);
    }
    else if (!strcmp(command, LEAVE_GAME)) {
        if (params.size() > 0) {
            return 0;
        }
        return command_leave_game(client_socket);
    }
    else if (!strcmp(command, PONG)) {
        if (params.size() > 0) {
            return 0;
        }
        return command_pong(client_socket);
    }
    else {
        return command_unknown(client_socket);
    }
}

/*
 * Nahodne vybere hrace, ktery bude zacinat, a rozesle zpravy s informaci kdo hraje.
 */
int pick_starting_player(game *game) {
    std::vector<char*> message_params;
    int random;

    if (!game) {
        return 0;
    }

    if (game->player1->state == ST_INGAME && game->player2->state == ST_INGAME && game->state == ST_INIT) {
        random = rand() % 2;
        if (random == 0) {
            if (allowed_game_transition(game->state, EV_CHOOSE_1)) {
                game->state = get_game_transition(game->state, EV_CHOOSE_1);

                message_params = {(char *const)"1"};
                send_message(game->player1->socket, (char*)PLAY, message_params);
                send_message(game->player2->socket, (char*)PLAY, message_params);

                return 1;
            }
            else {
                return 0;
            }
        }
        else {
            if (allowed_game_transition(game->state, EV_CHOOSE_2)) {
                game->state = get_game_transition(game->state, EV_CHOOSE_2);

                message_params = {(char *const)"2"};
                send_message(game->player1->socket, (char*)PLAY, message_params);
                send_message(game->player2->socket, (char*)PLAY, message_params);

                return 1;
            }
            else {
                return 0;
            }
        }
    }

    return 0;
}

/*
 * Vytvori novou hru pro 2 hrace ve fronte.
 * Obema hracum posle zpravu o zacatku hry.
 */
int create_game(player *player1, player *player2, int32_t vertex_count) {
    game *created_game;
    std::vector<char*> message_params;
    char *line;

    if (!player1 || !player2) {
        return 0;
    }

    if (vertex_count <= 0) {
        return 0;
    }

    line = (char*)calloc(LINE_LENGTH, sizeof(char));

    if (allowed_player_transition(player1->state, EV_START_GAME)
        && allowed_player_transition(player2->state, EV_START_GAME)) {

        player1->state = get_player_transition(player1->state, EV_START_GAME);
        player2->state = get_player_transition(player2->state, EV_START_GAME);

        created_game = new_game(player1, player2, vertex_count);
        player1->this_game = created_game;
        player2->this_game = created_game;
        global::games.push_back(created_game);

        snprintf(line, LINE_LENGTH, "New game was created for players %s (socket %d) and %s (socket %d).\n", player1->name, player1->socket, player2->name, player2->socket);
        printf("%s", line);
        log_line(global::log, line);

        message_params = {(char *const)"2", (char *const)player1->name};
        send_message(player2->socket, (char*)START_GAME, message_params);

        message_params.clear();
        message_params = {(char *const)"1", (char *const)player2->name};
        send_message(player1->socket, (char*)START_GAME, message_params);

        if (!pick_starting_player(created_game)) {
            free(line);
            return 0;
        }

        free(line);
        return 1;
    }

    free(line);
    return 0;
}

/*
 * Odstrani ze seznamu vsech her hru, ve ktere je jeden hrac NULL.
 */
void remove_unfinished_game(int game_index) {
    game *game = global::games[game_index];
    std::vector<char*> message_params;

    if (game->player1 != NULL) {
        send_message(game->player1->socket, (char*)OPPONENT_LEFT_GAME, message_params);

        game->player1->this_game = NULL;
        command_queue(game->player1->socket);
    }

    if (game->player2 != NULL) {
        send_message(game->player2->socket, (char*)OPPONENT_LEFT_GAME, message_params);

        game->player2->this_game = NULL;
        command_queue(game->player2->socket);
    }

    global::games.erase(global::games.begin() + game_index);
    free_game(game);
}

/*
 * Odstrani ze seznamu vsech her hru, ve ktere jsou oba hraci NULL.
 */
void remove_finished_game(int game_index) {
    game *game = global::games[game_index];
    global::games.erase(global::games.begin() + game_index);
    free_game(game);
}

/*
 * Posle status pripojeni hrace oponentovi.
 * 0 - ok, 1 - neaktivni (dlouho netahl, ale pinguje), 2 - nepinguje
 */
void send_connection_state_to_opponent(int client_socket, config *conf) {
    player *player;
    std::vector<char*> message_params;

    player = find_player(client_socket);

    if (!player) {
        return;
    }

    if (player->missed_pings > 0) {
        message_params = {(char *const)"2"};
    }
    else {
        if (time(NULL) - player->this_game->last_move > conf->allowed_move_time) {
            if (player == player->this_game->player1 && player->this_game->state == ST_PLAYING_1) {
                message_params = {(char *const)"1"};
            }
            else if (player == player->this_game->player2 && player->this_game->state == ST_PLAYING_2) {
                message_params = {(char *const)"1"};
            }
            else {
                message_params = {(char *const)"0"};
            }
        }
        else {
            message_params = {(char *const)"0"};
        }
    }

    if (player == player->this_game->player1) {
        send_message(player->this_game->player2->socket, (char *)OPPONENT_CONNECTION, message_params);
    }
    else if (player == player->this_game->player2) {
        send_message(player->this_game->player1->socket, (char *)OPPONENT_CONNECTION, message_params);
    }

    return;
}
