#ifndef COMMON_H
#define COMMON_H

#include <stdbool.h>
#include <stdint.h>

#include <doomgeneric/doomgeneric.h>

#define GDDOOM_SPAWN_SHM_NAME "/doom"
#define GDDOOM_SPAWN_SHM_ID GDDOOM_SPAWN_SHM_NAME "000000"

typedef struct SharedMemory {
	uint64_t ticks_msec;
	uint32_t screen_buffer;
	char keys_pressed[100];
	uint32_t sleep_ms;
	bool terminate;
	bool ready;
	bool init;
} SharedMemory;

#endif
