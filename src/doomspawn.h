#ifndef DOOMSPAWN_H
#define DOOMSPAWN_H

#include <stdint.h>

#include "doomgeneric/doomtype.h"

#include "doomcommon.h"

extern boolean terminate;
extern boolean start_loop;

void signal_handler(int signal);
void tick();

#endif /* DOOMSPAWN_H */
