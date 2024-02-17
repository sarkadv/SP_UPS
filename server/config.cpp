/*
 * Modul s konfiguracemi programu.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "config.h"
#include "parser.h"

/*
 * Zjisti, jestli je retezec IP adresou.
 */
bool is_ip_adress(char *ip)
{
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, ip, &(sa.sin_addr));
    return result == 1;
}

/*
 * Zjisti, jestli je retezec portem.
 */
bool is_port(char *port) {
    int number;

    if (!is_number(port)) {
        return false;
    }

    number = atoi(port);

    if (number < 0 || number > 65535) {
        return false;
    }

    return true;
}

/*
 * Nastavi do konfiguracni struktury defaultni hodnoty.
 */
void default_config(config *conf) {
    if (!conf) {
        return;
    }

    strncpy(conf->ip, DEFAULT_IP, 15);
    conf->port = DEFAULT_PORT;

    printf("Program arguments or config file containing sensible values was not provided.\nDefault values 127.0.0.1 and 10000 will be used.\n");
}

/*
 * Nastavi do konfiguracni struktury hodnoty ze souboru.
 */
void load_from_file(config *conf, char *filename) {
    FILE *fin;
    char ip[15];
    int32_t port;
    char buffer[20];

    if (!conf) {
        return;
    }

    memset(buffer, 0, sizeof(buffer));
    memset(ip, 0, sizeof(ip));

    printf("Program arguments were not provided or were wrong, will attempt to use config file.\n");

    fin  = fopen(filename, "r");

    if (!fin) {
        default_config(conf);
        return;
    }

    if (fgets (buffer, sizeof(buffer), fin) == NULL) {
        default_config(conf);
        return;
    }

    if (buffer[strlen(buffer) - 1] == '\n') {
        buffer[strlen(buffer) - 1] = '\0';
    }

    if (!strcmp(buffer, "localhost")) {
        strncpy(ip, "127.0.0.1", sizeof(ip));
    }
    else if (!strcmp(buffer, "any")) {
        strncpy(ip, "0.0.0.0", sizeof(ip));
    }
    else if (is_ip_adress(buffer)) {
        strncpy(ip, buffer, sizeof(ip));
    }
    else {
        default_config(conf);
        return;
    }

    memset(buffer, 0, sizeof(buffer));

    if (fgets (buffer, sizeof(buffer), fin) == NULL) {
        default_config(conf);
        return;
    }

    if (buffer[strlen(buffer) - 1] == '\n') {
        buffer[strlen(buffer) - 1] = '\0';
    }

    if (!is_port(buffer)) {
        default_config(conf);
        return;
    }

    port = atoi(buffer);

    fclose(fin);

    strncpy(conf->ip, ip, sizeof(conf->ip));
    conf->port = port;
}

/*
 * Nastavi do konfiguracni struktury hodnoty argumentu programu.
 */
config *load_configuration(int argc, char *argv[], char *filename) {
    config *conf;

    conf = (config*)calloc(1, sizeof(config));

    conf->vertex_count = DEFAULT_VERTEX_COUNT;
    conf->ping_seconds = DEFAULT_PING_SECONDS;
    conf->allowed_missed_pings = DEFAULT_ALLOWED_MISSED_PINGS;
    conf->allowed_move_time = DEFAULT_ALLOWED_MOVE_TIME;

    if (argc != 3) {
        printf("Usage: ./server <ip> <port>\n");
        load_from_file(conf, filename);
        return conf;
    }

    if (!strcmp(argv[1], "localhost")) {
        strncpy(conf->ip, "127.0.0.1", 15);
    }
    else if (!strcmp(argv[1], "any")) {
        strncpy(conf->ip, "0.0.0.0", 15);
    }
    else if (is_ip_adress(argv[1])) {
        strncpy(conf->ip, argv[1], 15);
    }
    else {
        load_from_file(conf, filename);
        return conf;
    }

    if (!is_port(argv[2])) {
        load_from_file(conf, filename);
        return conf;
    }

    conf->port = atoi(argv[2]);

    return conf;
}
