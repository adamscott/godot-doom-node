#include "doom.h"

#include <signal.h>
#include <stdint.h>
#include <uchar.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "godot_cpp/classes/audio_server.hpp"
#include "godot_cpp/classes/audio_stream_generator.hpp"
#include "godot_cpp/classes/audio_stream_generator_playback.hpp"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/audio_stream_player2d.hpp"
#include "godot_cpp/classes/audio_stream_wav.hpp"
#include "godot_cpp/classes/camera2d.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/dir_access.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/hashing_context.hpp"
#include "godot_cpp/classes/image_texture.hpp"
#include "godot_cpp/classes/input.hpp"
#include "godot_cpp/classes/input_event.hpp"
#include "godot_cpp/classes/input_event_key.hpp"
#include "godot_cpp/classes/input_event_mouse_button.hpp"
#include "godot_cpp/classes/input_event_mouse_motion.hpp"
#include "godot_cpp/classes/node.hpp"
#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/scene_tree.hpp"
#include "godot_cpp/classes/sub_viewport.hpp"
#include "godot_cpp/classes/sub_viewport_container.hpp"
#include "godot_cpp/classes/texture_rect.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/classes/time.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/core/memory.hpp"
#include "godot_cpp/core/object.hpp"
#include "godot_cpp/core/property_info.hpp"
#include "godot_cpp/variant/callable.hpp"
#include "godot_cpp/variant/char_string.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/packed_vector2_array.hpp"
#include "godot_cpp/variant/string.hpp"
#include "godot_cpp/variant/typed_array.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/variant.hpp"

#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomtype.h"

#include "doommus2mid.h"
#include "doomswap.h"

extern "C" {
#include <err.h>
#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <unistd.h>
// Shared memory
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

// GodotDoomNode
#include "doomcommon.h"
#include "doommutex.h"
#include "doomshm.h"

// Fluidsynth
#include "fluidsynth.h"
#include "fluidsynth/audio.h"
#include "fluidsynth/midi.h"
#include "fluidsynth/misc.h"
#include "fluidsynth/settings.h"
#include "fluidsynth/synth.h"
#include "fluidsynth/types.h"
}

#ifndef SPAWN_EXECUTABLE_NAME
#define SPAWN_EXECUTABLE_NAME "godot-doom-spawn.linux.template_debug.x86_64"
#endif

#define SOUND_SUBVIEWPORT_SIZE 512

using namespace godot;

void DOOM::_bind_methods() {
	// Signals.
	ADD_SIGNAL(MethodInfo("assets_imported"));

	ClassDB::bind_method(D_METHOD("_doom_thread_func"), &DOOM::_doom_thread_func);
	ClassDB::bind_method(D_METHOD("_wad_thread_func"), &DOOM::_wad_thread_func);
	ClassDB::bind_method(D_METHOD("_sound_fetching_thread_func"), &DOOM::_sound_fetching_thread_func);
	ClassDB::bind_method(D_METHOD("_midi_fetching_thread_func"), &DOOM::_midi_fetching_thread_func);
	ClassDB::bind_method(D_METHOD("_midi_thread_func"), &DOOM::_midi_thread_func);
	ClassDB::bind_method(D_METHOD("_wad_thread_end"), &DOOM::_wad_thread_end);
	ClassDB::bind_method(D_METHOD("_sound_fetching_thread_end"), &DOOM::_sound_fetching_thread_end);
	ClassDB::bind_method(D_METHOD("_midi_fetching_thread_end"), &DOOM::_midi_fetching_thread_end);

	ClassDB::bind_method(D_METHOD("import_assets"), &DOOM::import_assets);

	ADD_GROUP("DOOM", "doom_");
	ADD_GROUP("Assets", "assets_");

	ClassDB::bind_method(D_METHOD("get_assets_ready"), &DOOM::get_assets_ready);
	ClassDB::bind_method(D_METHOD("set_assets_ready"), &DOOM::set_assets_ready);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "doom_assets_ready"), "set_assets_ready", "get_assets_ready");

	ClassDB::bind_method(D_METHOD("get_enabled"), &DOOM::get_enabled);
	ClassDB::bind_method(D_METHOD("set_enabled", "enabled"), &DOOM::set_enabled);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "doom_enabled"), "set_enabled", "get_enabled");

	ClassDB::bind_method(D_METHOD("get_import_assets"), &DOOM::get_import_assets);
	ClassDB::bind_method(D_METHOD("set_import_assets", "import"), &DOOM::set_import_assets);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "assets_import_assets"), "set_import_assets", "get_import_assets");

	ClassDB::bind_method(D_METHOD("get_wad_path"), &DOOM::get_wad_path);
	ClassDB::bind_method(D_METHOD("set_wad_path", "wad_path"), &DOOM::set_wad_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "assets_wad_path", PROPERTY_HINT_FILE, "*.wad"), "set_wad_path", "get_wad_path");

	ClassDB::bind_method(D_METHOD("get_soundfont_path"), &DOOM::get_soundfont_path);
	ClassDB::bind_method(D_METHOD("set_soundfont_path", "soundfont_path"), &DOOM::set_soundfont_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "assets_soundfont_path", PROPERTY_HINT_FILE, "*.sf2,*.sf3"), "set_soundfont_path", "get_soundfont_path");

	ClassDB::bind_method(D_METHOD("get_wasd_mode"), &DOOM::get_wasd_mode);
	ClassDB::bind_method(D_METHOD("set_wasd_mode", "wasd_mode_enabled"), &DOOM::set_wasd_mode);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wasd_mode"), "set_wasd_mode", "get_wasd_mode");

	ClassDB::bind_method(D_METHOD("get_mouse_acceleration"), &DOOM::get_mouse_acceleration);
	ClassDB::bind_method(D_METHOD("set_mouse_acceleration", "mouse_acceleration"), &DOOM::set_mouse_acceleration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mouse_acceleration"), "set_mouse_acceleration", "get_mouse_acceleration");
}

