#ifndef SPAWN_H
#define SPAWN_H

#include <stdint.h>

#include "doomgeneric/doomtype.h"

#include "common.h"

extern boolean terminate;
extern boolean start_loop;

void signal_handler(int signal);
void tick();

#endif /* SPAWN_H */
