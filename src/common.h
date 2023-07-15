#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomtype.h"
#include "doomgeneric/w_wad.h"

#define GODOT_DOOM_SHM_NAME "/doom"
#define GODOT_DOOM_SPAWN_SHM_ID GODOT_DOOM_SHM_NAME "000000"

typedef enum SoundInstructionType {
	SOUND_INSTRUCTION_TYPE_EMPTY,
	SOUND_INSTRUCTION_TYPE_PRECACHE_SOUND,
	SOUND_INSTRUCTION_TYPE_START_SOUND,
	SOUND_INSTRUCTION_TYPE_STOP_SOUND,
	SOUND_INSTRUCTION_TYPE_UPDATE_SOUND_PARAMS,
	SOUND_INSTRUCTION_TYPE_SHUTDOWN,
	SOUND_INSTRUCTION_TYPE_MAX
} SoundInstructionType;

typedef struct SoundInstruction {
	SoundInstructionType type;
	char name[9];
	int8_t channel;
	int8_t volume;
	int8_t sep;
	int8_t pitch;
	int8_t priority;
	int8_t usefulness;
} SoundInstruction;

typedef enum MusicInstructionType {
	MUSIC_INSTRUCTION_TYPE_EMPTY
} MusicInstructionType;

typedef struct MusicInstruction {
	MusicInstructionType type;
} MusicInstruction;

typedef struct SharedMemory {
	uint64_t ticks_msec;
	unsigned char *screen_buffer[DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4];
	char window_title[255];
	char keys_pressed[100];
	uint32_t sleep_ms;
	uint8_t terminate : 1;
	uint8_t ready : 1;
	uint8_t init : 1;
	uint8_t sound_instructions_length;
	SoundInstruction sound_instructions[UINT8_MAX];
} SharedMemory;

#endif /* COMMON_H */