void DOOM::_ready() {
	_img_texture.instantiate();
	set_texture(_img_texture);
}

void DOOM::_enter_tree() {}

void DOOM::_exit_tree() {
	if (_enabled) {
		_kill_doom();
	}
}

void DOOM::_input(const Ref<InputEvent> &event) {
	if (_spawn_pid == 0 || _shm == nullptr) {
		return;
	}

	if (!has_focus()) {
		return;
	}

	Ref<InputEventKey> key = event;
	if (key.is_valid()) {
		if (key->is_echo()) {
			get_viewport()->set_input_as_handled();
			return;
		}

		Key key_pressed = key->get_physical_keycode();
		uint32_t pressed_flag = key->is_pressed() << 31;

		if (_wasd_mode) {
			switch (key_pressed) {
				case KEY_W: {
					key_pressed = KEY_UP;
				} break;

				case KEY_A: {
					key_pressed = KEY_COMMA;
				} break;

				case KEY_S: {
					key_pressed = KEY_DOWN;
				} break;

				case KEY_D: {
					key_pressed = KEY_PERIOD;
				} break;

				default: {
					// do nothing
				}
			}
		}

		for (int i = 0; i < _shm->keys_pressed_length; i++) {
			if (_shm->keys_pressed[i] == (pressed_flag | key_pressed)) {
				return;
			}
		}

		mutex_lock(_shm);
		_shm->keys_pressed[_shm->keys_pressed_length] = pressed_flag | key_pressed;
		_shm->keys_pressed_length += 1;
		mutex_unlock(_shm);

		if (key_pressed != KEY_ESCAPE) {
			get_viewport()->set_input_as_handled();
		}
		return;
	}

	Ref<InputEventMouseButton> mouse_button = event;
	if (mouse_button.is_valid()) {
		MouseButton mouse_button_index = mouse_button->get_button_index();
		uint32_t pressed_flag = mouse_button->is_pressed() << 31;

		mutex_lock(_shm);
		_shm->mouse_buttons_pressed[_shm->mouse_buttons_pressed_length] = pressed_flag | mouse_button_index;
		_shm->mouse_buttons_pressed_length += 1;
		mutex_unlock(_shm);
		return;
	}

	Ref<InputEventMouseMotion> mouse_motion = event;
	if (mouse_motion.is_valid()) {
		mutex_lock(_shm);
		_shm->mouse_x += mouse_motion->get_relative().x * _mouse_acceleration;
		// _shm->mouse_y += mouse_motion->get_relative().y * _mouse_acceleration;
		mutex_unlock(_shm);
		return;
	}
}

void DOOM::_gui_input(const Ref<InputEvent> &event) {
	if (_spawn_pid == 0 || _shm == nullptr) {
		return;
	}
}

bool DOOM::get_import_assets() {
	return false;
}

void DOOM::set_import_assets(bool p_import_assets) {
	call_deferred("import_assets");
}

bool DOOM::get_assets_ready() {
	return _assets_ready;
}

void DOOM::set_assets_ready(bool p_ready) {
}

bool DOOM::get_enabled() {
	return _enabled;
}

void DOOM::set_enabled(bool p_enabled) {
	_enabled = p_enabled;

	_update_doom();
}

