/*
 * Parser pro zpracovavani dat od klientu.
 */

#include <string.h>
#include <vector>
#include <string>
#include "parser.h"

/*
 * Rozdeli data na zpravy podle rozdelovace \n.
 */
std::vector<char*> divide_messages(char *messages) {
    char const *delimiter = MESSAGE_END;
    std::vector<char*> messages_divided;
    char* token;

    token = strtok(messages, delimiter);

    while (token != NULL) {
        messages_divided.push_back(token);

        token = strtok(NULL, delimiter);
    }

    return messages_divided;
}

/*
 * Zpracuje zpravu, na adresu command ulozi prikaz a do vektoru params ulozi parametry.
 */
int parse_message(char *message, char *command, std::vector<char*> &params) {
    char const *delimiter = MESSAGE_SEPARATOR;
    char* token;

    if (!message || !command) {
        return 0;
    }

    token = strtok(message, delimiter);

    if (token) {
        strncpy(command, token, COMMAND_LENGTH);
    }
    else {
        strncpy(command, "", COMMAND_LENGTH);
    }

    token = strtok(NULL, delimiter);

    while (token) {
        params.push_back(token);
        token = strtok(NULL, delimiter);
    }

    return 1;
}

/*
 * Zjisti, zda jsou vsechny znaky v retezci cislo.
 * Vrati 1 - ano, 0 - ne.
 */
int is_number(char *string) {
    int i;

    for (i = 0; i < int(strlen(string)); i++) {
        if(!isdigit(string[i])) {
            return 0;
        }
    }

    return 1;
}

/*
 * Odstrani z retezce mezery.
 */
void remove_spaces(char *str_trimmed, char *str_untrimmed) {
    while (*str_untrimmed != '\0') {
        if (!isspace(*str_untrimmed)) {
            *str_trimmed = *str_untrimmed;
            str_trimmed++;
        }

        str_untrimmed++;
    }

    *str_trimmed = '\0';
}
