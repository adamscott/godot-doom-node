#ifndef SHM_H
#define SHM_H

#include "shm.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

// Shared memory:
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "common.h"

char shm_base_id[256];
int shm_fd = 0;
SharedMemory *shm = NULL;

int init_shm(char *p_id) {
	strcpy(shm_base_id, p_id);

	shm_fd = shm_open(shm_base_id, O_RDWR, 0666);
	if (shm_fd == -1) {
		printf("shm_open failed. %s\n", strerror(errno));
		return -1;
	}

	shm = mmap(0, sizeof(SharedMemory), PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm == NULL) {
		printf("mmap failed. %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

#endif /* SHM_H */