String DOOM::get_wad_path() {
	return _wad_path;
}

void DOOM::set_wad_path(String p_wad_path) {
	_wad_path = p_wad_path;
}

String DOOM::get_soundfont_path() {
	return _soundfont_path;
}

void DOOM::set_soundfont_path(String p_soundfont_path) {
	_soundfont_path = p_soundfont_path;
	String global_path = ProjectSettings::get_singleton()->globalize_path(p_soundfont_path);
	const char *global_path_char = global_path.utf8().get_data();

	_mutex->lock();
	if (fluid_is_soundfont(global_path_char)) {
		_fluid_synth_id = fluid_synth_sfload(_fluid_synth, global_path_char, true);
	} else if (_fluid_synth_id != -1) {
		fluid_synth_sfunload(_fluid_synth, _fluid_synth_id, true);
		_fluid_synth_id = -1;
	}
	_mutex->unlock();
}

bool DOOM::get_wasd_mode() {
	return _wasd_mode;
}

void DOOM::set_wasd_mode(bool p_wasd_mode) {
	_wasd_mode = p_wasd_mode;
}

float DOOM::get_mouse_acceleration() {
	return _mouse_acceleration;
}

void DOOM::set_mouse_acceleration(float p_mouse_acceleration) {
	_mouse_acceleration = p_mouse_acceleration;
}

void DOOM::import_assets() {
	if (_wad_path.is_empty()) {
		UtilityFunctions::printerr(vformat("[Godot DOOM node] Wad path must not be empty."));
		return;
	}

	if (!FileAccess::file_exists(_wad_path)) {
		UtilityFunctions::printerr(vformat("[Godot DOOM node] Wad path (\"%s\") isn't a file.", _wad_path));
		return;
	}

	if (!_wad_thread.is_null() && _wad_thread->is_started()) {
		UtilityFunctions::printerr(vformat("[Godot DOOM node] A process is already importing assets."));
		return;
	}

	Ref<DirAccess> user_dir = DirAccess::open("user://");
	if (!user_dir->dir_exists("godot-doom")) {
		user_dir->make_dir("godot-doom");
	}

	_wad_thread.instantiate();
	Callable wad_func = Callable(this, "_wad_thread_func");
	_wad_thread->start(wad_func);
}

void DOOM::_update_doom() {
	if (_enabled && _assets_ready) {
		_init_doom();
	} else {
		_enabled = false;
		if (_spawn_pid > 0) {
			_kill_doom();
		}
	}
}

void DOOM::_init_doom() {
	_exiting = false;

	_init_shm();

	mutex_lock(_shm);
	_shm->ticks_msec = Time::get_singleton()->get_ticks_msec();
	mutex_unlock(_shm);

	_spawn_pid = _launch_doom_executable();

	_screen_buffer_array.resize(sizeof(_screen_buffer));

	_doom_thread.instantiate();
	_midi_thread.instantiate();

	Callable doom_func = Callable(this, "_doom_thread_func");
	_doom_thread->start(doom_func);
	Callable midi_func = Callable(this, "_midi_thread_func");
	_midi_thread->start(midi_func);
}

__pid_t DOOM::_launch_doom_executable() {
	String operating_system;
	String name = OS::get_singleton()->get_name();

	if (name == "Windows" || name == "UWP") {
		operating_system = "windows";
	} else if (name == "macOS") {
		operating_system = "macos";
	} else if (name == "Linux" || name == "FreeBSD" || name == "OpenBSD" || name == "BSD") {
		operating_system = "linux";
	}

	UtilityFunctions::print(vformat("res://addons/godot-doom-node/%s/%s", operating_system, SPAWN_EXECUTABLE_NAME));

	CharString path_cs = ProjectSettings::get_singleton()->globalize_path(vformat("res://addons/godot-doom-node/%s/%s", operating_system, SPAWN_EXECUTABLE_NAME)).utf8();
	CharString id_cs = vformat("%s", _shm_id).utf8();
	CharString wad_cs = ProjectSettings::get_singleton()->globalize_path(_wad_path).utf8();
	CharString config_dir_cs = ProjectSettings::get_singleton()->globalize_path(vformat("user://godot-doom/%s-%s/", _wad_path.get_file().get_basename(), _wad_hash)).utf8();

	char *args[] = {
		(char *)path_cs.get_data(),
		strdup("-id"),
		(char *)id_cs.get_data(),
		strdup("-iwad"),
		(char *)wad_cs.get_data(),
		strdup("-configdir"),
		(char *)config_dir_cs.get_data(),
		NULL
	};

	char *envp[] = {
		NULL
	};

	__pid_t pid = fork();
	if (pid == 0) {
		execve(args[0], args, envp);
		exit(0);
	}
	return pid;
}

