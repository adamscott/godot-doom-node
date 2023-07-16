#include "doom.h"

#include <signal.h>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>

#include "godot_cpp/classes/audio_server.hpp"
#include "godot_cpp/classes/audio_stream_generator.hpp"
#include "godot_cpp/classes/audio_stream_generator_playback.hpp"
#include "godot_cpp/classes/audio_stream_player.hpp"
#include "godot_cpp/classes/audio_stream_wav.hpp"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/dir_access.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/hashing_context.hpp"
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
#include "godot_cpp/variant/callable.hpp"
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

// GodotDoomNode
#include "common.h"

// Fluidsynth
#include "fluidsynth.h"
#include "fluidsynth/audio.h"
#include "fluidsynth/midi.h"
#include "fluidsynth/misc.h"
#include "fluidsynth/settings.h"
#include "fluidsynth/synth.h"
#include "fluidsynth/types.h"
}

using namespace godot;

void DOOM::_bind_methods() {
	// Signals.
	ADD_SIGNAL(MethodInfo("assets_imported"));

	ClassDB::bind_method(D_METHOD("doom_thread_func"), &DOOM::doom_thread_func);
	ClassDB::bind_method(D_METHOD("wad_thread_func"), &DOOM::wad_thread_func);
	ClassDB::bind_method(D_METHOD("sound_fetching_thread_func"), &DOOM::sound_fetching_thread_func);
	ClassDB::bind_method(D_METHOD("midi_fetching_thread_func"), &DOOM::midi_fetching_thread_func);
	ClassDB::bind_method(D_METHOD("midi_thread_func"), &DOOM::midi_thread_func);
	ClassDB::bind_method(D_METHOD("import_assets"), &DOOM::import_assets);
	ClassDB::bind_method(D_METHOD("wad_thread_end"), &DOOM::wad_thread_end);
	ClassDB::bind_method(D_METHOD("sound_fetching_thread_end"), &DOOM::sound_fetching_thread_end);
	ClassDB::bind_method(D_METHOD("midi_fetching_thread_end"), &DOOM::midi_fetching_thread_end);

	ADD_GROUP("DOOM", "doom_");
	ADD_GROUP("Assets", "assets_");

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
}

bool DOOM::get_import_assets() {
	return false;
}

void DOOM::set_import_assets(bool p_import_assets) {
	call_deferred("import_assets");
}

bool DOOM::get_enabled() {
	return enabled;
}

void DOOM::set_enabled(bool p_enabled) {
	enabled = p_enabled;

	update_doom();
}

String DOOM::get_wad_path() {
	return wad_path;
}

void DOOM::set_wad_path(String p_wad_path) {
	wad_path = p_wad_path;
}

String DOOM::get_soundfont_path() {
	return soundfont_path;
}

void DOOM::set_soundfont_path(String p_soundfont_path) {
	soundfont_path = p_soundfont_path;
}

void DOOM::import_assets() {
	if (wad_path.is_empty()) {
		UtilityFunctions::printerr(vformat("[Godot DOOM node] Wad path must not be empty."));
		return;
	}

	if (!FileAccess::file_exists(wad_path)) {
		UtilityFunctions::printerr(vformat("[Godot DOOM node] Wad path (\"%s\") isn't a file.", wad_path));
		return;
	}

	if (!wad_thread.is_null() && wad_thread->is_started()) {
		UtilityFunctions::printerr(vformat("[Godot DOOM node] A process is already importing assets."));
		return;
	}

	Ref<DirAccess> user_dir = DirAccess::open("user://");
	if (!user_dir->dir_exists("godot-doom")) {
		user_dir->make_dir("godot-doom");
	}

	wad_thread.instantiate();
	Callable wad_func = Callable(this, "wad_thread_func");
	wad_thread->start(wad_func);
}

