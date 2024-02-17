/*
 * Parser pro zpracovavani dat od klientu.
 */

#ifndef DOTSANDBOXES_PARSER_H
#define DOTSANDBOXES_PARSER_H
#include <vector>

#define MESSAGE_END "\n"
#define MESSAGE_SEPARATOR "|"
#define COMMAND_LENGTH 20

int parse_message(char *message, char *command, std::vector<char*> &params);
std::vector<char*> divide_messages(char *messages);
int is_number(char *string);
void remove_spaces(char *str_trimmed, char *str_untrimmed);

#endif //DOTSANDBOXES_PARSER_H
