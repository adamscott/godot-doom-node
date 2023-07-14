#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomtype.h"
#include "doomgeneric/w_wad.h"

#define GDDOOM_SPAWN_SHM_NAME "/doom"
#define GDDOOM_SPAWN_SHM_ID GDDOOM_SPAWN_SHM_NAME "000000"

typedef struct SharedMemory {
	uint64_t ticks_msec;
	unsigned char *screen_buffer[DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4];
	char window_title[255];
	char keys_pressed[100];
	uint32_t sleep_ms;
	uint8_t terminate : 1;
	uint8_t ready : 1;
	uint8_t init : 1;
	uint32_t sound_memory;
} SharedMemory;

typedef struct SharedMemoryLumps {
	lumpinfo_t lumps[UINT16_MAX];
} SharedMemoryLumps;

#endif /* COMMON_H */
