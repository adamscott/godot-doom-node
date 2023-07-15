#include "doomgeneric/doomtype.h"
#include "string.h"

#include "doomgeneric/deh_str.h"
#include "doomgeneric/i_sound.h"
#include "doomgeneric/sha1.h"

#include "common.h"
#include "shm.h"
#include "spawn.h"

#define SHA1_LEN 20

typedef struct LumpSha1 {
	unsigned char sha1[SHA1_LEN];
	void *handle;
} LumpSha1;

static LumpSha1 lump_sha1_list[UINT8_MAX];
static uint8_t lump_sha1_list_size = 0;

boolean static get_sha1_from_handle(void *handle, char *buffer) {
	char sha1[20];
	boolean found = false;
	for (int i = 0; i < lump_sha1_list_size; i++) {
		LumpSha1 lump = lump_sha1_list[i];
		if (lump.handle == handle) {
			memcpy(sha1, lump.sha1, SHA1_LEN);
			found = true;
			break;
		}
	}

	if (!found) {
		return false;
	}

	memcpy(buffer, sha1, SHA1_LEN);

	return true;
}

static boolean Godot_InitMusic(void) {
	return false;
}

static void Godot_ShutdownMusic(void) {
	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_SHUTDOWN_MUSIC;
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;
}

static void Godot_SetMusicVolume(int volume) {
	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME;
	inst.volume = volume;
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;
}

static void Godot_PauseSong(void) {
	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_PAUSE_SONG;
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;
}

static void Godot_ResumeSong(void) {
	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_RESUME_SONG;
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;
}

static void *Godot_RegisterSong(void *data, int len) {
	sha1_context_t context;
	sha1_digest_t hash;

	SHA1_Init(&context);
	SHA1_Update(&context, data, len);
	SHA1_Final(hash, &context);

	LumpSha1 sha;
	memcpy(sha.sha1, hash, SHA1_LEN);
	lump_sha1_list[lump_sha1_list_size] = sha;
	void *handle = &sha.handle;
	lump_sha1_list_size++;

	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_REGISTER_SONG;
	memcpy(inst.lump_sha1, hash, SHA1_LEN);
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;

	return handle;
}

static void Godot_UnRegisterSong(void *handle) {
	char sha1[20];

	if (!get_sha1_from_handle(handle, sha1)) {
		return;
	}

	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG;
	memcpy(inst.lump_sha1, sha1, SHA1_LEN);
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;
}

static void Godot_PlaySong(void *handle, boolean looping) {
	char sha1[20];

	if (!get_sha1_from_handle(handle, sha1)) {
		return;
	}

	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_PLAY_SONG;
	memcpy(inst.lump_sha1, sha1, SHA1_LEN);
	inst.looping = true;
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;
}

static void Godot_StopSong(void) {
	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_STOP_SONG;
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;
}

static boolean Godot_MusicIsPlaying(void) {
	return true;
}

static void Godot_PollMusic(void) {}

static snddevice_t music_sdl_devices[] = {
	SNDDEVICE_PAS,
	SNDDEVICE_GUS,
	SNDDEVICE_WAVEBLASTER,
	SNDDEVICE_SOUNDCANVAS,
	SNDDEVICE_GENMIDI,
	SNDDEVICE_AWE32,
};

music_module_t DG_music_module = {
	music_sdl_devices,
	arrlen(music_sdl_devices),
	Godot_InitMusic,
	Godot_ShutdownMusic,
	Godot_SetMusicVolume,
	Godot_PauseSong,
	Godot_ResumeSong,
	Godot_RegisterSong,
	Godot_UnRegisterSong,
	Godot_PlaySong,
	Godot_StopSong,
	Godot_MusicIsPlaying,
	Godot_PollMusic,
};
