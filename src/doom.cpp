#include "doom.h"

#include <stdint.h>
#include <cstdint>
#include <cstring>

#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/audio_stream_generator.hpp>
#include <godot_cpp/classes/audio_stream_generator_playback.hpp>
#include <godot_cpp/classes/audio_stream_player.hpp>
#include <godot_cpp/classes/audio_stream_player2d.hpp>
#include <godot_cpp/classes/audio_stream_wav.hpp>
#include <godot_cpp/classes/camera2d.hpp>
#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <godot_cpp/classes/hashing_context.hpp>
#include <godot_cpp/classes/image_texture.hpp>
#include <godot_cpp/classes/input.hpp>
#include <godot_cpp/classes/input_event.hpp>
#include <godot_cpp/classes/input_event_key.hpp>
#include <godot_cpp/classes/input_event_mouse_button.hpp>
#include <godot_cpp/classes/input_event_mouse_motion.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/sub_viewport.hpp>
#include <godot_cpp/classes/sub_viewport_container.hpp>
#include <godot_cpp/classes/texture_rect.hpp>
#include <godot_cpp/classes/thread.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/core/property_info.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/char_string.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

#include "doomcommon.h"
#include "doominstance.h"
#include "doommus2mid.h"

extern "C" {
// doomgeneric
#include "doomgeneric/doomgeneric.h"

// Fluidsynth
#include <fluidsynth/log.h>
#include <fluidsynth/midi.h>
#include <fluidsynth/misc.h>
#include <fluidsynth/settings.h>
#include <fluidsynth/synth.h>

// Fluidsynth logger
#include "doomfluidsynthlogger.h"
}

#define SOUND_SUBVIEWPORT_SIZE 512

namespace godot {

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
	ClassDB::bind_method(D_METHOD("pause"), &DOOM::pause);
	ClassDB::bind_method(D_METHOD("resume"), &DOOM::resume);

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

	ClassDB::bind_method(D_METHOD("get_sound_bus"), &DOOM::get_sound_bus);
	ClassDB::bind_method(D_METHOD("set_sound_bus", "sound_bus"), &DOOM::set_sound_bus);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "sound_bus"), "set_sound_bus", "get_sound_bus");

	ClassDB::bind_method(D_METHOD("get_music_bus"), &DOOM::get_music_bus);
	ClassDB::bind_method(D_METHOD("set_music_bus", "music_bus"), &DOOM::set_music_bus);
	ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "music_bus"), "set_music_bus", "get_music_bus");

	ClassDB::bind_method(D_METHOD("get_wasd_mode"), &DOOM::get_wasd_mode);
	ClassDB::bind_method(D_METHOD("set_wasd_mode", "wasd_mode_enabled"), &DOOM::set_wasd_mode);
	ADD_PROPERTY(PropertyInfo(Variant::BOOL, "wasd_mode"), "set_wasd_mode", "get_wasd_mode");

	ClassDB::bind_method(D_METHOD("get_mouse_acceleration"), &DOOM::get_mouse_acceleration);
	ClassDB::bind_method(D_METHOD("set_mouse_acceleration", "mouse_acceleration"), &DOOM::set_mouse_acceleration);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "mouse_acceleration"), "set_mouse_acceleration", "get_mouse_acceleration");

	ClassDB::bind_method(D_METHOD("get_autosave"), &DOOM::get_autosave);
	ClassDB::bind_method(D_METHOD("set_autosave", "autosave"), &DOOM::set_autosave);
	ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "autosave"), "set_autosave", "get_autosave");
}

