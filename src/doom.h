#ifndef DOOM_H
#define DOOM_H

#include <cstdint>

#include "doomgeneric/doomgeneric.h"
#include "godot_cpp/classes/audio_stream_generator.hpp"
#include "godot_cpp/classes/audio_stream_generator_playback.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/image_texture.hpp"
#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/texture_rect.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

extern "C" {
#include "fluidsynth.h"

#include "doomcommon.h"
#include "doomspawn.h"
}

namespace godot {

class DOOM : public TextureRect {
	GDCLASS(DOOM, TextureRect);

private:
	struct WadOriginalSignature {
		char sig[4];
		int32_t numFiles;
		int32_t offFAT;
	};

	struct WadSignature {
		String signature;
		int32_t number_of_files;
		int32_t fat_offset;
	};

	struct WadOriginalFileEntry {
		int32_t offData;
		int32_t lenData;
		char name[8];
	};

	struct WadFileEntry {
		int32_t offset_data;
		int32_t length_data;
		String name;
	};

	WadSignature _wad_signature;
	Dictionary _wad_files;

	static int _last_doom_instance_id;
	int _doom_instance_id;
	char _shm_id[255];

	bool _exiting = false;
	bool _assets_ready = false;
	bool _sound_fetch_complete = false;
	bool _midi_fetch_complete = false;

	Ref<Mutex> _mutex;
	Ref<Thread> _doom_thread = nullptr;
	Ref<Thread> _midi_thread = nullptr;
	Ref<Thread> _wad_thread = nullptr;
	Ref<Thread> _sound_fetching_thread = nullptr;
	Ref<Thread> _midi_fetching_thread = nullptr;

	SharedMemory *_shm;
	__pid_t _spawn_pid;
	int _shm_fd;

	Vector<String> _uuids;
	Vector<SoundInstruction> _sound_instructions;
	Vector<MusicInstruction> _music_instructions;

	bool _enabled = false;
	bool _wasd_mode = false;
	float _mouse_acceleration = 1.0f;
	bool _autosave = false;

	String _wad_path;
	String _soundfont_path;

	String _current_midi_path;
	bool current_midi_playing = false;
	uint32_t _current_midi_tick = 0;
	bool current_midi_pause = false;
	bool _current_midi_looping = false;
	uint32_t _current_midi_volume = 0;
	uint64_t _current_midi_last_tick_usec = 0;
	Dictionary _stored_midi_files;
	String _current_midi_file;

	fluid_settings_t *_fluid_settings;
	fluid_synth_t *_fluid_synth;
	int _fluid_synth_id = -1;
	fluid_player_t *_fluid_player;

	Ref<AudioStreamGenerator> _current_midi_stream = nullptr;
	Ref<AudioStreamGeneratorPlayback> _current_midi_playback = nullptr;

	String _wad_name;
	String _wad_hash;

	// TextureRect *texture_rect;
	Ref<ImageTexture> _img_texture = nullptr;
	unsigned char _screen_buffer[DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4];
	PackedByteArray _screen_buffer_array;

	void _init_shm();

	void _doom_thread_func();
	void _wad_thread_func();
	void _sound_fetching_thread_func();
	void _midi_fetching_thread_func();
	void _midi_thread_func();

	void _append_sounds();
	void _append_music();

	void _wad_thread_end();
	void _sound_fetching_thread_end();
	void _midi_fetching_thread_end();

	void _start_sound_fetching();
	void _start_midi_fetching();
	void _update_assets_status();

	void _stop_music();

	void _init_doom();
	void _kill_doom();
	void _stop_doom();
	__pid_t _launch_doom_executable();

	void _update_screen_buffer();
	void _update_sounds();
	void _update_music();
	void _update_doom();

protected:
	static void _bind_methods();

public:
	DOOM();
	~DOOM();

	bool get_autosave();
	void set_autosave(bool p_autosave);
	bool get_wasd_mode();
	void set_wasd_mode(bool p_wasd_mode);
	float get_mouse_acceleration();
	void set_mouse_acceleration(float p_mouse_acceleration);
	bool get_import_assets();
	void set_import_assets(bool p_import_assets);
	bool get_assets_ready();
	void set_assets_ready(bool p_ready);
	bool get_enabled();
	void set_enabled(bool p_enabled);
	String get_wad_path();
	void set_wad_path(String p_wad_path);
	String get_soundfont_path();
	void set_soundfont_path(String p_soundfont_path);

	void import_assets();

	virtual void _enter_tree() override;
	virtual void _exit_tree() override;
	virtual void _ready() override;
	virtual void _process(double p_delta) override;
	virtual void _input(const Ref<InputEvent> &event) override;
};

} // namespace godot

#endif // DOOM_H
