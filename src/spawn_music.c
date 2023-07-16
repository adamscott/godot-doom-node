#include "doomgeneric/doomtype.h"
#include "string.h"

#include "doomgeneric/deh_str.h"
#include "doomgeneric/i_sound.h"
#include "doomgeneric/sha1.h"

#include "common.h"
#include "shm.h"
#include "spawn.h"

#define SHA1_LEN 20
#define SHA1_HEX_LEN 40

typedef struct LumpSha1 {
	unsigned char sha1_hex[SHA1_HEX_LEN];
	uint8_t handle;
} LumpSha1;

static LumpSha1 lump_sha1_list[UINT8_MAX];
static uint8_t lump_sha1_list_size = 0;

boolean static get_sha1_hex_from_handle(void *handle, char *buffer) {
	uint64_t index = (uint64_t)handle;
	if (index >= lump_sha1_list_size) {
		return false;
	}
	LumpSha1 lump = lump_sha1_list[index];

	memcpy(buffer, lump.sha1_hex, SHA1_HEX_LEN);

	return true;
}

static char *bin2hex(const unsigned char *bin, size_t len) {
	char *out;
	size_t i;

	if (bin == NULL || len == 0)
		return NULL;

	out = malloc(len * 2 + 1);
	for (i = 0; i < len; i++) {
		out[i * 2] = "0123456789ABCDEF"[bin[i] >> 4];
		out[i * 2 + 1] = "0123456789ABCDEF"[bin[i] & 0x0F];
	}
	out[len * 2] = '\0';

	return out;
}

static boolean Godot_InitMusic(void) {
	return true;
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
	memcpy(sha.sha1_hex, bin2hex(hash, SHA1_LEN), SHA1_HEX_LEN);
	lump_sha1_list[lump_sha1_list_size] = sha;
	void *handle = (void *)(uint64_t)lump_sha1_list_size;
	lump_sha1_list_size++;

	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_REGISTER_SONG;
	memcpy(inst.lump_sha1_hex, sha.sha1_hex, SHA1_HEX_LEN);
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;

	return handle;
}

static void Godot_UnRegisterSong(void *handle) {
	char sha1[SHA1_HEX_LEN];

	if (!get_sha1_hex_from_handle(handle, sha1)) {
		return;
	}

	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG;
	memcpy(inst.lump_sha1_hex, sha1, SHA1_HEX_LEN);
	shm->music_instructions[shm->music_instructions_length] = inst;
	shm->music_instructions_length++;
}

static void Godot_PlaySong(void *handle, boolean looping) {
	char sha1[SHA1_HEX_LEN];

	if (!get_sha1_hex_from_handle(handle, sha1)) {
		return;
	}

	MusicInstruction inst;
	inst.type = MUSIC_INSTRUCTION_TYPE_PLAY_SONG;
	memcpy(inst.lump_sha1_hex, sha1, SHA1_HEX_LEN);
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
