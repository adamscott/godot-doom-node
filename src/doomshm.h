#ifndef DOOMSHM_H
#define DOOMSHM_H

#include "doomcommon.h"

extern char shm_base_id[256];
extern int shm_fd;
extern SharedMemory *shm;

int init_shm(char *p_id);

#endif /* DOOMSHM_H */
