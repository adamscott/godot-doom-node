#include "common.h"

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
	return false;
}

static void Godot_PrecacheSounds(sfxinfo_t *sounds, int num_sounds) {
	char namebuf[9];
	int i;

	printf("Godot_PrecacheSounds: Precaching all sound effects..");

	for (i = 0; i < num_sounds; ++i) {
		if ((i % 6) == 0) {
			printf(".");
			fflush(stdout);
		}

		GetSfxLumpName(&sounds[i], namebuf, sizeof(namebuf));

		sounds[i].lumpnum = W_CheckNumForName(namebuf);

		if (sounds[i].lumpnum != -1) {
			CacheSFX(&sounds[i]);
		}
	}

	printf("\n");
}

static boolean Godot_SoundIsPlaying(int handle) {
	return false;
}

static void Godot_StopSound(int handle) {}

static int Godot_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep) {
	return 0;
}

static void Godot_UpdateSoundParams(int handle, int vol, int sep) {}

static void Godot_UpdateSound(void) {}

static int Godot_GetSfxLumpNum(sfxinfo_t *sfx) {
	char namebuf[9];

	GetSfxLumpName(sfx, namebuf, sizeof(namebuf));

	return W_GetNumForName(namebuf);
}

static void Godot_ShutdownSound(void) {
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
