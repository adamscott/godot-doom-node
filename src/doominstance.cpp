#include "doominstance.h"
#include "doomcommon.h"
#include "godot_cpp/classes/time.hpp"

#include <ctype.h>
#include <cstdint>

#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

extern "C" {
#include <unistd.h>

#include "doomgeneric/deh_str.h"
#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomkeys.h"
#include "doomgeneric/i_sound.h"
#include "doomgeneric/m_misc.h"
#include "doomgeneric/sha1.h"
#include "doomgeneric/w_wad.h"
}

namespace godot {

// =================
// DOOMInstanceMusic
// =================

DOOMInstanceMusic::LumpSha1 DOOMInstanceMusic::lump_sha1_list[UINT8_MAX] = {};
uint8_t DOOMInstanceMusic::lump_sha1_list_size = 0;

snddevice_t DOOMInstanceMusic::music_sdl_devices[DOOMInstanceMusic::MUSIC_SDL_DEVICES_LENGTH] = {
	SNDDEVICE_PAS,
	SNDDEVICE_GUS,
	SNDDEVICE_WAVEBLASTER,
	SNDDEVICE_SOUNDCANVAS,
	SNDDEVICE_GENMIDI,
	SNDDEVICE_AWE32,
};

void DOOMInstanceMusic::add_instruction(Ref<DOOMMusicInstruction> &p_instruction) {
	DOOMInstance::get_singleton()->music_instructions.append(p_instruction);
}

bool DOOMInstanceMusic::get_sha1_hex_from_handle(void *p_handle, char *p_buffer) {
	uint64_t index = (uint64_t)p_handle;
	if (index >= lump_sha1_list_size) {
		return false;
	}
	LumpSha1 lump = lump_sha1_list[index];

	memcpy(p_buffer, lump.sha1_hex, SHA1_HEX_LEN);

	return true;
}

char *DOOMInstanceMusic::bin2hex(const unsigned char *p_bin, size_t p_length) {
	char *out;
	size_t i;

	if (p_bin == NULL || p_length == 0)
		return NULL;

	out = (char *)malloc(p_length * 2 + 1);
	for (i = 0; i < p_length; i++) {
		out[i * 2] = "0123456789ABCDEF"[p_bin[i] >> 4];
		out[i * 2 + 1] = "0123456789ABCDEF"[p_bin[i] & 0x0F];
	}
	out[p_length * 2] = '\0';

	return out;
}

bool DOOMInstanceMusic::Godot_InitMusic() {
	return true;
}

void DOOMInstanceMusic::Godot_ShutdownMusic() {
	Ref<DOOMMusicInstruction> inst;
	inst.instantiate();
	inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_SHUTDOWN_MUSIC;
	add_instruction(inst);
}

void DOOMInstanceMusic::Godot_SetMusicVolume(int p_volume) {
	Ref<DOOMMusicInstruction> inst;
	inst.instantiate();
	inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME;
	inst->volume = p_volume;
	add_instruction(inst);
}

void DOOMInstanceMusic::Godot_PauseSong() {
	Ref<DOOMMusicInstruction> inst;
	inst.instantiate();
	inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_PAUSE_SONG;
	add_instruction(inst);
}

void DOOMInstanceMusic::Godot_ResumeSong() {
	Ref<DOOMMusicInstruction> inst;
	inst.instantiate();
	inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_RESUME_SONG;
	add_instruction(inst);
}

void *DOOMInstanceMusic::Godot_RegisterSong(void *p_data, int p_length) {
	sha1_context_t context;
	sha1_digest_t hash;

	SHA1_Init(&context);
	SHA1_Update(&context, (byte *)p_data, p_length);
	SHA1_Final(hash, &context);

	LumpSha1 sha;
	memcpy(sha.sha1_hex, bin2hex(hash, SHA1_LEN), SHA1_HEX_LEN);
	lump_sha1_list[lump_sha1_list_size] = sha;
	void *handle = (void *)(uint64_t)lump_sha1_list_size;
	lump_sha1_list_size++;

	Ref<DOOMMusicInstruction> inst;
	inst.instantiate();
	inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_REGISTER_SONG;
	inst->lump_sha1_hex = String::utf8((const char *)sha.sha1_hex, SHA1_HEX_LEN);
	add_instruction(inst);

	return handle;
}

void DOOMInstanceMusic::Godot_UnRegisterSong(void *p_handle) {
	char sha1[SHA1_HEX_LEN];

	if (!get_sha1_hex_from_handle(p_handle, sha1)) {
		return;
	}

	Ref<DOOMMusicInstruction> inst;
	inst.instantiate();
	inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG;
	inst->lump_sha1_hex = sha1;
	add_instruction(inst);
}

void DOOMInstanceMusic::Godot_PlaySong(void *p_handle, boolean p_looping) {
	char sha1[SHA1_HEX_LEN];

	if (!get_sha1_hex_from_handle(p_handle, sha1)) {
		return;
	}

	Ref<DOOMMusicInstruction> inst;
	inst.instantiate();
	inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_PLAY_SONG;
	inst->lump_sha1_hex = sha1;
	inst->looping = true;
	add_instruction(inst);
}

void DOOMInstanceMusic::Godot_StopSong() {
	Ref<DOOMMusicInstruction> inst;
	inst.instantiate();
	inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_STOP_SONG;
	add_instruction(inst);
}

bool DOOMInstanceMusic::Godot_MusicIsPlaying() {
	return true;
}

void DOOMInstanceMusic::Godot_PollMusic() {}

// =================
// DOOMInstanceSound
// =================
bool DOOMInstanceSound::use_sfx_prefix = false;
sfxinfo_t *DOOMInstanceSound::channels_playing[NUM_CHANNELS] = {};

snddevice_t DOOMInstanceSound::sound_godot_devices[DOOMInstanceSound::SOUND_SDL_DEVICES_LENGTH] = {
	SNDDEVICE_SB,
	SNDDEVICE_PAS,
	SNDDEVICE_GUS,
	SNDDEVICE_WAVEBLASTER,
	SNDDEVICE_SOUNDCANVAS,
	SNDDEVICE_AWE32,
};

void DOOMInstanceSound::add_instruction(Ref<DOOMSoundInstruction> &p_instruction) {
	DOOMInstance::get_singleton()->sound_instructions.append(p_instruction);
}

void DOOMInstanceSound::GetSfxLumpName(sfxinfo_t *p_sfx, char *p_buffer, size_t p_buffer_length) {
	// Linked sfx lumps? Get the lump number for the sound linked to.

	if (p_sfx->link != NULL) {
		p_sfx = p_sfx->link;
	}

	// Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't
	// do this.

	if (use_sfx_prefix) {
		M_snprintf(p_buffer, p_buffer_length, "ds%s", DEH_String(p_sfx->name));
	} else {
		M_StringCopy(p_buffer, DEH_String(p_sfx->name), p_buffer_length);
	}
}

bool DOOMInstanceSound::CacheSFX(sfxinfo_t *p_sfxinfo) {
	return true;
}

void DOOMInstanceSound::Godot_PrecacheSounds(sfxinfo_t *p_sounds, int p_num_sounds) {
	// char namebuf[9];

	for (int i = 0; i < p_num_sounds; i++) {
		sfxinfo_t *sound = &p_sounds[i];
		// GetSfxLumpName(sound, namebuf, sizeof(namebuf));

		Ref<DOOMSoundInstruction> inst;
		inst.instantiate();
		inst->type = DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_PRECACHE_SOUND;
		inst->name = sound->name;
		inst->pitch = sound->pitch;
		inst->volume = sound->volume;
		inst->priority = sound->priority;
		inst->usefulness = sound->usefulness;

		add_instruction(inst);
	}
}

bool DOOMInstanceSound::Godot_SoundIsPlaying(int p_handle) {
	return true;
}

void DOOMInstanceSound::Godot_StopSound(int p_handle) {
	Ref<DOOMSoundInstruction> inst;
	inst.instantiate();
	inst->type = DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_STOP_SOUND;
	inst->channel = p_handle;

	add_instruction(inst);
}

int DOOMInstanceSound::Godot_StartSound(sfxinfo_t *p_sfxinfo, int p_channel, int p_vol, int p_sep) {
	Ref<DOOMSoundInstruction> inst;
	inst.instantiate();
	inst->type = DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_START_SOUND;
	inst->name = p_sfxinfo->name;
	inst->channel = p_channel;
	inst->volume = p_vol;
	inst->sep = p_sep;
	inst->pitch = p_sfxinfo->pitch;

	add_instruction(inst);

	channels_playing[p_channel] = p_sfxinfo;

	return p_channel;
}

void DOOMInstanceSound::Godot_UpdateSoundParams(int p_handle, int p_vol, int p_sep) {
	Ref<DOOMSoundInstruction> inst;
	inst.instantiate();
	inst->channel = p_handle;
	inst->volume = p_vol;
	inst->sep = p_sep;
}

void DOOMInstanceSound::Godot_UpdateSound() {}

int DOOMInstanceSound::Godot_GetSfxLumpNum(sfxinfo_t *p_sfx) {
	char namebuf[9];

	GetSfxLumpName(p_sfx, namebuf, sizeof(namebuf));

	return W_GetNumForName(namebuf);
}

void DOOMInstanceSound::Godot_ShutdownSound() {
	Ref<DOOMSoundInstruction> inst;
	inst.instantiate();
	inst->type = DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_SHUTDOWN_SOUND;
	add_instruction(inst);
}

bool DOOMInstanceSound::Godot_InitSound(bool p_use_sfx_prefix) {
	use_sfx_prefix = p_use_sfx_prefix;

	int i;
	for (i = 0; i < NUM_CHANNELS; ++i) {
		channels_playing[i] = NULL;
	}

	return true;
}

// ============
// DOOMInstance
// ============

thread_local DOOMInstance *DOOMInstance::_singleton = nullptr;

void DOOMInstance::create() {
	const int ARGS_LEN = 5;

	CharStringT<char> _wad = wad.utf8();
	CharStringT<char> _config_dir = config_dir.utf8();

	char *prepared_args[ARGS_LEN] = {
		strdup("godot-doom-node"),
		strdup("-iwad"),
		(char *)_wad.get_data(),
		strdup("-configdir"),
		(char *)_config_dir.get_data(),
	};

	char **args = prepared_args;
	UtilityFunctions::print(vformat("-iwad %s -configdir %s", args[1], args[3]));

	::doomgeneric_Create(ARGS_LEN, args);
	init = true;
}

void DOOMInstance::tick() {
	doomgeneric_Tick();
}

unsigned char DOOMInstance::convert_to_doom_key(Key p_doom_key) {
	switch (p_doom_key) {
		case Key::KEY_A:
		case Key::KEY_B:
		case Key::KEY_C:
		case Key::KEY_D:
		case Key::KEY_E:
		case Key::KEY_F:
		case Key::KEY_G:
		case Key::KEY_H:
		case Key::KEY_I:
		case Key::KEY_J:
		case Key::KEY_K:
		case Key::KEY_L:
		case Key::KEY_M:
		case Key::KEY_N:
		case Key::KEY_O:
		case Key::KEY_P:
		case Key::KEY_Q:
		case Key::KEY_R:
		case Key::KEY_S:
		case Key::KEY_T:
		case Key::KEY_U:
		case Key::KEY_V:
		case Key::KEY_W:
		case Key::KEY_X:
		case Key::KEY_Y:
		case Key::KEY_Z: {
			return toupper(p_doom_key);
		}

#pragma push_macro("KEY_F1")
#undef KEY_F1
		case Key::KEY_F1: {
#pragma pop_macro("KEY_F1")
			return KEY_F1;
		}
#pragma push_macro("KEY_F2")
#undef KEY_F2
		case Key::KEY_F2: {
#pragma pop_macro("KEY_F2")
			return KEY_F2;
		}
#pragma push_macro("KEY_F3")
#undef KEY_F3
		case Key::KEY_F3: {
#pragma pop_macro("KEY_F3")
			return KEY_F3;
		}
#pragma push_macro("KEY_F4")
#undef KEY_F4
		case Key::KEY_F4: {
#pragma pop_macro("KEY_F4")
			return KEY_F4;
		}
#pragma push_macro("KEY_F5")
#undef KEY_F5
		case Key::KEY_F5: {
#pragma pop_macro("KEY_F5")
			return KEY_F5;
		}
#pragma push_macro("KEY_F6")
#undef KEY_F6
		case Key::KEY_F6: {
#pragma pop_macro("KEY_F6")
			return KEY_F6;
		}
#pragma push_macro("KEY_F7")
#undef KEY_F7
		case Key::KEY_F7: {
#pragma pop_macro("KEY_F7")
			return KEY_F7;
		}
#pragma push_macro("KEY_F8")
#undef KEY_F8
		case Key::KEY_F8: {
#pragma pop_macro("KEY_F8")
			return KEY_F8;
		}
#pragma push_macro("KEY_F9")
#undef KEY_F9
		case Key::KEY_F9: {
#pragma pop_macro("KEY_F9")
			return KEY_F9;
		}
#pragma push_macro("KEY_F10")
#undef KEY_F10
		case Key::KEY_F10: {
#pragma pop_macro("KEY_F10")
			return KEY_F10;
		}
#pragma push_macro("KEY_F11")
#undef KEY_F11
		case Key::KEY_F11: {
#pragma pop_macro("KEY_F11")
			return KEY_F11;
		}
#pragma push_macro("KEY_F12")
#undef KEY_F12
		case Key::KEY_F12: {
#pragma pop_macro("KEY_F12")
			return KEY_F12;
		}

#pragma push_macro("KEY_ENTER")
#undef KEY_ENTER
		case Key::KEY_ENTER: {
#pragma pop_macro("KEY_ENTER")
			return KEY_ENTER;
		}
#pragma push_macro("KEY_ESCAPE")
#undef KEY_ESCAPE
		case Key::KEY_ESCAPE: {
#pragma pop_macro("KEY_ESCAPE")
			return KEY_ESCAPE;
		}
#pragma push_macro("KEY_TAB")
#undef KEY_TAB
		case Key::KEY_TAB: {
#pragma pop_macro("KEY_TAB")
			return KEY_TAB;
		}
		case Key::KEY_LEFT: {
			return KEY_LEFTARROW;
		}
		case Key::KEY_UP: {
			return KEY_UPARROW;
		}
		case Key::KEY_DOWN: {
			return KEY_DOWNARROW;
		}
		case Key::KEY_RIGHT: {
			return KEY_RIGHTARROW;
		}
#pragma push_macro("KEY_BACKSPACE")
#undef KEY_BACKSPACE
		case Key::KEY_BACKSPACE: {
#pragma pop_macro("KEY_BACKSPACE")
			return KEY_BACKSPACE;
		}
#pragma push_macro("KEY_PAUSE")
#undef KEY_PAUSE
		case Key::KEY_PAUSE: {
#pragma pop_macro("KEY_PAUSE")
			return KEY_PAUSE;
		}

		case Key::KEY_SHIFT: {
			return KEY_RSHIFT;
		}
		case Key::KEY_ALT: {
			return KEY_RALT;
		}
		case Key::KEY_CTRL: {
			return KEY_FIRE;
		}
		case Key::KEY_COMMA: {
			return KEY_STRAFE_L;
		}
		case Key::KEY_PERIOD: {
			return KEY_STRAFE_R;
		}
		case Key::KEY_SPACE: {
			return KEY_USE;
		}
		case Key::KEY_PLUS:
#pragma push_macro("KEY_MINUS")
#undef KEY_MINUS
		case Key::KEY_MINUS: {
#pragma pop_macro("KEY_MINUS")
			return KEY_MINUS;
		}
		case Key::KEY_EQUAL: {
			return KEY_EQUALS;
		}

		case Key::KEY_KP_0:
		case Key::KEY_KP_1:
		case Key::KEY_KP_2:
		case Key::KEY_KP_3:
		case Key::KEY_KP_4:
		case Key::KEY_KP_5:
		case Key::KEY_KP_6:
		case Key::KEY_KP_7:
		case Key::KEY_KP_8:
		case Key::KEY_KP_9:
		default: {
			return p_doom_key;
		}
	}
}

void DOOMInstance::DG_Init() {
	screen_buffer.resize(SCREEN_BUFFER_SIZE);
}

void DOOMInstance::DG_DrawFrame() {
	if (DG_ScreenBuffer == nullptr) {
		return;
	}

	memcpy(screen_buffer.ptrw(), DG_ScreenBuffer, SCREEN_BUFFER_SIZE);

	uint8_t *screen_buffer_ptr = screen_buffer.ptrw();
	for (int i = 0; i < SCREEN_BUFFER_SIZE; i += RGBA) {
		uint8_t red = screen_buffer_ptr[i + 2];
		uint8_t green = screen_buffer_ptr[i + 1];
		uint8_t blue = screen_buffer_ptr[i + 0];
		uint8_t alpha = 255 - screen_buffer_ptr[i + 3];

		screen_buffer_ptr[i + 0] = red;
		screen_buffer_ptr[i + 1] = green;
		screen_buffer_ptr[i + 2] = blue;
		screen_buffer_ptr[i + 3] = alpha;
	}
}

void DOOMInstance::DG_SleepMs(uint32_t p_ms) {
	sleep_ms = p_ms;
	OS::get_singleton()->delay_msec(p_ms);
}

uint32_t DOOMInstance::DG_GetTicksMs() {
	return Time::get_singleton()->get_ticks_msec(); /* return milliseconds */
}

int DOOMInstance::DG_GetKey(int *p_pressed, unsigned char *p_key) {
	// If the local buffer is empty, fill it
	if (keys_pressed_length > 0 && local_keys_pressed_length == 0) {
		memcpy(local_keys_pressed, keys_pressed, sizeof(local_keys_pressed));
		local_keys_pressed_length = keys_pressed_length;
		keys_pressed_length = 0;
	}

	if (local_keys_pressed_length > 0) {
		uint32_t key_pressed = local_keys_pressed[local_keys_pressed_length - 1];
		*p_pressed = key_pressed >> 31;
		*p_key = convert_to_doom_key((Key)(key_pressed & ~(1 << 31)));

		local_keys_pressed_length -= 1;
		return true;
	}

	local_keys_pressed_length = 0;
	return false;
}

void DOOMInstance::DG_GetMouseState(int *p_mouse_x, int *p_mouse_y, int *p_mouse_button_bitfield) {
	*p_mouse_x = mouse_x;
	*p_mouse_y = mouse_y;

	mouse_x = 0;
	mouse_y = 0;

	boolean pressed = false;
	int8_t index = 0;

	while (mouse_buttons_pressed_length > 0) {
		uint32_t mouse_button_pressed = mouse_buttons_pressed[mouse_buttons_pressed_length - 1];
		pressed = mouse_button_pressed >> 31;
		index = ~(1 << 31) & mouse_button_pressed;

		switch (index) {
			// left
			case 1: {
				left_mouse_button_pressed = pressed;
			} break;

			// right
			case 2: {
				right_mouse_button_pressed = pressed;
			} break;

			// middle
			case 3: {
				middle_mouse_button_pressed = pressed;
			} break;

			default: {
			}
		}

		mouse_buttons_pressed_length -= 1;
	}

	mouse_buttons_pressed_length = 0;

	*p_mouse_button_bitfield = left_mouse_button_pressed | right_mouse_button_pressed << 1 | middle_mouse_button_pressed << 2;
}

void DOOMInstance::DG_SetWindowTitle(const char *p_title) {
	// printf("DG_SetWindowTitle(%s)\n", title);
	if (sizeof(p_title) > sizeof(window_title)) {
		printf("WARN: Could not copy window title \"%s\", as it's longer than 255 characters.", p_title);
		return;
	}

	strcpy(window_title, p_title);
}

void DOOMInstance::_bind_methods() {
}

DOOMInstance::DOOMInstance() {
	_singleton = this;
	mutex.instantiate();
}

DOOMInstance::~DOOMInstance() {
	_singleton = nullptr;
}

} //namespace godot