void DOOM::_input(const Ref<InputEvent> &event) {
	if (!has_focus()) {
		return;
	}

	Ref<InputEventKey> key = event;
	if (key.is_valid()) {
		if (key->is_echo()) {
			get_viewport()->set_input_as_handled();
			return;
		}

		Key active_key = key->get_physical_keycode();
		if (_wasd_mode) {
			switch (active_key) {
				case KEY_W: {
					active_key = KEY_UP;
				} break;

				case KEY_A: {
					active_key = KEY_COMMA;
				} break;

				case KEY_S: {
					active_key = KEY_DOWN;
				} break;

				case KEY_D: {
					active_key = KEY_PERIOD;
				} break;

				default: {
					// do nothing
				}
			}
		}

		if (key->is_pressed() && !_keys_pressed_queue.has(active_key)) {
			_keys_pressed_queue.append(active_key);
		} else if (key->is_released() && !_keys_released_queue.has(active_key)) {
			_keys_released_queue.append(active_key);
		}

		if (active_key != KEY_ESCAPE) {
			get_viewport()->set_input_as_handled();
			Input::get_singleton()->set_mouse_mode(Input::MouseMode::MOUSE_MODE_CAPTURED);
		}
		return;
	}

	Ref<InputEventMouseButton> mouse_button = event;
	if (mouse_button.is_valid()) {
		MouseButton mouse_button_index = mouse_button->get_button_index();

		if (mouse_button->is_pressed() && !_mouse_buttons_pressed_queue.has(mouse_button_index)) {
			_mouse_buttons_pressed_queue.append(mouse_button_index);
		} else if (mouse_button->is_released() && !_mouse_buttons_released_queue.has(mouse_button_index)) {
			_mouse_buttons_released_queue.append(mouse_button_index);
		}
		return;
	}

	Ref<InputEventMouseMotion> mouse_motion = event;
	if (mouse_motion.is_valid()) {
		_doom_instance->mouse_x += mouse_motion->get_relative().x * _mouse_acceleration;
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
	if (p_enabled == _enabled) {
		return;
	}

	_enabled = p_enabled;

	_update_doom();
}

String DOOM::get_wad_path() {
	return _wad_path;
}

void DOOM::set_wad_path(String p_wad_path) {
	if (_wad_path == p_wad_path) {
		return;
	}
	bool wad_path_is_empty = _wad_path.is_empty();

	_wad_path = p_wad_path;
	if (!wad_path_is_empty) {
		_kill_doom();
	}

	_assets_ready = false;
	_enabled = false;
}

String DOOM::get_soundfont_path() {
	return _soundfont_path;
}

void DOOM::set_soundfont_path(String p_soundfont_path) {
	_soundfont_path = p_soundfont_path;
	String soundfont_global_path = ProjectSettings::get_singleton()->globalize_path(p_soundfont_path);
	CharString soundfont_global_path_cs = soundfont_global_path.utf8();
	const char *soundfont_global_path_char = soundfont_global_path_cs.ptr();
	UtilityFunctions::print_verbose(vformat("Loading soundfont: %s", soundfont_global_path_char));

	if (fluid_is_soundfont(soundfont_global_path_char)) {
		_fluid_synth_id = fluid_synth_sfload(_fluid_synth, soundfont_global_path_char, true);
	} else if (_fluid_synth_id != -1) {
		fluid_synth_sfunload(_fluid_synth, _fluid_synth_id, true);
		_fluid_synth_id = -1;
	}

	if (!soundfont_global_path.is_empty() && _fluid_synth_id == -1) {
		UtilityFunctions::printerr(vformat("Could not load soundfont \"%s\"", soundfont_global_path));
	}
}

StringName DOOM::get_sound_bus() {
	return _sound_bus;
}

void DOOM::set_sound_bus(StringName p_sound_bus) {
	_sound_bus = p_sound_bus;

	SubViewport *subviewport = Object::cast_to<SubViewport>(get_node_or_null("\%SoundSubViewport"));
	if (subviewport == nullptr) {
		return;
	}
	TypedArray<Node> children = subviewport->get_children();
	for (int i = 0; i < children.size(); i++) {
		AudioStreamPlayer2D *player = Object::cast_to<AudioStreamPlayer2D>(children[i]);
		if (player == nullptr) {
			continue;
		}
		player->set_bus(_sound_bus);
	}
}

StringName DOOM::get_music_bus() {
	return _music_bus;
}

void DOOM::set_music_bus(StringName p_music_bus) {
	_music_bus = p_music_bus;

	AudioStreamPlayer *player = Object::cast_to<AudioStreamPlayer>(get_node_or_null("\%Player"));
	if (player == nullptr) {
		return;
	}
	player->set_bus(p_music_bus);
}

bool DOOM::get_autosave() {
	return _autosave;
}

void DOOM::set_autosave(bool p_autosave) {
	_autosave = p_autosave;
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

	if (_wad_thread.is_valid() && _wad_thread->is_alive()) {
		UtilityFunctions::printerr(vformat("[Godot DOOM node] A process is already importing assets."));
		return;
	}

	Ref<DirAccess> user_dir = DirAccess::open("user://");
	if (!user_dir->dir_exists("godot-doom-node")) {
		user_dir->make_dir("godot-doom-node");
	}

	if (_wad_thread.is_null()) {
		_wad_thread.instantiate();
	}
	Callable wad_func = Callable(this, "_wad_thread_func");
	_wad_thread->start(wad_func);
}

void DOOM::pause() {
	_stop_doom();
}

void DOOM::resume() {
	if (!_current_midi_file.is_empty()) {
		Ref<DOOMMusicInstruction> inst;
		inst.instantiate();
		inst->type = DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_RESUME_SONG;
		_music_instructions.append(inst);
	}

	_start_threads();
}

void DOOM::_stop_music() {
	if (_fluid_player != nullptr) {
		fluid_player_stop(_fluid_player);
		fluid_player_join(_fluid_player);
		delete_fluid_player(_fluid_player);
		_fluid_player = nullptr;
	}

	if (_fluid_synth != nullptr) {
		fluid_synth_system_reset(_fluid_synth);
	}
}

void DOOM::_update_doom() {
	if (_enabled && _assets_ready) {
		_init_doom();
	} else {
		_kill_doom();
	}
}

void DOOM::_init_doom() {
	_enabled = true;

	_screen_buffer.resize(SCREEN_BUFFER_SIZE);

	_start_threads();
}

void DOOM::_start_threads() {
	_exiting = false;

	if (_doom_thread.is_null()) {
		_doom_thread.instantiate();
	} else if (_doom_thread->is_started()) {
		_doom_thread->wait_to_finish();
	}
	if (_midi_thread.is_null()) {
		_midi_thread.instantiate();
	} else if (_midi_thread->is_started()) {
		_midi_thread->wait_to_finish();
	}

	Callable doom_func = Callable(this, "_doom_thread_func");
	_doom_thread->start(doom_func);
	Callable midi_func = Callable(this, "_midi_thread_func");
	_midi_thread->start(midi_func);
}

void DOOM::_doom_thread_func() {
	String wad = ProjectSettings::get_singleton()->globalize_path(_wad_path);
	String config_dir = ProjectSettings::get_singleton()->globalize_path(vformat("user://godot-doom/%s-%s/", _wad_path.get_file().get_basename(), _wad_hash));

	_doom_instance = DOOMInstance::get_singleton();

	UtilityFunctions::print(vformat("wad: %s, config_dir: %s", wad, config_dir));

	_doom_instance->wad = wad;
	_doom_instance->config_dir = config_dir;
	_doom_instance->ticks_msec = Time::get_singleton()->get_ticks_msec();
	_doom_instance->create();

	while (true) {
		if (_exiting || !_enabled) {
			return;
		}

		_doom_instance->ticks_msec = Time::get_singleton()->get_ticks_msec();
		_doom_instance->tick();

		if (_exiting || !_enabled) {
			return;
		}

		// Screenbuffer
		memcpy(_screen_buffer.ptrw(), _doom_instance->screen_buffer.ptrw(), SCREEN_BUFFER_SIZE);

		// Sounds
		_sound_instructions_mutex->lock();
		_sound_instructions.append_array(_doom_instance->sound_instructions);
		_doom_instance->sound_instructions.clear();
		_sound_instructions_mutex->unlock();

		// Music
		_music_instructions_mutex->lock();
		_music_instructions.append_array(_doom_instance->music_instructions);
		_doom_instance->music_instructions.clear();
		_music_instructions_mutex->unlock();

		// Let's sleep the time Doom asks
		OS::get_singleton()->delay_msec(_doom_instance->sleep_ms);

		_update_input();
	}
}

void DOOM::_midi_thread_func() {
	while (true) {
		if (_exiting) {
			return;
		}

		// Parse music instructions already, sooner the better

		_music_instructions_mutex->lock();
		for (Ref<DOOMMusicInstruction> &instruction : _music_instructions) {
			switch (instruction->type) {
				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_REGISTER_SONG: {
					String sha1 = instruction->lump_sha1_hex;
					String midi_file;

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
							_stop_music();
							OS::get_singleton()->delay_usec(1000);

							_fluid_player = new_fluid_player(_fluid_synth);
						}

						_current_midi_file = midi_file;

						PackedByteArray stored_midi_file = _stored_midi_files[midi_file];
						fluid_player_add_mem(_fluid_player, stored_midi_file.ptrw(), stored_midi_file.size());
					}
				} break;

				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG: {
					_current_midi_path = "";

				} break;

				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_PLAY_SONG: {
					if (_fluid_player == nullptr) {
						_fluid_player = new_fluid_player(_fluid_synth);
						PackedByteArray stored_midi_file = _stored_midi_files[_current_midi_file];
						fluid_player_add_mem(_fluid_player, stored_midi_file.ptrw(), stored_midi_file.size());
					}

					_current_midi_looping = instruction->looping;

					fluid_player_seek(_fluid_player, 0);
					fluid_player_set_loop(_fluid_player, _current_midi_looping ? -1 : 0);
					fluid_player_play(_fluid_player);
				} break;

				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_RESUME_SONG: {
					if (_fluid_player == nullptr) {
						_fluid_player = new_fluid_player(_fluid_synth);
						PackedByteArray stored_midi_file = _stored_midi_files[_current_midi_file];
						fluid_player_add_mem(_fluid_player, stored_midi_file.ptrw(), stored_midi_file.size());
					}

					fluid_player_seek(_fluid_player, _current_midi_tick);
					fluid_player_set_loop(_fluid_player, _current_midi_looping ? -1 : 0);
					fluid_player_play(_fluid_player);
				} break;

				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_SHUTDOWN_MUSIC:
				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_STOP_SONG: {
					_stop_music();
					OS::get_singleton()->delay_usec(100);
				} break;

				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_PAUSE_SONG: {
					if (_fluid_player != nullptr) {
						_current_midi_tick = fluid_player_get_current_tick(_fluid_player);
					}

					_stop_music();
					OS::get_singleton()->delay_usec(100);
				} break;

				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME: {
					// current_midi_volume = instruction->volume;
				} break;

				case DOOMMusicInstruction::MUSIC_INSTRUCTION_TYPE_EMPTY:
				default: {
				}
			}
		}
		_music_instructions.clear();
		_music_instructions_mutex->unlock();

		if (_exiting) {
			return;
		}

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

				float *bufl = (float *)memalloc(len_asked * sizeof(float));
				if (bufl == nullptr) {
					break;
				}
				float *bufr = (float *)memalloc(len_asked * sizeof(float));
				if (bufr == nullptr) {
					memfree(bufl);
					break;
				}

				if (fluid_synth_write_float(_fluid_synth, len_asked, bufl, 0, 1, bufr, 0, 1) == FLUID_FAILED) {
					memfree(bufl);
					bufl = nullptr;
					memfree(bufr);
					bufr = nullptr;
					break;
				}
				PackedVector2Array frames;
				for (int i = 0; i < len_asked; i++) {
					frames.append(Vector2(bufl[i], bufr[i]));
				}

				_current_midi_playback->push_buffer(frames);
				frames.clear();

				memfree(bufl);
				bufl = nullptr;
				memfree(bufr);
				bufr = nullptr;

				_current_midi_tick = fluid_player_get_current_tick(_fluid_player);
			} break;
		}

		OS::get_singleton()->delay_usec(10);
	}
}

