/*
 * Vstupni modul programu, pripravi server na prijem zprav a vola prislusne prikazy.
 */

#ifndef DOTSANDBOXES_SERVER_H
#define DOTSANDBOXES_SERVER_H

#include <queue>
#include <vector>
#include "state_machine.h"
#include "logger.h"

namespace global {
    extern std::vector<player*> player_queue;  // fronta hracu
    extern std::vector<player*> players;   // vsichni hraci
    extern std::vector<game*> games;   // vsechny hry
    extern logger *log;
}

#endif //DOTSANDBOXES_SERVER_H