void DOOM::update_doom() {
	if (enabled && assets_ready) {
		init_doom();
	} else {
		enabled = false;
		if (spawn_pid > 0) {
			kill_doom();
		}
	}
}

void DOOM::init_doom() {
	exiting = false;

	init_shm();
	shm->ticks_msec = Time::get_singleton()->get_ticks_msec();

	spawn_pid = fork();
	if (spawn_pid == 0) {
		launch_doom_executable();
	}

	doom_thread.instantiate();
	midi_thread.instantiate();

	Callable doom_func = Callable(this, "doom_thread_func");
	doom_thread->start(doom_func);
	Callable midi_func = Callable(this, "midi_thread_func");
	midi_thread->start(midi_func);
}

void DOOM::launch_doom_executable() {
	const char *path = ProjectSettings::get_singleton()->globalize_path("res://bin/godot-doom-spawn.linux.template_debug.x86_64").utf8().get_data();
	const char *id = vformat("%s", shm_id).utf8().get_data();
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

	execve(args[0], args, envp);
}

void DOOM::midi_thread_func() {
	while (true) {
		if (exiting) {
			return;
		}

		if (current_midi_path.is_empty() || !current_midi_playing || current_midi_pause) {
			usleep(1000);
			continue;
		}

		fluid_settings_t *settings;
		fluid_synth_t *synth;
		fluid_player_t *player;
		fluid_audio_driver_t *audio_driver;

		String soundfont_global_path = ProjectSettings::get_singleton()->globalize_path(soundfont_path);
		char *soundfont_global_path_char = strdup(soundfont_global_path.utf8().get_data());

		String current_midi_global_path = ProjectSettings::get_singleton()->globalize_path(current_midi_path);
		char *current_midi_global_path_char = strdup(current_midi_global_path.utf8().get_data());

		settings = new_fluid_settings();
		// fluid_settings_setint(settings, "player.reset-synth", false);
		// fluid_settings_setstr(settings, "player.timing-source", "sample");

		synth = new_fluid_synth(settings);
		fluid_synth_set_gain(synth, 1.0f);

		if (!fluid_is_soundfont(soundfont_global_path_char)) {
			soundfont_path = "";
			usleep(1000);
			continue;
		}

		int synth_id = fluid_synth_sfload(synth, soundfont_global_path_char, false);

		player = new_fluid_player(synth);
		// audio_driver = new_fluid_audio_driver(settings, synth);

		if (!fluid_is_midifile(current_midi_global_path_char)) {
			current_midi_path = "";
			usleep(1000);
			continue;
		}

		fluid_player_add(player, current_midi_global_path_char);
		fluid_player_seek(player, current_midi_tick);
		fluid_player_play(player);

		current_midi_last_tick = Time::get_singleton()->get_ticks_usec();

		while (fluid_player_get_status(player) == FLUID_PLAYER_PLAYING) {
			// UtilityFunctions::print("PLAYING");
			if (exiting) {
				fluid_player_stop(player);
				fluid_player_join(player);

				// delete_fluid_audio_driver(audio_driver);
				delete_fluid_player(player);
				delete_fluid_synth(synth);
				delete_fluid_settings(settings);
				return;
			}

			if (current_midi_pause) {
				current_midi_tick = fluid_player_get_current_tick(player);
				fluid_player_stop(player);
			}

			if (!current_midi_playing) {
				current_midi_tick = 0;
				fluid_player_stop(player);
			}

			if (current_midi_playback.is_null()) {
				continue;
			}

			usleep(100);

			uint64_t ticks = Time::get_singleton()->get_ticks_usec();
			uint64_t diff = ticks - current_midi_last_tick;

			uint32_t len_asked = current_midi_stream->get_mix_rate() / 100000 * diff;

			current_midi_last_tick = ticks;

			// uint32_t len_asked = current_midi_playback->get_frames_available();
			// UtilityFunctions::print(vformat("len asked: %s, avail: %s", len_asked, current_midi_playback->get_frames_available()));

			if (current_midi_playback->get_frames_available() < len_asked) {
				len_asked = current_midi_playback->get_frames_available();
			}

			if (len_asked <= 0) {
				continue;
			}

			float bufl[len_asked], bufr[len_asked];

			// float *buf = (float *)malloc(sizeof(float) * 2 * len_asked);
			// if (buf == nullptr) {
			// 	continue;
			// }

			// memset(buf, 0, sizeof(float) * 2 * len_asked);

			if (fluid_synth_write_float(synth, len_asked, bufl, 0, 1, bufr, 0, 1) == FLUID_FAILED) {
				break;
			}

			PackedVector2Array frames;
			for (int i = 0; i < len_asked; i++) {
				// current_midi_playback->push_frame(Vector2(current_midi_buffer[i], current_midi_buffer[i + 1]));
				frames.append(Vector2(bufl[i], bufr[i]));
			}

			// memfree(buf);

			current_midi_playback->push_buffer(frames);
			frames.clear();

			// memfree(buf);

			// current_midi_frames.append_array(frames);

			len_asked = 0;

			// if (current_midi_playback->can_push_buffer(current_midi_frames.size())) {
			// 	// UtilityFunctions::print(vformat("push %s", current_midi_frames.size()));
			// 	current_midi_playback->push_buffer(current_midi_frames);
			// }
		}

		fluid_player_stop(player);
		fluid_player_join(player);

		delete_fluid_player(player);

		fluid_synth_sfunload(synth, synth_id, false);
		delete_fluid_synth(synth);

		delete_fluid_settings(settings);
	}
}

