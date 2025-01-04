#ifndef DOOMINSTANCE_H
#define DOOMINSTANCE_H

#include <cstdint>

#include <godot_cpp/classes/mutex.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/wrapped.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include "doomcommon.h"
#include "godot_cpp/variant/packed_byte_array.hpp"

extern "C" {
#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/sounds.h"

#include "doominput.h"
}

#define SHA1_LEN 20
#define SHA1_HEX_LEN 41

#define NUM_CHANNELS 16

namespace godot {

class DOOMInstanceMusic {
private:
	typedef struct LumpSha1 {
		unsigned char sha1_hex[SHA1_HEX_LEN];
		uint8_t handle;
	} LumpSha1;

	static LumpSha1 lump_sha1_list[UINT8_MAX];
	static uint8_t lump_sha1_list_size;

public:
	static const uint8_t MUSIC_SDL_DEVICES_LENGTH = 6;
	static snddevice_t music_sdl_devices[MUSIC_SDL_DEVICES_LENGTH];

	static void add_instruction(Ref<DOOMMusicInstruction> &p_instruction);
	static bool get_sha1_hex_from_handle(void *p_handle, char *p_buffer);
	static char *bin2hex(const unsigned char *p_bin, size_t p_length);
	static bool Godot_InitMusic();
	static void Godot_ShutdownMusic();
	static void Godot_SetMusicVolume(int p_volume);
	static void Godot_PauseSong();
	static void Godot_ResumeSong();
	static void *Godot_RegisterSong(void *p_data, int p_length);
	static void Godot_UnRegisterSong(void *p_handle);
	static void Godot_PlaySong(void *p_handle, boolean p_looping);
	static void Godot_StopSong();
	static bool Godot_MusicIsPlaying();
	static void Godot_PollMusic();
};

class DOOMInstanceSound {
private:
	static bool use_sfx_prefix;
	static sfxinfo_t *channels_playing[NUM_CHANNELS];

public:
	static const uint8_t SOUND_SDL_DEVICES_LENGTH = 6;
	static snddevice_t sound_godot_devices[SOUND_SDL_DEVICES_LENGTH];

	static void add_instruction(Ref<DOOMSoundInstruction> &p_instruction);
	static void GetSfxLumpName(sfxinfo_t *p_sfx, char *p_buf, size_t p_buf_len);
	static bool CacheSFX(sfxinfo_t *p_sfxinfo);
	static void Godot_PrecacheSounds(sfxinfo_t *p_sounds, int p_num_sounds);
	static bool Godot_SoundIsPlaying(int p_handle);
	static void Godot_StopSound(int p_handle);
	static int Godot_StartSound(sfxinfo_t *p_sfxinfo, int p_channel, int p_vol, int p_sep);
	static void Godot_UpdateSoundParams(int p_handle, int p_vol, int p_sep);
	static void Godot_UpdateSound();
	static int Godot_GetSfxLumpNum(sfxinfo_t *p_sfx);
	static void Godot_ShutdownSound();
	static bool Godot_InitSound(bool p_use_sfx_prefix);
};

class DOOMInstance : public Object {
	GDCLASS(DOOMInstance, Object);

private:
	thread_local static DOOMInstance *_singleton;

private:
	bool terminate = false;

	bool left_mouse_button_pressed = false;
	bool middle_mouse_button_pressed = false;
	bool right_mouse_button_pressed = false;

	uint32_t local_keys_pressed[UINT8_MAX] = {};
	uint8_t local_keys_pressed_length = 0;
	uint32_t local_mouse_buttons_pressed[UINT8_MAX] = {};
	uint8_t local_mouse_buttons_pressed_length = 0;

public:
	Ref<Mutex> mutex = nullptr;

	uint64_t ticks_msec = 0;
	PackedByteArray screen_buffer;
	char window_title[UINT8_MAX] = {};
	uint32_t keys_pressed[UINT8_MAX] = {};
	uint8_t keys_pressed_length = 0;
	uint32_t mouse_buttons_pressed[UINT8_MAX] = {};
	uint8_t mouse_buttons_pressed_length = 0;
	float mouse_x = 0.0f;
	float mouse_y = 0.0f;
	uint32_t sleep_ms = 0;
	Vector<Ref<DOOMSoundInstruction>> sound_instructions;
	Vector<Ref<DOOMMusicInstruction>> music_instructions;

	String wad;
	String config_dir;

	bool init;
	bool lock;

public:
	void create();
	void tick();
	void wait();
	static unsigned char convert_to_doom_key(Key p_doom_key);

	void DG_Init();
	void DG_DrawFrame();
	void DG_SleepMs(uint32_t p_ms);
	uint32_t DG_GetTicksMs();
	int DG_GetKey(int *p_pressed, unsigned char *p_key);
	void DG_GetMouseState(int *p_mouse_x, int *p_mouse_y, int *p_mouse_button_bitfield);
	void DG_SetWindowTitle(const char *p_title);

protected:
	static void _bind_methods();

public:
	static DOOMInstance *get_singleton() {
		if (_singleton == nullptr) {
			memnew(DOOMInstance);
		}
		return _singleton;
	}

	DOOMInstance();
	~DOOMInstance();
};

} //namespace godot

#endif // DOOMINSTANCE_H