void DOOM::_midi_thread_func() {
	while (true) {
		_mutex->lock();
		Vector<MusicInstruction> instructions = _music_instructions;
		_mutex->unlock();

		// Parse music instructions already, sooner the better
		for (MusicInstruction instruction : instructions) {
			switch (instruction.type) {
				case MUSIC_INSTRUCTION_TYPE_REGISTER_SONG: {
					UtilityFunctions::print("MUSIC_INSTRUCTION_TYPE_REGISTER_SONG");
					String sha1;
					String midi_file;

					for (int i = 0; i < sizeof(instruction.lump_sha1_hex); i += sizeof(instruction.lump_sha1_hex[0])) {
						sha1 += vformat("%c", instruction.lump_sha1_hex[i]);
					}

					Array keys = _wad_files.keys();
					for (int i = 0; i < keys.size(); i++) {
						String key = keys[i];
						if (!key.begins_with("D_")) {
							continue;
						}

						Dictionary info = _wad_files[key];
						if (sha1.to_lower() == String(info["sha1"]).to_lower()) {
							midi_file = key;
							break;
						}
					}

					if (midi_file.is_empty()) {
						break;
					}

					if (_stored_midi_files.has(midi_file)) {
						if (_fluid_player == nullptr) {
							_fluid_player = new_fluid_player(_fluid_synth);
						} else if (midi_file != _current_midi_file) {
							fluid_player_stop(_fluid_player);
							fluid_player_join(_fluid_player);
							delete_fluid_player(_fluid_player);
							fluid_synth_system_reset(_fluid_synth);
							_fluid_player = nullptr;

							OS::get_singleton()->delay_usec(100);

							_fluid_player = new_fluid_player(_fluid_synth);
						}

						_mutex->lock();
						_current_midi_file = midi_file;
						_mutex->unlock();

						PackedByteArray stored_midi_file = _stored_midi_files[midi_file];
						fluid_player_add_mem(_fluid_player, stored_midi_file.ptrw(), stored_midi_file.size());
					}
				} break;

				case MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG: {
					UtilityFunctions::print("MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG");
					_mutex->lock();
					_current_midi_path = "";
					_mutex->unlock();
				} break;

				case MUSIC_INSTRUCTION_TYPE_PLAY_SONG: {
					UtilityFunctions::print("MUSIC_INSTRUCTION_TYPE_PLAY_SONG");
					if (_fluid_player == nullptr) {
						_fluid_player = new_fluid_player(_fluid_synth);
						PackedByteArray stored_midi_file = _stored_midi_files[_current_midi_file];
						fluid_player_add_mem(_fluid_player, stored_midi_file.ptrw(), stored_midi_file.size());
					}

					_current_midi_looping = instruction.looping;

					fluid_player_seek(_fluid_player, 0);
					fluid_player_set_loop(_fluid_player, _current_midi_looping ? -1 : 0);
					fluid_player_play(_fluid_player);
				} break;

				case MUSIC_INSTRUCTION_TYPE_RESUME_SONG: {
					UtilityFunctions::print("MUSIC_INSTRUCTION_TYPE_RESUME_SONG");
					if (_fluid_player == nullptr) {
						_fluid_player = new_fluid_player(_fluid_synth);
						PackedByteArray stored_midi_file = _stored_midi_files[_current_midi_file];
						fluid_player_add_mem(_fluid_player, stored_midi_file.ptrw(), stored_midi_file.size());
					}

					fluid_player_seek(_fluid_player, _current_midi_tick);
					fluid_player_set_loop(_fluid_player, _current_midi_looping ? -1 : 0);
					fluid_player_play(_fluid_player);
				} break;

				case MUSIC_INSTRUCTION_TYPE_SHUTDOWN_MUSIC:
				case MUSIC_INSTRUCTION_TYPE_STOP_SONG: {
					UtilityFunctions::print("MUSIC_INSTRUCTION_TYPE_STOP_SONG");
					if (_fluid_player != nullptr) {
						fluid_player_stop(_fluid_player);
						fluid_player_join(_fluid_player);
						delete_fluid_player(_fluid_player);
						fluid_synth_system_reset(_fluid_synth);
						_fluid_player = nullptr;

						OS::get_singleton()->delay_usec(100);
					}
				} break;

				case MUSIC_INSTRUCTION_TYPE_PAUSE_SONG: {
					UtilityFunctions::print("MUSIC_INSTRUCTION_TYPE_PAUSE_SONG");
					if (_fluid_player != nullptr) {
						_current_midi_tick = fluid_player_get_current_tick(_fluid_player);

						fluid_player_stop(_fluid_player);
						fluid_player_join(_fluid_player);
						delete_fluid_player(_fluid_player);
						fluid_synth_system_reset(_fluid_synth);
						_fluid_player = nullptr;

						OS::get_singleton()->delay_usec(100);
					}
				} break;

				case MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME: {
					UtilityFunctions::print("MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME");
					// current_midi_volume = instruction.volume;
				} break;

				case MusicInstructionType::MUSIC_INSTRUCTION_TYPE_EMPTY:
				default: {
				}
			}
		}
		_mutex->lock();
		_music_instructions.clear();
		_mutex->unlock();

		_mutex->lock();
		if (_exiting) {
			_mutex->unlock();
			return;
		}
		_mutex->unlock();

		if (_fluid_player == nullptr) {
			continue;
		}

		switch (fluid_player_get_status(_fluid_player)) {
			case FLUID_PLAYER_PLAYING: {
				if (_current_midi_playback == nullptr) {
					continue;
				}

				uint64_t ticks = Time::get_singleton()->get_ticks_usec();
				uint64_t diff = ticks - _current_midi_last_tick_usec;
				_current_midi_last_tick_usec = ticks;

				uint32_t len_asked = _current_midi_stream->get_mix_rate() / 1000 * diff / 10;

				if (_current_midi_playback->get_frames_available() < len_asked) {
					len_asked = _current_midi_playback->get_frames_available();
				}

				if (len_asked <= 0) {
					continue;
				}

				float bufl[len_asked], bufr[len_asked];
				if (fluid_synth_write_float(_fluid_synth, len_asked, bufl, 0, 1, bufr, 0, 1) == FLUID_FAILED) {
					break;
				}

				PackedVector2Array frames;
				for (int i = 0; i < len_asked; i++) {
					frames.append(Vector2(bufl[i], bufr[i]));
				}

				_current_midi_playback->push_buffer(frames);
				frames.clear();
			} break;
		}

		OS::get_singleton()->delay_usec(10);
	}
}