void DOOM::_wad_thread_func() {
	_wad_files.clear();

	String local_wad_path = _wad_path;

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

	_wad_hash = hash.hex_encode();

	call_deferred("_wad_thread_end");
}

void DOOM::_sound_fetching_thread_func() {
	Array keys = _wad_files.keys();

	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("DS")) {
			continue;
		}

		Dictionary info = _wad_files[key];

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

	Array keys = _wad_files.keys();

	// Rendering loop
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("D_")) {
			continue;
		}

		Dictionary info = _wad_files[key];

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
	SubViewportContainer *sound_subviewportcontainer = Object::cast_to<SubViewportContainer>(get_node_or_null("SoundSubViewportContainer"));
	if (sound_subviewportcontainer != nullptr) {
		sound_subviewportcontainer->set_name("tobedeleted");
		sound_subviewportcontainer->queue_free();
	}
	sound_subviewportcontainer = memnew(SubViewportContainer);
	sound_subviewportcontainer->set_visible(false);
	sound_subviewportcontainer->set_name("SoundSubViewportContainer");
	call_deferred("add_child", sound_subviewportcontainer);
	sound_subviewportcontainer->call_deferred("set_owner", this);

	SubViewport *sound_subviewport = Object::cast_to<SubViewport>(get_node_or_null("SoundSubViewport"));
	if (sound_subviewport != nullptr) {
		sound_subviewport->set_name("tobedeleted");
		sound_subviewport->queue_free();
	}
	sound_subviewport = memnew(SubViewport);
	sound_subviewport->set_as_audio_listener_2d(true);
	sound_subviewport->set_name("SoundSubViewport");
	sound_subviewport->set_size(Vector2(SOUND_SUBVIEWPORT_SIZE, SOUND_SUBVIEWPORT_SIZE));
	sound_subviewport->set_unique_name_in_owner(true);
	sound_subviewportcontainer->add_child(sound_subviewport);
	sound_subviewport->call_deferred("set_owner", this);

	Camera2D *camera = memnew(Camera2D);
	camera->set_name("Camera2D");
	sound_subviewport->add_child(camera);
	camera->call_deferred("set_owner", this);

	for (int i = 0; i < 16; i++) {
		AudioStreamPlayer2D *player = memnew(AudioStreamPlayer2D);
		player->set_position(Vector2(0, 0));
		sound_subviewport->add_child(player);
		player->set_name(vformat("Channel%s", i));
		player->set_bus(_sound_bus);
		player->call_deferred("set_owner", this);
	}

	Node *sound_container = get_node_or_null("SoundContainer");
	if (sound_container != nullptr) {
		sound_container->set_name("tobedeleted");
		sound_container->queue_free();
	}
	sound_container = memnew(Node);
	call_deferred("add_child", sound_container);
	sound_container->set_name("SoundContainer");
	sound_container->call_deferred("set_owner", this);

	Array keys = _wad_files.keys();

	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("DS")) {
			continue;
		}

		Dictionary info = _wad_files[key];

		AudioStreamPlayer *player = Object::cast_to<AudioStreamPlayer>(info["player"]);
		sound_container->add_child(player);
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
	music_container->call_deferred("set_owner", this);

	AudioStreamPlayer *player = memnew(AudioStreamPlayer);
	music_container->add_child(player);
	player->set_name("Player");
	player->set_unique_name_in_owner(true);
	player->set_bus(_music_bus);
	player->call_deferred("set_owner", this);

	Ref<AudioStreamGenerator> stream;
	stream.instantiate();
	stream->set_buffer_length(0.05);
	_current_midi_stream = stream;
	player->set_stream(_current_midi_stream);
	player->call_deferred("play");
}