void DOOM::wad_thread_func() {
	files.clear();

	Ref<FileAccess> wad = FileAccess::open(wad_path, FileAccess::ModeFlags::READ);
	if (wad.is_null()) {
		UtilityFunctions::printerr(vformat("Could not open \"%s\"", wad_path));
		return;
	}

	PackedByteArray sig_array = wad->get_buffer(sizeof(WadOriginalSignature));
	signature.signature = vformat("%c%c%c%c", sig_array[0], sig_array[1], sig_array[2], sig_array[3]);
	signature.number_of_files = sig_array.decode_s32(sizeof(WadOriginalSignature::sig));
	signature.fat_offset = sig_array.decode_s32(sizeof(WadOriginalSignature::sig) + sizeof(WadOriginalSignature::numFiles));

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

		if (file_array.size() > 0) {
			Ref<HashingContext> hashing_context;
			hashing_context.instantiate();
			hashing_context->start(HashingContext::HASH_SHA1);
			hashing_context->update(file_array);
			info["sha1"] = hashing_context->finish().hex_encode();
		} else {
			info["sha1"] = Variant();
		}

		files[file_entry.name] = info;
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
	wad_hash = hash.hex_encode();

	call_deferred("wad_thread_end");
}

void DOOM::sound_fetching_thread_func() {
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

	call_deferred("sound_fetching_thread_end");
}