void DOOM::_wad_thread_func() {
	_wad_files.clear();

	_mutex->lock();
	String local_wad_path = _wad_path;
	_mutex->unlock();

	Ref<FileAccess> wad = FileAccess::open(local_wad_path, FileAccess::ModeFlags::READ);
	if (wad.is_null()) {
		UtilityFunctions::printerr(vformat("Could not open \"%s\"", local_wad_path));
		return;
	}

	PackedByteArray sig_array = wad->get_buffer(sizeof(WadOriginalSignature));
	_wad_signature.signature = vformat("%c%c%c%c", sig_array[0], sig_array[1], sig_array[2], sig_array[3]);
	_wad_signature.number_of_files = sig_array.decode_s32(sizeof(WadOriginalSignature::sig));
	_wad_signature.fat_offset = sig_array.decode_s32(sizeof(WadOriginalSignature::sig) + sizeof(WadOriginalSignature::numFiles));

	wad->seek(_wad_signature.fat_offset);
	PackedByteArray wad_files = wad->get_buffer(sizeof(WadOriginalFileEntry) * _wad_signature.number_of_files);
	for (int i = 0; i < _wad_signature.number_of_files; i++) {
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

		if (file_array.size() > 0) {
			Ref<HashingContext> hashing_context;
			hashing_context.instantiate();
			hashing_context->start(HashingContext::HASH_SHA1);
			hashing_context->update(file_array);
			info["sha1"] = hashing_context->finish().hex_encode();
		} else {
			info["sha1"] = Variant();
		}

		_wad_files[file_entry.name] = info;
	}

	// Calculate hash
	Ref<HashingContext> hashing_context;
	hashing_context.instantiate();
	hashing_context->start(HashingContext::HASH_SHA256);
	wad->seek(0);
	while (!wad->eof_reached()) {
		hashing_context->update(wad->get_buffer(1024));
	}
	wad->close();
	PackedByteArray hash = hashing_context->finish();

	_mutex->lock();
	_wad_hash = hash.hex_encode();
	_mutex->unlock();

	call_deferred("_wad_thread_end");
}

void DOOM::_sound_fetching_thread_func() {
	_mutex->lock();
	Array keys = _wad_files.keys();
	_mutex->unlock();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("DS")) {
			continue;
		}
		_mutex->lock();
		Dictionary info = _wad_files[key];
		_mutex->unlock();
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

	call_deferred("_sound_fetching_thread_end");
}

