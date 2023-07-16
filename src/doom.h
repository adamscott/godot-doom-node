#ifndef DOOM_H
#define DOOM_H

#include <cstdint>

#include "doomgeneric/doomgeneric.h"
#include "godot_cpp/classes/audio_stream_generator.hpp"
#include "godot_cpp/classes/audio_stream_generator_playback.hpp"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/image_texture.hpp"
#include "godot_cpp/classes/mutex.hpp"
#include "godot_cpp/classes/texture_rect.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

extern "C" {
#include "common.h"
#include "spawn.h"
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

	WadSignature signature;
	Dictionary files;

	static int last_id;
	int id;
	char shm_id[255];

	bool exiting = false;
	bool assets_ready = false;
	bool sound_fetch_complete = false;
	bool midi_fetch_complete = false;

	Ref<Mutex> mutex;
	Ref<Thread> doom_thread = nullptr;
	Ref<Thread> midi_thread = nullptr;
	Ref<Thread> wad_thread = nullptr;
	Ref<Thread> sound_fetching_thread = nullptr;
	Ref<Thread> midi_fetching_thread = nullptr;

	SharedMemory *shm;
	__pid_t spawn_pid;
	int shm_fd;

	Vector<String> uuids;
	Vector<SoundInstruction> sound_instructions;
	Vector<MusicInstruction> music_instructions;

	bool enabled = false;
	String wad_path;
	String soundfont_path;

	String current_midi_path;
	bool current_midi_playing = false;
	uint32_t current_midi_tick = 0;
	bool current_midi_pause = false;
	bool current_midi_looping = false;
	uint32_t current_midi_volume = 0;
	uint32_t len_asked = 0;
	// PackedVector2Array current_midi_frames;
	float current_midi_buffer[32767 * 2];
	uint64_t current_midi_last_tick = 0;

	Ref<AudioStreamGenerator> current_midi_stream = nullptr;
	Ref<AudioStreamGeneratorPlayback> current_midi_playback = nullptr;

	String wad_name;
	String wad_hash;

	// TextureRect *texture_rect;
	Ref<ImageTexture> img_texture = nullptr;
	unsigned char screen_buffer[DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4];
	PackedByteArray screen_buffer_array;

	void init_shm();

	void doom_thread_func();
	void wad_thread_func();
	void sound_fetching_thread_func();
	void midi_fetching_thread_func();
	void midi_thread_func();

	void append_sounds();
	void append_music();

	void wad_thread_end();
	void sound_fetching_thread_end();
	void midi_fetching_thread_end();

	void start_sound_fetching();
	void start_midi_fetching();
	void update_assets_status();

	void init_doom();
	void kill_doom();
	void launch_doom_executable();

	void update_screen_buffer();
	void update_sounds();
	void update_music();

	void update_doom();

	bool get_import_assets();
	void set_import_assets(bool p_import_assets);

protected:
	static void _bind_methods();

public:
	DOOM();
	~DOOM();

	bool get_enabled();
	void set_enabled(bool p_enabled);
	String get_wad_path();
	void set_wad_path(String p_wad_path);
	String get_soundfont_path();
	void set_soundfont_path(String p_soundfont_path);

	void import_assets();

	void _enter_tree() override;
	void _exit_tree() override;
	void _ready() override;
	void _process(double delta) override;
};

} // namespace godot

#endif // DOOM_H