void DOOM::midi_fetching_thread_func() {
	if (!FileAccess::file_exists(soundfont_path)) {
		return;
	}

	Array keys = files.keys();

	// Rendering loop
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("D_")) {
			continue;
		}

		Dictionary info = files[key];
		PackedByteArray file_array = info["data"];
		PackedByteArray midi_output;

		String hash_dir = vformat("user://godot-doom/%s-%s", wad_path.get_file().get_basename(), wad_hash);
		String midi_file_name = vformat("%s.mid", key);
		String midi_file_path = vformat("%s/%s", hash_dir, midi_file_name);
		String wav_file_name = vformat("%s.wav", key);
		String wav_file_path = vformat("%s/%s", hash_dir, wav_file_name);

		if (FileAccess::file_exists(midi_file_path) && FileAccess::file_exists(wav_file_path)) {
			// The file exist, we can skip processing it
			break;
		}

		// The files are incomplete, deleting them is preferrable
		Ref<DirAccess> godot_doom_dir = DirAccess::open("user://godot-doom");
		if (!godot_doom_dir->dir_exists(hash_dir)) {
			godot_doom_dir->make_dir(hash_dir);
		}

		if (FileAccess::file_exists(midi_file_path)) {
			godot_doom_dir->remove(vformat("%s/%s", hash_dir, midi_file_name));
		}
		if (FileAccess::file_exists(wav_file_path)) {
			godot_doom_dir->remove(vformat("%s/%s", hash_dir, wav_file_name));
		}

		String midi_file_path_globalized = ProjectSettings::get_singleton()->globalize_path(midi_file_path);
		char *midi_file_path_globalized_char = strdup(midi_file_path_globalized.utf8().get_data());
		String wav_file_path_globalized = ProjectSettings::get_singleton()->globalize_path(wav_file_path);
		char *wav_file_path_globalized_char = strdup(wav_file_path_globalized.utf8().get_data());

		// Mus to midi conversion
		bool converted = !DOOMMus2Mid::get_singleton()->mus2mid(file_array, midi_output);
		if (!converted) {
			continue;
		}

		Ref<FileAccess> mid = FileAccess::open(midi_file_path, FileAccess::ModeFlags::WRITE);
		mid->store_buffer(midi_output);
		mid->close();

		// Midi rendering
		// settings = new_fluid_settings();
		// fluid_settings_setstr(settings, "audio.file.name", wav_file_path_globalized_char);
		// fluid_settings_setstr(settings, "audio.file.type", "auto");
		// fluid_settings_setstr(settings, "player.timing-source", "sample");
		// fluid_settings_setint(settings, "synth.lock-memory", 0);
		// fluid_settings_setint(settings, "player.reset-synth", false);

		// synth = new_fluid_synth(settings);
		// int synth_id = fluid_synth_sfload(synth, soundfont_global_path_char, false);

		// player = new_fluid_player(synth);
		// fluid_player_add(player, midi_file_path_globalized_char);
		// fluid_player_play(player);

		// renderer = new_fluid_file_renderer(synth);

		// while (fluid_player_get_status(player) == FLUID_PLAYER_PLAYING) {
		// 	if (fluid_file_renderer_process_block(renderer) != FLUID_OK) {
		// 		break;
		// 	}
		// }

		// fluid_player_stop(player);
		// fluid_player_join(player);

		// delete_fluid_file_renderer(renderer);

		// delete_fluid_player(player);

		// fluid_synth_sfunload(synth, synth_id, false);
		// delete_fluid_synth(synth);

		// delete_fluid_settings(settings);
	}

	// Resource creation loop
	// for (int i = 0; i < keys.size(); i++) {
	// 	String key = keys[i];
	// 	if (!key.begins_with("D_")) {
	// 		continue;
	// 	}

	// 	Dictionary info = files[key];
	// 	PackedByteArray data = info["data"];

	// 	String hash_dir = vformat("user://godot-doom/%s-%s", wad_path.get_file().get_basename(), wad_hash);
	// 	String midi_file_name = vformat("%s.mid", key);
	// 	String midi_file_path = vformat("%s/%s", hash_dir, midi_file_name);
	// 	String wav_file_name = vformat("%s.wav", key);
	// 	String wav_file_path = vformat("%s/%s", hash_dir, wav_file_name);

	// 	Ref<FileAccess> wav_file = FileAccess::open(wav_file_path, FileAccess::ModeFlags::READ);
	// 	Ref<AudioStreamWAV> wav;
	// 	wav.instantiate();

	// 	PackedByteArray samples;
	// 	constexpr int SAMPLE_SIZE = 1024;
	// 	for (int i = 0; i < wav_file->get_length(); i += SAMPLE_SIZE) {
	// 		int buffer_length = i + SAMPLE_SIZE > wav_file->get_length() ? wav_file->get_length() - i : SAMPLE_SIZE;
	// 		samples.append_array(wav_file->get_buffer(buffer_length));
	// 	}
	// 	wav->set_data(samples);
	// 	wav->set_stereo(true);
	// 	wav->set_format(AudioStreamWAV::Format::FORMAT_16_BITS);

	// 	AudioStreamPlayer *player = memnew(AudioStreamPlayer);
	// 	player->set_name(key);
	// 	player->set_stream(wav);

	// 	Ref<HashingContext> hashing_context;
	// 	hashing_context.instantiate();
	// 	hashing_context->start(HashingContext::HashType::HASH_SHA1);
	// 	hashing_context->update(data);
	// 	PackedByteArray hash = hashing_context->finish();
	// 	player->set_meta("sha1_mus", hash.hex_encode());

	// 	info["player"] = player;
	// }

	call_deferred("midi_fetching_thread_end");
}