void DOOM::_midi_fetching_thread_func() {
	if (!FileAccess::file_exists(_soundfont_path)) {
		return;
	}

	_mutex->lock();
	Array keys = _wad_files.keys();
	_mutex->unlock();

	// Rendering loop
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("D_")) {
			continue;
		}

		_mutex->lock();
		Dictionary info = _wad_files[key];
		_mutex->unlock();

		PackedByteArray file_array = info["data"];
		PackedByteArray midi_output;

		// Mus to midi conversion
		bool converted = !DOOMMus2Mid::get_singleton()->mus2mid(file_array, midi_output);
		if (!converted) {
			continue;
		}

		_stored_midi_files[key] = midi_output;
	}

	call_deferred("_midi_fetching_thread_end");
}

void DOOM::_append_sounds() {
	SubViewportContainer *sound_subviewportcontainer = (SubViewportContainer *)get_node_or_null("SoundSubViewportContainer");
	if (sound_subviewportcontainer != nullptr) {
		sound_subviewportcontainer->set_name("tobedeleted");
		sound_subviewportcontainer->queue_free();
	}
	sound_subviewportcontainer = memnew(SubViewportContainer);
	sound_subviewportcontainer->set_visible(false);
	sound_subviewportcontainer->set_name("SoundSubViewportContainer");
	call_deferred("add_child", sound_subviewportcontainer);
	// sound_subviewportcontainer->set_owner(get_tree()->get_edited_scene_root());

	SubViewport *sound_subviewport = (SubViewport *)get_node_or_null("SoundSubViewport");
	if (sound_subviewport != nullptr) {
		sound_subviewport->set_name("tobedeleted");
		sound_subviewport->queue_free();
	}
	sound_subviewport = memnew(SubViewport);
	sound_subviewport->set_as_audio_listener_2d(true);
	sound_subviewport->set_name("SoundSubViewport");
	sound_subviewport->set_size(Vector2(SOUND_SUBVIEWPORT_SIZE, SOUND_SUBVIEWPORT_SIZE));
	sound_subviewportcontainer->add_child(sound_subviewport);
	// sound_subviewport->set_owner(get_tree()->get_edited_scene_root());

	Camera2D *camera = memnew(Camera2D);
	camera->set_name("Camera2D");
	sound_subviewport->add_child(camera);
	// camera->set_owner(get_tree()->get_edited_scene_root());

	for (int i = 0; i < 16; i++) {
		AudioStreamPlayer2D *player = memnew(AudioStreamPlayer2D);
		player->set_position(Vector2(0, 0));
		sound_subviewport->add_child(player);
		player->set_name(vformat("Channel%s", i));
		player->set_bus("SFX");
		// player->set_owner(get_tree()->get_edited_scene_root());
	}

	Node *sound_container = get_node_or_null("SoundContainer");
	if (sound_container != nullptr) {
		sound_container->set_name("tobedeleted");
		sound_container->queue_free();
	}
	sound_container = memnew(Node);
	call_deferred("add_child", sound_container);
	sound_container->set_name("SoundContainer");
	// sound_container->set_owner(get_tree()->get_edited_scene_root());

	_mutex->lock();
	Array keys = _wad_files.keys();
	_mutex->unlock();

	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("DS")) {
			continue;
		}

		_mutex->lock();
		Dictionary info = _wad_files[key];
		_mutex->unlock();

		AudioStreamPlayer *player = reinterpret_cast<AudioStreamPlayer *>((Object *)info["player"]);
		sound_container->add_child(player);
		// player->set_owner(get_tree()->get_edited_scene_root());
	}
}

void DOOM::_append_music() {
	Node *music_container = get_node_or_null("MusicContainer");
	if (music_container != nullptr) {
		music_container->set_name("tobedeleted");
		music_container->queue_free();
	}
	music_container = memnew(Node);
	call_deferred("add_child", music_container);
	music_container->set_name("MusicContainer");
	// music_container->set_owner(get_tree()->get_edited_scene_root());

	AudioStreamPlayer *player = memnew(AudioStreamPlayer);
	music_container->add_child(player);
	player->set_name("Player");
	player->set_bus("Music");
	// player->set_owner(get_tree()->get_edited_scene_root());

	Ref<AudioStreamGenerator> stream;
	stream.instantiate();
	stream->set_buffer_length(0.05);
	_current_midi_stream = stream;
	player->set_stream(_current_midi_stream);
	player->call_deferred("play");
}

void DOOM::_wad_thread_end() {
	if (_wad_thread->is_alive()) {
		_wad_thread->wait_to_finish();
	}

	_start_sound_fetching();
	_start_midi_fetching();
}

