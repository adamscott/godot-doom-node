#ifndef GDDOOM_H
#define GDDOOM_H

#include <cstdint>

#include "doomgeneric/doomgeneric.h"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/image_texture.hpp"
#include "godot_cpp/classes/texture_rect.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/templates/vector.hpp"
#include "godot_cpp/variant/dictionary.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"

extern "C" {
#include "common.h"
#include "spawn.h"
}

namespace godot {

class GDDoom : public TextureRect {
	GDCLASS(GDDoom, TextureRect);

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

	static int _last_id;
	int _id;
	char _shm_id[255];

	bool _exiting = false;

	Ref<Thread> _thread = nullptr;
	Ref<Thread> _thread_wad = nullptr;

	SharedMemory *_shm;
	__pid_t _spawn_pid;
	int _shm_fd;

	Vector<String> _uuids;

	bool enabled = false;
	String wad_path;

	// TextureRect *texture_rect;
	Ref<ImageTexture> img_texture = nullptr;
	unsigned char screen_buffer[DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4];
	PackedByteArray screen_buffer_array;

	void _init_shm();
	void _thread_func();
	void _thread_parse_wad();

	void init_doom();
	void kill_doom();
	void launch_doom_executable();
	void append_sounds();

	void update_doom();

protected:
	static void _bind_methods();

public:
	GDDoom();
	~GDDoom();

	bool get_enabled();
	void set_enabled(bool p_enabled);
	String get_wad_path();
	void set_wad_path(String p_wad_path);

	void _enter_tree() override;
	void _exit_tree() override;
	void _ready() override;
	void _process(double delta) override;
};

} // namespace godot

#endif // GDDOOM_H