void DOOM::append_sounds() {
	Node *sound_container = get_node_or_null("SoundContainer");
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
		player->set_bus("SFX");
	}

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

void DOOM::append_music() {
	Node *music_container = get_node_or_null("MusicContainer");
	if (music_container != nullptr) {
		music_container->set_name("tobedeleted");
		music_container->queue_free();
	}
	music_container = memnew(Node);
	add_child(music_container);
	music_container->set_owner(get_tree()->get_edited_scene_root());
	music_container->set_name("MusicContainer");

	AudioStreamPlayer *player = memnew(AudioStreamPlayer);
	music_container->add_child(player);
	player->set_owner(get_tree()->get_edited_scene_root());
	player->set_name("Player");
	player->set_bus("Music");

	Ref<AudioStreamGenerator> stream;
	stream.instantiate();
	stream->set_buffer_length(0.05);
	current_midi_stream = stream;
	player->set_stream(current_midi_stream);
	player->play();
}

void DOOM::wad_thread_end() {
	UtilityFunctions::print(vformat("wad_thread_end"));
	if (wad_thread->is_alive()) {
		wad_thread->wait_to_finish();
	}

	start_sound_fetching();
	start_midi_fetching();
}

void DOOM::sound_fetching_thread_end() {
	UtilityFunctions::print(vformat("sound_fetching_thread_end"));
	if (sound_fetching_thread->is_alive()) {
		sound_fetching_thread->wait_to_finish();
	}

	append_sounds();

	sound_fetch_complete = true;
	update_assets_status();
}

void DOOM::midi_fetching_thread_end() {
	UtilityFunctions::print(vformat("midi_fetching_thread_end"));
	if (sound_fetching_thread->is_alive()) {
		sound_fetching_thread->wait_to_finish();
	}

	append_music();

	midi_fetch_complete = true;
	update_assets_status();
}

void DOOM::start_sound_fetching() {
	sound_fetching_thread.instantiate();
	Callable func = Callable(this, "sound_fetching_thread_func");
	sound_fetching_thread->start(func);
}

void DOOM::start_midi_fetching() {
	midi_fetching_thread.instantiate();
	Callable func = Callable(this, "midi_fetching_thread_func");
	midi_fetching_thread->start(func);
}

void DOOM::update_assets_status() {
	if (sound_fetch_complete && midi_fetch_complete) {
		emit_signal("assets_imported");
		assets_ready = true;
	}
}

void DOOM::kill_doom() {
	exiting = true;
	if (!doom_thread.is_null()) {
		doom_thread->wait_to_finish();
	}

	if (!wad_thread.is_null()) {
		wad_thread->wait_to_finish();
	}

	int result = shm_unlink(shm_id);
	if (result < 0) {
		UtilityFunctions::printerr(vformat("ERROR unlinking shm %s: %s", shm_id, strerror(errno)));
	}

	if (spawn_pid > 0) {
		kill(spawn_pid, SIGKILL);
	}
	spawn_pid = 0;
}

