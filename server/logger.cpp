/*
 * Struktura pro logger do souboru.
 */

#include <stdlib.h>
#include "logger.h"

/*
 * Vytvori a vrati strukturu loggeru.
 */
logger *create_logger(char *filename) {
    logger *log;
    FILE *fp;

    log = (logger*)calloc(1, sizeof(logger));
    fp  = fopen(filename, "w");

    if (!fp) {
        return NULL;
    }

    log->file = fp;
    return log;
}

/*
 * Zaloguje radek line do souboru.
 */
int log_line(logger *log, char *line) {
    if (!log) {
        return 0;
    }

    fprintf(log->file, "%s", line);
    fflush(log->file);

    return 1;
}

/*
 * Uvolni strukturu loggeru.
 */
void free_logger(logger *log) {
    if (!log) {
        return;
    }

    fclose(log->file);
    free(log);
}
