/*
 * Vstupni modul programu, pripravi server na prijem zprav a vola prislusne prikazy.
 */

#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <time.h>
#include "parser.h"
#include "commands.h"
#include "server.h"
#include "config.h"
#include "logger.h"

namespace global {
    std::vector<player*> players = {};   // vsichni hraci
    std::vector<player*> player_queue = {};  // fronta hracu
    std::vector<game*> games = {};   // vsechny hry
    logger *log = create_logger((char*)"log.txt");  // logger
}

/*
 * Posle pripojenym klientum zpravu o ukonceni serveru.
 */
void send_term_messages() {
    int i;
    player *current_player;
    std::vector<char*> message_params;

    for (i = 0; i < int(global::players.size()); i++) {
        current_player = global::players[i];
        send_message(current_player->socket, (char *)SERVER_SHUTDOWN, message_params);
        //close(current_player->socket);
    }
}

/*
 * Zavola se kdyz se server ukonci Ctrl + C.
 * Posle informaci vsem klientum a uvolni pamet.
 */
void term(int signum) {
    int i;
    player *current_player;
    game *current_game;
    char *line;

    line = (char*)calloc(LINE_LENGTH, sizeof(char));

    send_term_messages();

    for (i = 0; i < int(global::players.size()); i++) {
        current_player = global::players[i];
        free_player(current_player);
    }

    global::players.clear();
    global::player_queue.clear();

    for (i = 0; i < int(global::games.size()); i++) {
        current_game = global::games[i];
        free_game(current_game);
    }

    global::games.clear();

    snprintf(line, LINE_LENGTH, "Server is shutting down.\n");
    printf("%s", line);
    log_line(global::log, line);

    free_logger(global::log);
    free(line);
    exit(EXIT_SUCCESS);
}

/*
 * Rozdeli data na jednotlive zpravy, z tech ziska prikazy a parametry.
 * Preda prikaz a parametry funkci try_running_command(), ktera se postara o vykonani spravneho prikazu.
 * Vraci 0 pro nespravny prikaz (klient bude odpojen) a 1 pri uspechu.
 */
int handle_messages(char *messages, int client_socket) {
    char command[COMMAND_LENGTH];
    int result;
    int i;
    player *player;
    char *line;

    line = (char*)calloc(LINE_LENGTH, sizeof(char));
    memset(command, 0, COMMAND_LENGTH);

    std::vector<char*> messages_divided = divide_messages(messages);
    for (i = 0; i < int(messages_divided.size()); i++) {
        std::vector<char*> params;
        player = find_player(client_socket);

        if (player != NULL) {
            snprintf(line, LINE_LENGTH, "Received message from socket %d (player %s): %s\n", client_socket, player->name, messages_divided[i]);
        }
        else {
            snprintf(line, LINE_LENGTH, "Received message from socket %d: %s\n", client_socket, messages_divided[i]);
        }

        printf("%s", line);
        log_line(global::log, line);

        parse_message(messages_divided[i], command, params);

        result = try_running_command(command, params, client_socket);

        if (!result) {
            free(line);
            return 0;
        }
    }

    free(line);

    return 1;
}

/*
 * Funkce je zavolana kazdych DEFAULT_PING_SECONDS sekund.
 * Zkontroluje vsechny klienty, jestli poslali PONG.
 * Pokud uz DEFAULT_ALLOWED_MISSED_PINGS krat neposlali, jsou odpojeni.
 * Pote je vsem klientum rozeslana zprava PING.
 */
void handle_ping(fd_set *client_socks, config *conf) {
    int i;
    player *current_player;
    std::vector<char*> message_params;
    std::vector<player*> players_to_disconnect;
    char *line;

    line = (char*)calloc(LINE_LENGTH, sizeof(char));

    for (i = 0; i < int(global::players.size()); i++) {
        current_player = global::players[i];

        // kontrola pongu
        if (!current_player->ponged) {
            current_player->missed_pings++;
        }
        else {
            current_player->missed_pings = 0;
        }

        current_player->ponged = false;

        // odpojeni neaktivniho hrace
        if (current_player->missed_pings >= conf->allowed_missed_pings) {
            players_to_disconnect.push_back(current_player);
            continue;
        }

        send_message(current_player->socket, (char*)PING, message_params);
    }

    for (i = 0; i < int(players_to_disconnect.size()); i++) {
        current_player = players_to_disconnect[i];

        send_message(current_player->socket, (char*)DISCONNECTED_INACTIVITY, message_params);

        close(current_player->socket);
        FD_CLR(current_player->socket, client_socks);

        remove_player(current_player->socket);
    }

    free(line);
}

/*
 * Vstupni funkce programu.
 * Stara se o pripravu serveru a prijem zprav.
 */