void DOOM::_sound_fetching_thread_end() {
	if (sound_fetching_thread->is_alive()) {
		sound_fetching_thread->wait_to_finish();
	}

	_append_sounds();

	_sound_fetch_complete = true;
	_update_assets_status();
}

void DOOM::_midi_fetching_thread_end() {
	if (sound_fetching_thread->is_alive()) {
		sound_fetching_thread->wait_to_finish();
	}

	_append_music();

	_midi_fetch_complete = true;
	_update_assets_status();
}

void DOOM::_start_sound_fetching() {
	sound_fetching_thread.instantiate();
	Callable func = Callable(this, "_sound_fetching_thread_func");
	sound_fetching_thread->start(func);
}

void DOOM::_start_midi_fetching() {
	midi_fetching_thread.instantiate();
	Callable func = Callable(this, "_midi_fetching_thread_func");
	midi_fetching_thread->start(func);
}

void DOOM::_update_assets_status() {
	if (_sound_fetch_complete && _midi_fetch_complete) {
		_assets_ready = true;
		emit_signal("assets_imported");
	}
}

void DOOM::_kill_doom() {
	_exiting = true;
	if (!_doom_thread.is_null()) {
		_doom_thread->wait_to_finish();
	}

	if (!_wad_thread.is_null()) {
		_wad_thread->wait_to_finish();
	}

	int result = shm_unlink(_shm_id);
	if (result < 0) {
		UtilityFunctions::printerr(vformat("ERROR unlinking shm %s: %s", _shm_id, strerror(errno)));
	}

	if (_spawn_pid > 0) {
		kill(_spawn_pid, SIGKILL);
	}
	_spawn_pid = 0;
}

void DOOM::_process(double delta) {
	if (_spawn_pid == 0) {
		return;
	}

	_update_screen_buffer();
	_update_sounds();
	_update_music();
}

void DOOM::_update_screen_buffer() {
	memcpy(_screen_buffer_array.ptrw(), _screen_buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * RGBA);

	Ref<Image> image = Image::create_from_data(DOOMGENERIC_RESX, DOOMGENERIC_RESY, false, Image::Format::FORMAT_RGBA8, _screen_buffer_array);
	if (_img_texture->get_image().is_null()) {
		_img_texture->set_image(image);
	} else {
		_img_texture->update(image);
	}
}

void DOOM::_update_sounds() {
	Node *sound_container = get_node<Node>("SoundContainer");
	if (sound_container == nullptr) {
		return;
	}

	for (SoundInstruction instruction : _sound_instructions) {
		switch (instruction.type) {
			case SOUND_INSTRUCTION_TYPE_START_SOUND: {
				String name = vformat("DS%s", String(instruction.name).to_upper());
				AudioStreamPlayer *source = (AudioStreamPlayer *)sound_container->get_node_or_null(name);
				String channel_name = vformat("SoundSubViewportContainer/SoundSubViewport/Channel%s", instruction.channel);
				AudioStreamPlayer2D *channel = (AudioStreamPlayer2D *)get_node_or_null(channel_name);

				if (source == nullptr || channel == nullptr) {
					continue;
				}

				channel->stop();
				channel->set_stream(source->get_stream());
				// squatting_sound->set_pitch_scale(
				// 		UtilityFunctions::remap(instruction.pitch, 0, INT8_MAX, 0, 2));
				channel->set_volume_db(
						UtilityFunctions::linear_to_db(
								UtilityFunctions::remap(instruction.volume, 0.0, INT8_MAX, 0.0, 2.0)));

				double pan = UtilityFunctions::remap(instruction.sep, 0, INT8_MAX, -1, 1);
				channel->set_position(Vector2(
						(SOUND_SUBVIEWPORT_SIZE / 2.0) * pan,
						0));

				channel->play();
			} break;

			case SOUND_INSTRUCTION_TYPE_STOP_SOUND: {
				String channel_name = vformat("SoundSubViewportContainer/SoundSubViewport/Channel%s", instruction.channel);
				AudioStreamPlayer2D *channel = (AudioStreamPlayer2D *)get_node_or_null(channel_name);
				if (channel == nullptr) {
					continue;
				}

				channel->stop();
			} break;

			case SOUND_INSTRUCTION_TYPE_UPDATE_SOUND_PARAMS: {
				String channel_name = vformat("SoundSubViewportContainer/SoundSubViewport/Channel%s", instruction.channel);
				AudioStreamPlayer2D *channel = (AudioStreamPlayer2D *)get_node_or_null(channel_name);
				if (channel == nullptr) {
					continue;
				}

				channel->set_volume_db(
						UtilityFunctions::linear_to_db(
								UtilityFunctions::remap(instruction.volume, 0.0, INT8_MAX, 0, 2)));
				double pan = UtilityFunctions::remap(instruction.sep, 0, INT8_MAX, -1, 1);
				channel->set_position(Vector2(
						(SOUND_SUBVIEWPORT_SIZE / 2.0) * pan,
						0));
			} break;

			case SOUND_INSTRUCTION_TYPE_SHUTDOWN_SOUND: {
				SubViewport *sound_subviewport = (SubViewport *)get_node_or_null("SoundSubViewportContainer/SoundSubViewport");
				if (sound_subviewport == nullptr) {
					continue;
				}

				TypedArray<Node> children = sound_subviewport->get_children();
				for (int i = 0; i < children.size(); i++) {
					AudioStreamPlayer2D *channel = (AudioStreamPlayer2D *)(Object *)children[i];
					if (channel == nullptr) {
						continue;
					}

					channel->stop();
				}
			} break;

			case SOUND_INSTRUCTION_TYPE_EMPTY:
			case SOUND_INSTRUCTION_TYPE_PRECACHE_SOUND:
			default: {
				continue;
			}
		}
	}
	_sound_instructions.clear();
}

