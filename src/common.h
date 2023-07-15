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
	int32_t channel;
	int32_t volume;
	int32_t sep;
	int32_t pitch;
	int32_t priority;
	int32_t usefulness;
} SoundInstruction;

typedef enum MusicInstructionType {
	MUSIC_INSTRUCTION_TYPE_EMPTY,
	MUSIC_INSTRUCTION_TYPE_INIT,
	MUSIC_INSTRUCTION_TYPE_SHUTDOWN,
	MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME,
	MUSIC_INSTRUCTION_TYPE_PAUSE_SONG,
	MUSIC_INSTRUCTION_TYPE_RESUME_SONG,
	MUSIC_INSTRUCTION_TYPE_REGISTER_SONG,
	MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG,
	MUSIC_INSTRUCTION_TYPE_PLAY_SONG,
	MUSIC_INSTRUCTION_TYPE_STOP_SONG,
} MusicInstructionType;

typedef struct MusicInstruction {
	MusicInstructionType type;
	char lump_sha1[20];
	int32_t volume;
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