int main (int argc, char *argv[]) {
    int server_socket;
    int client_socket;
    int fd;
    int return_value;
    char cbuf[MESSAGE_LENGTH];
    socklen_t len_addr;
    int a2read;
    struct sockaddr_in my_addr, peer_addr;
    fd_set client_socks, tests;
    int i;
    time_t time_ping1, time_ping2;
    struct timeval timeout;
    timeout.tv_sec  = 0;
    timeout.tv_usec = 0;
    int reuse_port = 1;
    config *conf;
    char *line;
    struct sigaction action_term, action_int;

    memset(cbuf, 0, sizeof(cbuf));

    // akce pro ukonceni serveru (ctrl + c)
    memset(&action_term, 0, sizeof(struct sigaction));
    action_term.sa_handler = term;
    sigaction(SIGTERM, &action_term, NULL);

    memset(&action_int, 0, sizeof(struct sigaction));
    action_int.sa_handler = term;
    sigaction(SIGINT, &action_int, NULL);

    conf = load_configuration(argc, argv, (char*)"config.txt");

    line = (char*)calloc(LINE_LENGTH, sizeof(char));

    srand(time(NULL));

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_port, sizeof(reuse_port));

    memset(&my_addr, 0, sizeof(struct sockaddr_in));
    memset(&peer_addr, 0, sizeof(struct sockaddr_in));
    memset(&len_addr, 0, sizeof(socklen_t));

    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(conf->port);
    my_addr.sin_addr.s_addr = inet_addr(conf->ip);

    return_value = bind(server_socket, (struct sockaddr *) &my_addr, sizeof(struct sockaddr_in));

    if (return_value == 0)
        printf("Bind - OK\n");
    else {
        printf("Bind - ERR\n");
        return -1;
    }

    return_value = listen(server_socket, 5);
    if (return_value == 0){
        printf("Listen - OK\n");
    } else {
        printf("Listen - ER\n");
    }

    snprintf(line, LINE_LENGTH, "Server is listening for connections.\n");
    printf("%s", line);
    log_line(global::log, line);

    time_ping1 = time(NULL);

    // vyprazdnime sadu deskriptoru a vlozime server socket
    FD_ZERO(&client_socks);
    FD_SET(server_socket, &client_socks);

    while (1) {
        tests = client_socks;
        // sada deskriptoru je po kazdem volani select prepsana sadou deskriptoru kde se neco delo
        return_value = select(FD_SETSIZE, &tests, (fd_set *)0, (fd_set *)0, &timeout);

        if (return_value != -1) {
            // vynechavame stdin, stdout, stderr
            for(fd = 3; fd < FD_SETSIZE; fd++) {
                // je dany socket v sade fd ze kterych lze cist ?
                if (FD_ISSET(fd, &tests)) {
                    // je to server socket ? prijmeme nove spojeni
                    if (fd == server_socket) {
                        client_socket = accept(server_socket, (struct sockaddr *) &peer_addr, &len_addr);
                        FD_SET(client_socket, &client_socks);
                        //setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, &reuse_port, sizeof(reuse_port));

                        snprintf(line, LINE_LENGTH, "New connection on socket %d.\n", client_socket);
                        printf("%s", line);
                        log_line(global::log, line);
                    }
                        // je to klientsky socket ? prijmem data
                    else {
                        // pocet bajtu co je pripraveno ke cteni
                        ioctl(fd, FIONREAD, &a2read);
                        // mame co cist
                        if (a2read > 0) {
                            recv(fd, &cbuf, sizeof(cbuf), 0);
                            if (!handle_messages(cbuf, fd)) {
                                disconnect_suspicious_client(fd);
                                FD_CLR(fd, &client_socks);
                            }
                            memset(cbuf, 0, sizeof(cbuf));
                        }
                            // na socketu se stalo neco spatneho
                        else {
                            //close(fd);
                            FD_CLR(fd, &client_socks);
                            //remove_player(fd);

                            snprintf(line, LINE_LENGTH, "Client on socket %d disconnected.\n", fd);
                            printf("%s", line);
                            log_line(global::log, line);
                        }
                    }
                }
            }
        }

        // pingovani vsech hracu kazdych 10 sekund
        time_ping2 = time(NULL);
        if (time_ping2 - time_ping1 >= conf->ping_seconds) {
            handle_ping(&client_socks, conf);

            // poslat upozorneni oponentovi o pripojeni hrace (pingnul - ok, nepingnul - kratkodobe nedostupny)
            for (i = 0; i < int(global::games.size()); i++) {
                if (global::games[i]->state != ST_WIN) {
                    if (global::games[i]->player1 && global::games[i]->player2) {
                        send_connection_state_to_opponent(global::games[i]->player1->socket, conf);
                        send_connection_state_to_opponent(global::games[i]->player2->socket, conf);
                    }
                }
            }

            time_ping1 = time_ping2;
        }

        // vytvoreni nove hry pokud jsou ve fronte aspon 2
        if (global::player_queue.size() >= 2) {
            player *player1 = global::player_queue.front();
            global::player_queue.erase(global::player_queue.begin());

            player *player2 = global::player_queue.front();
            global::player_queue.erase(global::player_queue.begin());

            create_game(player1, player2, conf->vertex_count);
        }

        // osetreni jestli v kazde hre jsou oba pripojeni
        // ukoncit hru pokud je minimalne jeden dlouhodobe odpojeny
        for (i = 0; i < int(global::games.size()); i++) {
            if (!global::games[i]->player1 || !global::games[i]->player2) {
                if (global::games[i]->state != ST_WIN) {
                    remove_unfinished_game(i);
                }
            }
        }

        // osetreni pro hry ve stavu WIN, odstraneni hry
        for (i = 0; i < int(global::games.size()); i++) {
            if (!global::games[i]->player1 && !global::games[i]->player2) {
                if (global::games[i]->state == ST_WIN) {
                    remove_finished_game(i);
                }
            }
        }
    }

    return 0;
}