void DOOM::_update_music() {
	AudioStreamPlayer *player = (AudioStreamPlayer *)get_node_or_null("MusicContainer/Player");
	if (player == nullptr) {
		return;
	}

	if (!player->is_playing()) {
		player->play();
	}
	Ref<AudioStreamGeneratorPlayback> playback = player->get_stream_playback();
	if (playback.is_null()) {
		return;
	}
	_current_midi_playback = playback;
}

void DOOM::_doom_thread_func() {
	while (true) {
		if (_exiting) {
			return;
		}

		// Send the tick signal
		while (!_shm->init) {
			if (_exiting) {
				return;
			}
			OS::get_singleton()->delay_msec(10);
		}
		kill(_spawn_pid, SIGUSR1);

		mutex_lock(_shm);
		_shm->ticks_msec = Time::get_singleton()->get_ticks_msec();
		mutex_unlock(_shm);

		// // Let's wait for the shared memory to be ready
		while (!_shm->ready) {
			if (_exiting) {
				return;
			}
			OS::get_singleton()->delay_msec(10);
		}

		if (_exiting) {
			return;
		}
		// The shared memory is ready
		// Screenbuffer
		mutex_lock(_shm);
		memcpy(_screen_buffer, _shm->screen_buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
		mutex_unlock(_shm);

		// Sounds
		for (int i = 0; i < _shm->sound_instructions_length; i++) {
			mutex_lock(_shm);
			SoundInstruction instruction = _shm->sound_instructions[i];
			mutex_unlock(_shm);
			_sound_instructions.append(instruction);
		}
		mutex_lock(_shm);
		_shm->sound_instructions_length = 0;
		mutex_unlock(_shm);

		// Music
		for (int i = 0; i < _shm->music_instructions_length; i++) {
			mutex_lock(_shm);
			MusicInstruction instruction = _shm->music_instructions[i];
			mutex_unlock(_shm);
			_music_instructions.append(instruction);
		}
		mutex_lock(_shm);
		_shm->music_instructions_length = 0;
		mutex_unlock(_shm);

		// Let's sleep the time Doom asks
		OS::get_singleton()->delay_usec(_shm->sleep_ms * 1000);

		// Reset the shared memory
		mutex_lock(_shm);
		_shm->ready = false;
		mutex_unlock(_shm);

		if (_exiting) {
			return;
		}
	}
}

void DOOM::_init_shm() {
	_doom_instance_id = _last_doom_instance_id;
	_last_doom_instance_id += 1;

	const char *spawn_id = vformat("%s-%06d", GODOT_DOOM_SHM_NAME, _doom_instance_id).utf8().get_data();
	strcpy(_shm_id, spawn_id);

	shm_unlink(_shm_id);
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

DOOM::DOOM() {
	_spawn_pid = 0;
	_mutex.instantiate();

	_fluid_settings = new_fluid_settings();
	fluid_settings_setstr(_fluid_settings, "player.timing-source", "system");
	_fluid_synth = new_fluid_synth(_fluid_settings);
	fluid_synth_set_gain(_fluid_synth, 0.8f);
	_fluid_player = new_fluid_player(_fluid_synth);
}

DOOM::~DOOM() {
	delete_fluid_player(_fluid_player);
	delete_fluid_synth(_fluid_synth);
	delete_fluid_settings(_fluid_settings);
}

int DOOM::_last_doom_instance_id = 0;