void DOOM::_process(double delta) {
	if (spawn_pid == 0) {
		return;
	}

	update_screen_buffer();
	update_sounds();
	update_music();
}

void DOOM::update_screen_buffer() {
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

void DOOM::update_sounds() {
	Node *sound_container = get_node<Node>("SoundContainer");
	if (sound_container == nullptr) {
		return;
	}

	for (SoundInstruction instruction : sound_instructions) {
		switch (instruction.type) {
			case SOUND_INSTRUCTION_TYPE_START_SOUND: {
				String name = vformat("DS%s", String(instruction.name).to_upper());
				AudioStreamPlayer *source = sound_container->get_node<AudioStreamPlayer>(name);

				if (source == nullptr) {
					continue;
				}

				String channel_name = vformat("Channel%s", instruction.channel);
				AudioStreamPlayer *squatting_sound = sound_container->get_node<AudioStreamPlayer>(channel_name);

				if (squatting_sound != nullptr) {
					squatting_sound->stop();
					squatting_sound->set_stream(source->get_stream());
					// squatting_sound->set_pitch_scale(
					// 		UtilityFunctions::remap(instruction.pitch, 0, INT8_MAX, 0, 2));
					squatting_sound->set_volume_db(
							UtilityFunctions::linear_to_db(
									UtilityFunctions::remap(instruction.volume, 0.0, INT8_MAX, 0.0, 2.0)));
					squatting_sound->play();
				}
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

void DOOM::update_music() {
	AudioStreamPlayer *player = get_node<AudioStreamPlayer>("MusicContainer/Player");
	if (player == nullptr) {
		return;
	}

	for (MusicInstruction instruction : music_instructions) {
		switch (instruction.type) {
			case MUSIC_INSTRUCTION_TYPE_REGISTER_SONG: {
				UtilityFunctions::print(vformat("MUSIC_INSTRUCTION_TYPE_REGISTER_SONG"));

				String sha1;
				String midi_file;

				for (int i = 0; i < sizeof(instruction.lump_sha1_hex); i += sizeof(instruction.lump_sha1_hex[0])) {
					sha1 += vformat("%c", instruction.lump_sha1_hex[i]);
				}

				UtilityFunctions::print(vformat("sha1 of received instruction: %s", sha1));

				Array keys = files.keys();
				for (int i = 0; i < keys.size(); i++) {
					String key = keys[i];
					if (!key.begins_with("D_")) {
						continue;
					}

					Dictionary info = files[key];
					UtilityFunctions::print(vformat("Comparing with %s (%s)", key, info["sha1"]));
					if (sha1.to_lower() == String(info["sha1"]).to_lower()) {
						midi_file = key;
						break;
					}
				}

				if (midi_file.is_empty()) {
					break;
				}

				current_midi_path = vformat("user://godot-doom/%s-%s/%s.mid", wad_path.get_file().get_basename(), wad_hash, midi_file);
			} break;

			case MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG: {
				UtilityFunctions::print(vformat("MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG"));
				current_midi_path = "";
			} break;

			case MUSIC_INSTRUCTION_TYPE_PLAY_SONG: {
				UtilityFunctions::print(vformat("MUSIC_INSTRUCTION_TYPE_PLAY_SONG"));
				current_midi_looping = instruction.looping;
				current_midi_playing = true;
			} break;

			case MUSIC_INSTRUCTION_TYPE_STOP_SONG: {
				UtilityFunctions::print(vformat("MUSIC_INSTRUCTION_TYPE_STOP_SONG"));
				current_midi_playing = false;
			} break;

			case MUSIC_INSTRUCTION_TYPE_PAUSE_SONG: {
				UtilityFunctions::print(vformat("MUSIC_INSTRUCTION_TYPE_PAUSE_SONG"));
				current_midi_pause = true;
			} break;

			case MUSIC_INSTRUCTION_TYPE_RESUME_SONG: {
				UtilityFunctions::print(vformat("MUSIC_INSTRUCTION_TYPE_RESUME_SONG"));
				current_midi_pause = false;
			} break;

			case MUSIC_INSTRUCTION_TYPE_SHUTDOWN_MUSIC: {
				UtilityFunctions::print(vformat("MUSIC_INSTRUCTION_TYPE_SHUTDOWN_MUSIC"));
				current_midi_playing = false;
			} break;

			case MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME: {
				UtilityFunctions::print(vformat("MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME"));
				current_midi_volume = instruction.volume;
			} break;

			case MusicInstructionType::MUSIC_INSTRUCTION_TYPE_EMPTY:
			default: {
			}
		}
	}
	music_instructions.clear();

	player->play();
	Ref<AudioStreamGeneratorPlayback> playback = player->get_stream_playback();
	if (playback.is_null()) {
		return;
	}

	current_midi_playback = playback;

	// if (current_midi_frames.size() > 0) {
	// 	if (playback->can_push_buffer(current_midi_frames.size())) {
	// 		UtilityFunctions::print(vformat("push %s", current_midi_frames.size()));
	// 		playback->push_buffer(current_midi_frames);
	// 		current_midi_frames.clear();
	// 	}
	// }

	// len_asked = playback->get_frames_available();
}

void DOOM::_ready() {
	img_texture.instantiate();
	set_texture(img_texture);
}

void DOOM::_enter_tree() {
}

void DOOM::_exit_tree() {
	if (enabled) {
		kill_doom();
	}
}

void DOOM::doom_thread_func() {
	while (true) {
		if (this->exiting) {
			return;
		}

		// Send the tick signal
		while (!this->shm->init) {
			if (this->exiting) {
				return;
			}
			OS::get_singleton()->delay_msec(10);
		}
		kill(spawn_pid, SIGUSR1);

		shm->ticks_msec = Time::get_singleton()->get_ticks_msec();

		// // Let's wait for the shared memory to be ready
		while (!this->shm->ready) {
			if (this->exiting) {
				return;
			}
			OS::get_singleton()->delay_msec(10);
		}

		if (this->exiting) {
			return;
		}
		// The shared memory is ready
		// Screenbuffer
		memcpy(screen_buffer, shm->screen_buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
		// Sounds
		for (int i = 0; i < shm->sound_instructions_length; i++) {
			SoundInstruction instruction = shm->sound_instructions[i];
			sound_instructions.append(instruction);
		}
		shm->sound_instructions_length = 0;

		// Music
		for (int i = 0; i < shm->music_instructions_length; i++) {
			UtilityFunctions::print("got instruction!");
			MusicInstruction instruction = shm->music_instructions[i];
			music_instructions.append(instruction);
		}
		shm->music_instructions_length = 0;

		// Let's sleep the time Doom asks
		usleep(this->shm->sleep_ms * 1000);

		// Reset the shared memory
		this->shm->ready = false;
	}
}

void DOOM::init_shm() {
	id = last_id;
	last_id += 1;

	const char *spawn_id = vformat("%s-%06d", GODOT_DOOM_SHM_NAME, id).utf8().get_data();
	strcpy(shm_id, spawn_id);

	shm_unlink(shm_id);
	shm_fd = shm_open(shm_id, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (shm_fd < 0) {
		UtilityFunctions::printerr(vformat("ERROR: %s", strerror(errno)));
		return;
	}

	ftruncate(shm_fd, sizeof(SharedMemory));
	shm = (SharedMemory *)mmap(0, sizeof(SharedMemory), PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm == nullptr) {
		UtilityFunctions::printerr(vformat("ERROR: %s", strerror(errno)));
		return;
	}
}

DOOM::DOOM() {
	spawn_pid = 0;
}

DOOM::~DOOM() {
}

int DOOM::last_id = 0;
