#ifndef SHM_H
#define SHM_H

#include "common.h"

extern char shm_base_id[256];
extern int shm_fd;
extern SharedMemory *shm;

int init_shm(char *p_id);

#endif /* SHM_H */
