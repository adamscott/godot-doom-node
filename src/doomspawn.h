#ifndef DOOMSPAWN_H
#define DOOMSPAWN_H

#include <stdint.h>

#include "doomgeneric/doomtype.h"

#include "doomcommon.h"
#include "doominput.h"

extern boolean terminate;
extern boolean start_loop;

void signal_handler(int signal);
void tick();
unsigned char convert_to_doom_key(Key p_doom_key);

#endif /* DOOMSPAWN_H */
