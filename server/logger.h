/*
 * Struktura pro logger do souboru.
 */

#include <stdio.h>

#ifndef SP_LOGGER_H
#define SP_LOGGER_H

#define LINE_LENGTH 300

typedef struct {
    FILE *file;
} logger;

logger *create_logger(char *filename);
int log_line(logger *log, char *line);
void free_logger(logger *log);

#endif //SP_LOGGER_H
