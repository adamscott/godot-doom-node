#include <signal.h>
#include <cstdint>
#include <cstring>

#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/audio_stream_wav.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/image_texture.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/texture_rect.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/variant.hpp"

#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomtype.h"

#include "gddoom.h"
#include "mus2mid.h"
#include "swap.h"

extern "C" {
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
// Shared memory
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

// GDDoom
#include "common.h"

// Fluidsynth
#include "fluidsynth.h"
}

using namespace godot;

// ======
// GDDoom
// ======

void GDDoom::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_thread_func"), &GDDoom::_thread_func);
	ClassDB::bind_method(D_METHOD("_thread_parse_wad"), &GDDoom::_thread_parse_wad);
	ClassDB::bind_method(D_METHOD("append_sounds"), &GDDoom::append_sounds);

	ADD_GROUP("DOOM", "doom_");

	ClassDB::bind_method(D_METHOD("get_enabled"), &GDDoom::get_enabled);
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &GDDoom::set_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "doom_enabled"), "set_enabled", "get_enabled");

	ClassDB::bind_method(D_METHOD("get_wad_path"), &GDDoom::get_wad_path);
	ClassDB::bind_method(D_METHOD("set_wad_path", "wad_path"), &GDDoom::set_wad_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "doom_wad_path", PROPERTY_HINT_FILE, "*.wad"), "set_wad_path", "get_wad_path");
}

bool GDDoom::get_enabled() {
	return enabled;
}

void GDDoom::set_enabled(bool p_enabled) {
	enabled = p_enabled;

	if (p_enabled) {
		init_doom();
	} else {
		kill_doom();
	}
}

String GDDoom::get_wad_path() {
	return wad_path;
}

void GDDoom::set_wad_path(String p_wad_path) {
	wad_path = p_wad_path;
}

void GDDoom::update_doom() {
	if (enabled && FileAccess::file_exists(wad_path)) {
		init_doom();
	} else {
		kill_doom();
	}
}

void GDDoom::init_doom() {
	_exiting = false;

	UtilityFunctions::print("init_doom!");
	_init_shm();
	_shm->ticks_msec = Time::get_singleton()->get_ticks_msec();

	_spawn_pid = fork();
	if (_spawn_pid == 0) {
		launch_doom_executable();
	}

	_thread_wad.instantiate();
	_thread.instantiate();

	Node *sound_container = get_node<Node>("SoundContainer");
	if (sound_container != nullptr) {
		sound_container->set_name("tobedeleted");
		sound_container->queue_free();
	}
	sound_container = memnew(Node);
	add_child(sound_container);
	sound_container->set_owner(get_tree()->get_edited_scene_root());
	sound_container->set_name("SoundContainer");

	for (int i = 0; i < 16; i++) {
		AudioStreamPlayer *player = memnew(AudioStreamPlayer);
		sound_container->add_child(player);
		player->set_owner(get_tree()->get_edited_scene_root());
		player->set_name(vformat("Channel%s", i));
	}

	Callable func = Callable(this, "_thread_func");
	Callable parse_wad = Callable(this, "_thread_parse_wad");
	UtilityFunctions::print("Calling _thread.start()");
	_thread->start(func);
	_thread_wad->start(parse_wad);
	// _thread_parse_wad();
}

void GDDoom::launch_doom_executable() {
	const char *path = ProjectSettings::get_singleton()->globalize_path("res://bin/gddoom-spawn.linux.template_debug.x86_64").utf8().get_data();
	const char *id = vformat("%s", _shm_id).utf8().get_data();
	const char *doom1_wad = vformat("%s", ProjectSettings::get_singleton()->globalize_path(wad_path)).utf8().get_data();

	char *args[] = {
		strdup(path),
		strdup("-id"),
		strdup(id),
		strdup("-iwad"),
		strdup(doom1_wad),
		NULL
	};

	char *envp[] = {
		NULL
	};

	// UtilityFunctions::print("launching execve");
	execve(args[0], args, envp);
}