void DOOM::_wad_thread_end() {
	if (_wad_thread.is_valid() && _wad_thread->is_started()) {
		_wad_thread->wait_to_finish();
	}

	_sound_fetch_complete = false;
	_midi_fetch_complete = false;

	_start_sound_fetching();
	_start_midi_fetching();
}

void DOOM::_sound_fetching_thread_end() {
	if (_sound_fetching_thread.is_valid() && _sound_fetching_thread->is_started()) {
		_sound_fetching_thread->wait_to_finish();
	}

	_append_sounds();

	_sound_fetch_complete = true;
	_update_assets_status();
}

void DOOM::_midi_fetching_thread_end() {
	if (_midi_fetching_thread.is_valid() && _midi_fetching_thread->is_started()) {
		_midi_fetching_thread->wait_to_finish();
	}

	_append_music();

	_midi_fetch_complete = true;
	_update_assets_status();
}

void DOOM::_start_sound_fetching() {
	if (_sound_fetching_thread.is_null()) {
		_sound_fetching_thread.instantiate();
	} else if (_sound_fetching_thread->is_started()) {
		_sound_fetching_thread->wait_to_finish();
	}
	Callable func = Callable(this, "_sound_fetching_thread_func");
	_sound_fetching_thread->start(func);
}

void DOOM::_start_midi_fetching() {
	if (_midi_fetching_thread.is_null()) {
		_midi_fetching_thread.instantiate();
	} else if (_midi_fetching_thread->is_started()) {
		_midi_fetching_thread->wait_to_finish();
	}
	Callable func = Callable(this, "_midi_fetching_thread_func");
	_midi_fetching_thread->start(func);
}

