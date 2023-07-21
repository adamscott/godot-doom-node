#ifndef DOOMSHM_H
#define DOOMSHM_H

#ifdef __cplusplus
extern "C" {
#endif

#include "doomcommon.h"

extern char shm_base_id[256];
extern int shm_fd;
extern SharedMemory *shm;

int init_shm(char *p_id);

#ifdef __cplusplus
}
#endif

#endif /* DOOMSHM_H */
