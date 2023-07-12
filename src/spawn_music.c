#include "common.h"

#include "doomgeneric/deh_str.h"
#include "doomgeneric/i_sound.h"

static boolean Godot_InitMusic(void) {
	return false;
}

static void Godot_ShutdownMusic(void) {}

static void Godot_SetMusicVolume(int volume) {}

static void Godot_PauseSong(void) {}

static void Godot_ResumeSong(void) {}

static void *Godot_RegisterSong(void *data, int len) {}

static void Godot_UnRegisterSong(void *handle) {}

static void Godot_PlaySong(void *handle, boolean looping) {}

static void Godot_StopSong(void) {}

static boolean Godot_MusicIsPlaying(void) {
	return false;
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