void DOOM::_update_assets_status() {
	if (_sound_fetch_complete && _midi_fetch_complete) {
		_assets_ready = true;
		emit_signal("assets_imported");
	}
}

void DOOM::_wait_for_threads() {
	if (_doom_thread.is_valid() && _doom_thread->is_started()) {
		_doom_thread->wait_to_finish();
	}

	if (_wad_thread.is_valid() && _wad_thread->is_started()) {
		_wad_thread->wait_to_finish();
	}

	if (_midi_fetching_thread.is_valid() && _midi_fetching_thread->is_started()) {
		_midi_fetching_thread->wait_to_finish();
	}

	if (_sound_fetching_thread.is_valid() && _sound_fetching_thread->is_started()) {
		_sound_fetching_thread->wait_to_finish();
	}

	if (_midi_thread.is_valid() && _midi_thread->is_started()) {
		_midi_thread->wait_to_finish();
	}
}

void DOOM::_kill_doom() {
	_enabled = false;
	_doom_instance = nullptr;

	OS::get_singleton()->delay_msec(250);

	if (_fluid_player != nullptr) {
		fluid_player_stop(_fluid_player);
		fluid_player_join(_fluid_player);
		delete_fluid_player(_fluid_player);
		_fluid_player = nullptr;
	}
	if (_fluid_synth != nullptr) {
		fluid_synth_system_reset(_fluid_synth);
	}

	_exiting = true;
	_wait_for_threads();
}