int use_libsamplerate = 0;
float libsamplerate_scale = 1.0;

void DG_DrawFrame() {
	godot::DOOMInstance *instance = godot::DOOMInstance::get_singleton();
	instance->DG_DrawFrame();
}

int DG_GetKey(int *p_pressed, unsigned char *p_key) {
	return godot::DOOMInstance::get_singleton()->DG_GetKey(p_pressed, p_key);
}

uint32_t DG_GetTicksMs() {
	return godot::DOOMInstance::get_singleton()->DG_GetTicksMs();
}

void DG_Init() {
	godot::DOOMInstance::get_singleton()->DG_Init();
}

void DG_SetWindowTitle(const char *p_title) {
	godot::DOOMInstance::get_singleton()->DG_SetWindowTitle(p_title);
}

void DG_SleepMs(uint32_t p_ms) {
	godot::DOOMInstance::get_singleton()->DG_SleepMs(p_ms);
}

// Music module.

music_module_t DG_music_module = {
	godot::DOOMInstanceMusic::music_sdl_devices,
	sizeof(godot::DOOMInstanceMusic::music_sdl_devices),
	godot::DOOMInstanceMusic::Godot_InitMusic,
	godot::DOOMInstanceMusic::Godot_ShutdownMusic,
	godot::DOOMInstanceMusic::Godot_SetMusicVolume,
	godot::DOOMInstanceMusic::Godot_PauseSong,
	godot::DOOMInstanceMusic::Godot_ResumeSong,
	godot::DOOMInstanceMusic::Godot_RegisterSong,
	godot::DOOMInstanceMusic::Godot_UnRegisterSong,
	godot::DOOMInstanceMusic::Godot_PlaySong,
	godot::DOOMInstanceMusic::Godot_StopSong,
	godot::DOOMInstanceMusic::Godot_MusicIsPlaying,
	godot::DOOMInstanceMusic::Godot_PollMusic,
};

sound_module_t DG_sound_module = {
	godot::DOOMInstanceSound::sound_godot_devices,
	sizeof(godot::DOOMInstanceSound::sound_godot_devices),
	godot::DOOMInstanceSound::Godot_InitSound,
	godot::DOOMInstanceSound::Godot_ShutdownSound,
	godot::DOOMInstanceSound::Godot_GetSfxLumpNum,
	godot::DOOMInstanceSound::Godot_UpdateSound,
	godot::DOOMInstanceSound::Godot_UpdateSoundParams,
	godot::DOOMInstanceSound::Godot_StartSound,
	godot::DOOMInstanceSound::Godot_StopSound,
	godot::DOOMInstanceSound::Godot_SoundIsPlaying,
	godot::DOOMInstanceSound::Godot_PrecacheSounds,
};