void GDDoom::_thread_parse_wad() {
	UtilityFunctions::print(vformat("_thread_parse_wad called"));

	files.clear();

	Ref<FileAccess> wad = FileAccess::open(wad_path, FileAccess::ModeFlags::READ);
	if (wad.is_null()) {
		UtilityFunctions::printerr(vformat("Could not open \"%s\"", wad_path));
		return;
	}

	wad->seek(0);
	PackedByteArray sig_array = wad->get_buffer(sizeof(WadOriginalSignature));

	signature.signature = vformat("%c%c%c%c", sig_array[0], sig_array[1], sig_array[2], sig_array[3]);
	signature.number_of_files = sig_array.decode_s32(sizeof(WadOriginalSignature::sig));
	signature.fat_offset = sig_array.decode_s32(sizeof(WadOriginalSignature::sig) + sizeof(WadOriginalSignature::numFiles));
	UtilityFunctions::print(vformat("signature: %s, number of files: %x, offset: %x", signature.signature, signature.number_of_files, signature.fat_offset));

	wad->seek(signature.fat_offset);
	PackedByteArray wad_files = wad->get_buffer(sizeof(WadOriginalFileEntry) * signature.number_of_files);
	for (int i = 0; i < signature.number_of_files; i++) {
		WadFileEntry file_entry;
		file_entry.offset_data = wad_files.decode_s32(i * sizeof(WadOriginalFileEntry));
		file_entry.length_data = wad_files.decode_s32(i * sizeof(WadOriginalFileEntry) + sizeof(WadOriginalFileEntry::offData));

		uint16_t name_offset = i * sizeof(WadOriginalFileEntry) + sizeof(WadOriginalFileEntry::offData) + sizeof(WadOriginalFileEntry::lenData);
		file_entry.name = vformat(
				"%c%c%c%c%c%c%c%c",
				wad_files[name_offset + 0],
				wad_files[name_offset + 1],
				wad_files[name_offset + 2],
				wad_files[name_offset + 3],
				wad_files[name_offset + 4],
				wad_files[name_offset + 5],
				wad_files[name_offset + 6],
				wad_files[name_offset + 7]);
		wad->seek(file_entry.offset_data);
		PackedByteArray file_array = wad->get_buffer(file_entry.length_data);

		Dictionary info;
		info["data"] = file_array;

		files[file_entry.name] = info;
		// UtilityFunctions::print(vformat("file %s, size: %x", file_entry.name, file_entry.length_data));
	}

	// Find sounds
	Array keys = files.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("DS")) {
			continue;
		}
		Dictionary info = files[key];
		PackedByteArray file_array = info["data"];

		AudioStreamPlayer *player = memnew(AudioStreamPlayer);

		player->set_name(key);

		// https://doomwiki.org/wiki/Sound
		uint16_t format_number = file_array.decode_u16(0x00);
		uint16_t sample_rate = file_array.decode_u16(0x02);
		uint32_t number_of_samples = file_array.decode_u32(0x04);
		PackedByteArray samples;
		for (int j = 0; j < number_of_samples - 0x10; j++) {
			// https://docs.godotengine.org/en/stable/classes/class_audiostreamwav.html#property-descriptions
			samples.append(file_array.decode_u8(j + 0x18) - 128);
		}

		Ref<AudioStreamWAV> wav;
		wav.instantiate();
		wav->set_data(samples);
		wav->set_mix_rate(sample_rate);
		player->set_stream(wav);

		info["player"] = player;
	}

	// Find music
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("D_")) {
			continue;
		}

		Dictionary info = files[key];
		PackedByteArray file_array = info["data"];
		PackedByteArray midi_output;

		GDDoomMus2Mid::get_singleton()->mus2mid(file_array, midi_output);
	}

	call_deferred("append_sounds");
}

void GDDoom::append_sounds() {
	Node *sound_container = get_node<Node>("SoundContainer");

	// Find sounds
	Array keys = files.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("DS")) {
			continue;
		}
		Dictionary info = files[key];
		AudioStreamPlayer *player = reinterpret_cast<AudioStreamPlayer *>((Object *)info["player"]);
		sound_container->add_child(player);
		player->set_owner(get_tree()->get_edited_scene_root());
	}
}

void GDDoom::kill_doom() {
	if (_spawn_pid == 0) {
		return;
	}

	// UtilityFunctions::print("kill_doom()");
	_exiting = true;
	if (!_thread.is_null()) {
		_thread->wait_to_finish();
	}

	// UtilityFunctions::print(vformat("Removing %s", _shm_id));
	int result = shm_unlink(_shm_id);
	if (result < 0) {
		UtilityFunctions::printerr(vformat("ERROR unlinking shm %s: %s", _shm_id, strerror(errno)));
	}

	if (_spawn_pid > 0) {
		kill(_spawn_pid, SIGKILL);
	}
	_spawn_pid = 0;
	// UtilityFunctions::print("ending kill doom!");
}

void GDDoom::_process(double delta) {
	if (_spawn_pid == 0) {
		return;
	}

	// UtilityFunctions::print(vformat("_spawn_pid: %s", _spawn_pid));

	update_screen_buffer();
	update_sounds();
}

void GDDoom::update_screen_buffer() {
	screen_buffer_array.resize(sizeof(screen_buffer));
	screen_buffer_array.clear();

	for (int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4; i += 4) {
		// screen_buffer_array.insert(i, screen_buffer[i]);
		screen_buffer_array.insert(i, screen_buffer[i + 2]);
		screen_buffer_array.insert(i + 1, screen_buffer[i + 1]);
		screen_buffer_array.insert(i + 2, screen_buffer[i + 0]);
		screen_buffer_array.insert(i + 3, 255 - screen_buffer[i + 3]);
	}

	Ref<Image> image = Image::create_from_data(DOOMGENERIC_RESX, DOOMGENERIC_RESY, false, Image::Format::FORMAT_RGBA8, screen_buffer_array);
	if (img_texture->get_image().is_null()) {
		img_texture->set_image(image);
	} else {
		img_texture->update(image);
	}
}

