/*
 * Modul s konfiguracemi programu.
 */

#ifndef DOTSANDBOXES_CONFIG_H
#define DOTSANDBOXES_CONFIG_H

#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_PORT 10000
#define DEFAULT_VERTEX_COUNT 25
#define DEFAULT_PING_SECONDS 5
#define DEFAULT_ALLOWED_MISSED_PINGS 12
#define DEFAULT_ALLOWED_MOVE_TIME 30

typedef struct {
    char ip[15];
    int32_t port;
    int32_t vertex_count;
    int32_t ping_seconds;
    int32_t allowed_missed_pings;
    int32_t allowed_move_time;
} config;

config *load_configuration(int argc, char *argv[], char *filename);

#endif //DOTSANDBOXES_CONFIG_H
