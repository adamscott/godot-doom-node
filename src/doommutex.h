#ifndef DOOMMUTEX_H
#define DOOMMUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <assert.h>
#include <unistd.h>

#include "doomcommon.h"

static inline void mutex_lock(SharedMemory *shm) {
	assert(shm != NULL);
	while (shm->lock) {
		usleep(1);
	}
	shm->lock = true;
}

static inline void mutex_unlock(SharedMemory *shm) {
	shm->lock = false;
}

#ifdef __cplusplus
}
#endif

#endif /* DOOMMUTEX_H */