void GDDoom::update_sounds() {
	Node *sound_container = get_node<Node>("SoundContainer");
	if (sound_container == nullptr) {
		return;
	}

	for (SoundInstructions instructions : sound_instructions) {
		switch (instructions.type) {
			case SOUND_INSTRUCTION_TYPE_START_SOUND: {
				String name = vformat("DS%s", String(instructions.name).to_upper());
				AudioStreamPlayer *source = sound_container->get_node<AudioStreamPlayer>(name);
				UtilityFunctions::print(vformat("trying to duplicate %s, %d", name, instructions.pitch));

				if (source == nullptr) {
					continue;
				}

				// AudioStreamPlayer *clone = (AudioStreamPlayer *)source->duplicate();
				// clone->set_pitch_scale(instructions.pitch)
				// clone->set_volume_db(
				// 		UtilityFunctions::linear_to_db(
				// 				UtilityFunctions::remap(instructions.volume, 0.0, 127.0, 0.0, 1.0)));
				// clone->set_autoplay(true);
				String channel_name = vformat("Channel%s", instructions.channel);
				AudioStreamPlayer *squatting_sound = sound_container->get_node<AudioStreamPlayer>(channel_name);

				if (squatting_sound != nullptr) {
					squatting_sound->stop();
					squatting_sound->set_stream(source->get_stream());
					squatting_sound->set_pitch_scale(
							UtilityFunctions::remap(instructions.pitch, -127, 127, 0, 2));
					squatting_sound->set_volume_db(
							UtilityFunctions::linear_to_db(
									UtilityFunctions::remap(instructions.volume, 0.0, 127.0, 0.0, 2.0)));
					squatting_sound->play();
				}

				// sound_container->add_child(clone);
				// clone->set_owner(get_tree()->get_edited_scene_root());
				// clone->set_name(channel_name);
				// clone->play();
			} break;

			case SOUND_INSTRUCTION_TYPE_EMPTY:
			case SOUND_INSTRUCTION_TYPE_PRECACHE_SOUND:
			default: {
				continue;
			}
		}
	}
	sound_instructions.clear();
}

void GDDoom::_ready() {
	img_texture.instantiate();
	set_texture(img_texture);
}

void GDDoom::_enter_tree() {
	// UtilityFunctions::print("_enter_tree()");
}

void GDDoom::_exit_tree() {
	if (enabled) {
		kill_doom();
	}

	// texture_rect->queue_free();
}

void GDDoom::_thread_func() {
	// UtilityFunctions::print("_thread_func START!");

	while (true) {
		// UtilityFunctions::print("Start thread loop");
		if (this->_exiting) {
			return;
		}

		// Send the tick signal
		// UtilityFunctions::print(vformat("Send SIGUSR1 signal to %s!", _spawn_pid));
		while (!this->_shm->init) {
			if (this->_exiting) {
				return;
			}
			// UtilityFunctions::print("thread: not init...");
			// OS::get_singleton()->delay_usec(10);
			OS::get_singleton()->delay_msec(1000);
		}
		// UtilityFunctions::print("thread detects shm is init!");
		kill(_spawn_pid, SIGUSR1);

		// OS::get_singleton()->delay_msec(1000);
		_shm->ticks_msec = Time::get_singleton()->get_ticks_msec();

		// // Let's wait for the shared memory to be ready
		while (!this->_shm->ready) {
			if (this->_exiting) {
				return;
			}
			OS::get_singleton()->delay_usec(10);
		}
		// UtilityFunctions::print("thread discovered that shm is ready!");
		if (this->_exiting) {
			return;
		}
		// The shared memory is ready
		// Screenbuffer
		memcpy(screen_buffer, _shm->screen_buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
		// Sounds
		for (int i = 0; i < _shm->sound_instructions_length; i++) {
			SoundInstructions instructions = _shm->sound_instructions[i];
			sound_instructions.append(instructions);
		}
		_shm->sound_instructions_length = 0;

		// Let's sleep the time Doom asks
		usleep(this->_shm->sleep_ms * 1000);

		// Reset the shared memory
		this->_shm->ready = false;
	}
}

void GDDoom::_init_shm() {
	_id = _last_id;
	_last_id += 1;

	const char *spawn_id = vformat("%s-%06d", GDDOOM_SPAWN_SHM_NAME, _id).utf8().get_data();
	strcpy(_shm_id, spawn_id);

	// UtilityFunctions::print(vformat("gddoom init_shm unlinking preemptively \"%s\"", _shm_id));
	shm_unlink(_shm_id);
	// UtilityFunctions::print(vformat("gddoom init_shm open shm \"%s\"", _shm_id));
	_shm_fd = shm_open(_shm_id, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (_shm_fd < 0) {
		UtilityFunctions::printerr(vformat("ERROR: %s", strerror(errno)));
		return;
	}

	ftruncate(_shm_fd, sizeof(SharedMemory));
	_shm = (SharedMemory *)mmap(0, sizeof(SharedMemory), PROT_WRITE, MAP_SHARED, _shm_fd, 0);
	if (_shm == nullptr) {
		UtilityFunctions::printerr(vformat("ERROR: %s", strerror(errno)));
		return;
	}
}

GDDoom::GDDoom() {
	_spawn_pid = 0;
}

GDDoom::~GDDoom() {
}

int GDDoom::_last_id = 0;
