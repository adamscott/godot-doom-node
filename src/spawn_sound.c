#include <string.h>

// Shared memory:
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "common.h"
#include "shm.h"
#include "spawn.h"

#include "doomgeneric/deh_str.h"
#include "doomgeneric/i_sound.h"
#include "doomgeneric/m_misc.h"
#include "doomgeneric/w_wad.h"

#define NUM_CHANNELS 16

static boolean use_sfx_prefix;
static sfxinfo_t *channels_playing[NUM_CHANNELS];

static snddevice_t sound_godot_devices[] = {
	SNDDEVICE_SB,
	SNDDEVICE_PAS,
	SNDDEVICE_GUS,
	SNDDEVICE_WAVEBLASTER,
	SNDDEVICE_SOUNDCANVAS,
	SNDDEVICE_AWE32,
};

static void add_instruction(SoundInstruction inst) {
	shm->sound_instructions[shm->sound_instructions_length] = inst;
	shm->sound_instructions_length++;
}

static void GetSfxLumpName(sfxinfo_t *sfx, char *buf, size_t buf_len) {
	// Linked sfx lumps? Get the lump number for the sound linked to.

	if (sfx->link != NULL) {
		sfx = sfx->link;
	}

	// Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't
	// do this.

	if (use_sfx_prefix) {
		M_snprintf(buf, buf_len, "ds%s", DEH_String(sfx->name));
	} else {
		M_StringCopy(buf, DEH_String(sfx->name), buf_len);
	}
}

// Preload all the sound effects - stops nasty ingame freezes

static boolean CacheSFX(sfxinfo_t *sfxinfo) {
	return true;
}

static void Godot_PrecacheSounds(sfxinfo_t *sounds, int num_sounds) {
	// char namebuf[9];

	for (int i = 0; i < num_sounds; i++) {
		sfxinfo_t *sound = &sounds[i];
		// GetSfxLumpName(sound, namebuf, sizeof(namebuf));

		SoundInstruction inst;
		inst.type = SOUND_INSTRUCTION_TYPE_PRECACHE_SOUND;
		// strcpy(inst.name, namebuf);
		strcpy(inst.name, sound->name);
		inst.pitch = sound->pitch;
		inst.volume = sound->volume;
		inst.priority = sound->priority;
		inst.usefulness = sound->usefulness;

		add_instruction(inst);
	}
}

static boolean Godot_SoundIsPlaying(int handle) {
	return true;
}

static void Godot_StopSound(int handle) {
	SoundInstruction inst;
	inst.type = SOUND_INSTRUCTION_TYPE_STOP_SOUND;
	inst.channel = handle;

	add_instruction(inst);
}

static int Godot_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep) {
	SoundInstruction inst;
	inst.type = SOUND_INSTRUCTION_TYPE_START_SOUND;
	strcpy(inst.name, sfxinfo->name);
	inst.channel = channel;
	inst.volume = vol;
	inst.sep = sep;
	inst.pitch = sfxinfo->pitch;

	shm->sound_instructions[shm->sound_instructions_length] = inst;
	shm->sound_instructions_length++;

	channels_playing[channel] = sfxinfo;

	return channel;
}

static void Godot_UpdateSoundParams(int handle, int vol, int sep) {
	// printf("update sound %d, vol: %d\n", handle, vol);

	SoundInstruction inst;
	inst.channel = handle;
	inst.volume = vol;
	inst.sep = sep;
}

static void Godot_UpdateSound(void) {
	// printf("update sound\n");
}

static int Godot_GetSfxLumpNum(sfxinfo_t *sfx) {
	char namebuf[9];

	GetSfxLumpName(sfx, namebuf, sizeof(namebuf));

	return W_GetNumForName(namebuf);
}

static void Godot_ShutdownSound(void) {
	printf("shutdown sound");
}

static boolean Godot_InitSound(boolean _use_sfx_prefix) {
	use_sfx_prefix = _use_sfx_prefix;

	int i;
	for (i = 0; i < NUM_CHANNELS; ++i) {
		channels_playing[i] = NULL;
	}

	return true;
}

sound_module_t DG_sound_module = {
	sound_godot_devices,
	arrlen(sound_godot_devices),
	Godot_InitSound,
	Godot_ShutdownSound,
	Godot_GetSfxLumpNum,
	Godot_UpdateSound,
	Godot_UpdateSoundParams,
	Godot_StartSound,
	Godot_StopSound,
	Godot_SoundIsPlaying,
	Godot_PrecacheSounds,
};