void DOOM::_stop_doom() {
	_stop_music();

	_exiting = true;

	_wait_for_threads();
}

void DOOM::_update_screen_buffer() {
	_screen_buffer_mutex->lock();
	Ref<Image> image = Image::create_from_data(DOOMGENERIC_RESX, DOOMGENERIC_RESY, false, Image::Format::FORMAT_RGBA8, _screen_buffer);
	_screen_buffer_mutex->unlock();
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

	_sound_instructions_mutex->lock();
	for (Ref<DOOMSoundInstruction> &instruction : _sound_instructions) {
		switch (instruction->type) {
			case DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_START_SOUND: {
				String name = vformat("DS%s", instruction->name.to_upper());
				AudioStreamPlayer *source = (AudioStreamPlayer *)sound_container->get_node_or_null(name);
				String channel_name = vformat("SoundSubViewportContainer/SoundSubViewport/Channel%s", instruction->channel);
				AudioStreamPlayer2D *channel = (AudioStreamPlayer2D *)get_node_or_null(channel_name);

				if (source == nullptr || channel == nullptr) {
					continue;
				}

				channel->stop();
				channel->set_stream(source->get_stream());
				// squatting_sound->set_pitch_scale(
				// 		UtilityFunctions::remap(instruction->pitch, 0, INT8_MAX, 0, 2));
				channel->set_volume_db(
						UtilityFunctions::linear_to_db(
								UtilityFunctions::remap(instruction->volume, 0.0, INT8_MAX, 0.0, 2.0)));

				double pan = UtilityFunctions::remap(instruction->sep, 0, INT8_MAX, -1, 1);
				channel->set_position(Vector2(
						(SOUND_SUBVIEWPORT_SIZE / 2.0) * pan,
						0));

				channel->play();
			} break;

			case DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_STOP_SOUND: {
				String channel_name = vformat("SoundSubViewportContainer/SoundSubViewport/Channel%s", instruction->channel);
				AudioStreamPlayer2D *channel = (AudioStreamPlayer2D *)get_node_or_null(channel_name);
				if (channel == nullptr) {
					continue;
				}

				channel->stop();
			} break;

			case DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_UPDATE_SOUND_PARAMS: {
				String channel_name = vformat("SoundSubViewportContainer/SoundSubViewport/Channel%s", instruction->channel);
				AudioStreamPlayer2D *channel = (AudioStreamPlayer2D *)get_node_or_null(channel_name);
				if (channel == nullptr) {
					continue;
				}

				channel->set_volume_db(
						UtilityFunctions::linear_to_db(
								UtilityFunctions::remap(instruction->volume, 0.0, INT8_MAX, 0, 2)));
				double pan = UtilityFunctions::remap(instruction->sep, 0, INT8_MAX, -1, 1);
				channel->set_position(Vector2(
						(SOUND_SUBVIEWPORT_SIZE / 2.0) * pan,
						0));
			} break;

			case DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_SHUTDOWN_SOUND: {
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

			case DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_EMPTY:
			case DOOMSoundInstruction::SOUND_INSTRUCTION_TYPE_PRECACHE_SOUND:
			default: {
				continue;
			}
		}
	}
	_sound_instructions.clear();
	_sound_instructions_mutex->unlock();
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

void DOOM::_update_input() {
	if (_doom_instance == nullptr) {
		return;
	}

	for (Key key_pressed : _keys_pressed_queue) {
		if (!_keys_pressed.has(key_pressed)) {
			_keys_pressed.append(key_pressed);
		}

		_doom_instance->keys_pressed[_doom_instance->keys_pressed_length] = key_pressed | (1 << 31);
		_doom_instance->keys_pressed_length += 1;
	}
	_keys_pressed_queue.clear();

	for (Key key_released : _keys_released_queue) {
		if (_keys_pressed.has(key_released)) {
			_keys_pressed.erase(key_released);
		}

		_doom_instance->keys_pressed[_doom_instance->keys_pressed_length] = key_released;
		_doom_instance->keys_pressed_length += 1;
	}
	_keys_released_queue.clear();

	for (MouseButton mouse_button_pressed : _mouse_buttons_pressed_queue) {
		if (!_mouse_buttons_pressed.has(mouse_button_pressed)) {
			_mouse_buttons_pressed.append(mouse_button_pressed);
		}

		_doom_instance->mouse_buttons_pressed[_doom_instance->mouse_buttons_pressed_length] = mouse_button_pressed | (1 << 31);
		_doom_instance->mouse_buttons_pressed_length += 1;
	}
	_mouse_buttons_pressed_queue.clear();

	for (MouseButton mouse_button_released : _mouse_buttons_released_queue) {
		if (_mouse_buttons_pressed.has(mouse_button_released)) {
			_mouse_buttons_pressed.erase(mouse_button_released);
		}

		_doom_instance->mouse_buttons_pressed[_doom_instance->mouse_buttons_pressed_length] = mouse_button_released;
		_doom_instance->mouse_buttons_pressed_length += 1;
	}
	_mouse_buttons_released_queue.clear();
}

void DOOM::doom_ready() {
	_img_texture.instantiate();
	set_texture(_img_texture);

	set_process(true);
	set_process_input(true);
}

void DOOM::doom_process(double delta) {
	if (_doom_instance == nullptr) {
		return;
	}

	_update_screen_buffer();
	_update_sounds();
	_update_music();

	if (!has_focus()) {
		for (Key key_pressed : _keys_pressed) {
			_doom_instance->keys_pressed[_doom_instance->keys_pressed_length] = key_pressed;
			_doom_instance->keys_pressed_length += 1;
		}
		_keys_pressed.clear();
	}
}

void DOOM::_notification(int p_what) {
	switch (p_what) {
		case NOTIFICATION_READY: {
			doom_ready();
		} break;
		case NOTIFICATION_PROCESS: {
			doom_process(get_process_delta_time());
		} break;
		case NOTIFICATION_EXIT_TREE: {
			_kill_doom();
		} break;
	}
}

DOOM::DOOM() {
	_screen_buffer_mutex.instantiate();
	_music_instructions_mutex.instantiate();
	_sound_instructions_mutex.instantiate();

	_fluid_settings = new_fluid_settings();
	fluid_settings_setstr(_fluid_settings, "player.timing-source", "system");
	fluid_set_log_function(FLUID_ERR, ::doom_custom_log_function, nullptr);
	_fluid_synth = new_fluid_synth(_fluid_settings);
	fluid_synth_set_gain(_fluid_synth, 0.8f);
	_fluid_player = new_fluid_player(_fluid_synth);
}

DOOM::~DOOM() {
	if (_fluid_player != nullptr) {
		delete_fluid_player(_fluid_player);
	}
	if (_fluid_synth != nullptr) {
		delete_fluid_synth(_fluid_synth);
	}
	if (_fluid_settings != nullptr) {
		delete_fluid_settings(_fluid_settings);
	}

	_wait_for_threads();
}

}; //namespace godot
