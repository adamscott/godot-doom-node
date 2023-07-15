#include "doom.h"

#include <signal.h>
#include <cstdint>
#include <cstring>

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
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/packed_int32_array.hpp"
#include "godot_cpp/variant/string.hpp"
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

// GDDoom
#include "common.h"

// Fluidsynth
#include "fluidsynth.h"
#include "fluidsynth/audio.h"
#include "fluidsynth/midi.h"
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
	ClassDB::bind_method(D_METHOD("append_sounds"), &DOOM::append_sounds);
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
	import_assets();
}

bool DOOM::get_enabled() {
	return enabled;
}

void DOOM::set_enabled(bool p_enabled) {
	enabled = p_enabled;

	if (p_enabled) {
		init_doom();
	} else {
		kill_doom();
	}
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
		UtilityFunctions::printerr(vformat("[GDDoom] Wad path must not be empty."));
		return;
	}

	if (!FileAccess::file_exists(wad_path)) {
		UtilityFunctions::printerr(vformat("[GDDoom] Wad path (\"%s\") isn't a file.", wad_path));
		return;
	}

	if (wad_thread->is_started()) {
		UtilityFunctions::printerr(vformat("[GDDoom] A process is already importing assets."));
		return;
	}

	Callable wad_func = Callable(this, "wad_thread_func");
	wad_thread->start(wad_func);
}

void DOOM::update_doom() {
	if (enabled && assets_ready) {
		init_doom();
	} else if (spawn_pid > 0) {
		kill_doom();
	}
}

void DOOM::init_doom() {
	exiting = false;

	UtilityFunctions::print("init_doom!");
	init_shm();
	shm->ticks_msec = Time::get_singleton()->get_ticks_msec();

	spawn_pid = fork();
	if (spawn_pid == 0) {
		launch_doom_executable();
	}

	wad_thread.instantiate();
	doom_thread.instantiate();

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

	Callable doom_func = Callable(this, "doom_thread_func");
	UtilityFunctions::print("Calling _thread.start()");
	doom_thread->start(doom_func);
}

void DOOM::launch_doom_executable() {
	const char *path = ProjectSettings::get_singleton()->globalize_path("res://bin/gddoom-spawn.linux.template_debug.x86_64").utf8().get_data();
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

	// UtilityFunctions::print("launching execve");
	execve(args[0], args, envp);
}

void DOOM::wad_thread_func() {
	UtilityFunctions::print(vformat("wad_thread called"));

	files.clear();

	Ref<FileAccess> wad = FileAccess::open(wad_path, FileAccess::ModeFlags::READ);
	if (wad.is_null()) {
		UtilityFunctions::printerr(vformat("Could not open \"%s\"", wad_path));
		return;
	}

	// Calculate hash
	Ref<HashingContext> hashing_context;
	hashing_context->start(HashingContext::HASH_SHA256);
	while (!wad->eof_reached()) {
		hashing_context->update(wad->get_buffer(1024));
	}
	wad->seek(0);
	PackedByteArray hash = hashing_context->finish();
	wad_hash = hash.hex_encode();

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
	}

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
	fluid_settings_t *settings;
	fluid_synth_t *synth;
	fluid_player_t *player;
	fluid_file_renderer_t *renderer;

	String soundfont_global_path = ProjectSettings::get_singleton()->globalize_path(soundfont_path);
	char *soundfont_global_path_char = strdup(soundfont_global_path.utf8().get_data());

	Array keys = files.keys();
	for (int i = 0; i < keys.size(); i++) {
		String key = keys[i];
		if (!key.begins_with("D_")) {
			continue;
		}

		Dictionary info = files[key];
		PackedByteArray file_array = info["data"];
		PackedByteArray midi_output;

		String hash_dir = vformat("res://gddoom/%s-%s/", wad_path.get_basename(), wad_hash);
		String midi_file_name = vformat("%s.mid", key);
		String midi_file_path = vformat("%s/%s", hash_dir, midi_file_name);
		String ogg_file_name = vformat("%s.ogg", key);
		String ogg_file_path = vformat("%s/%s", hash_dir, ogg_file_name);

		if (FileAccess::file_exists(midi_file_path) && FileAccess::file_exists(ogg_file_path)) {
			// The file exist, we can skip processing it
			continue;
		}

		// The files are incomplete, deleting them is preferrable
		Ref<DirAccess> dir = DirAccess::open(hash_dir);
		if (FileAccess::file_exists(midi_file_path)) {
			dir->remove(midi_file_name);
		}
		if (FileAccess::file_exists(ogg_file_path)) {
			dir->remove(ogg_file_name);
		}

		String midi_file_path_globalized = ProjectSettings::get_singleton()->globalize_path(midi_file_path);
		char *midi_file_path_globalized_char = strdup(midi_file_path_globalized.utf8().get_data());
		String ogg_file_path_globalized = ProjectSettings::get_singleton()->globalize_path(ogg_file_path);
		char *ogg_file_path_globalized_char = strdup(ogg_file_path_globalized.utf8().get_data());

		bool converted = !DOOMMus2Mid::get_singleton()->mus2mid(file_array, midi_output);
		if (!converted) {
			continue;
		}

		Ref<FileAccess> mid = FileAccess::open(midi_file_path, FileAccess::ModeFlags::WRITE);
		mid->store_buffer(midi_output);
		mid->close();

		settings = new_fluid_settings();

		fluid_settings_setstr(settings, "audio.file.name", ogg_file_path_globalized_char);
		fluid_settings_setstr(settings, "audio.file.type", "oga");
		fluid_settings_setstr(settings, "player.timing-source", "sample");
		fluid_settings_setint(settings, "synth.lock-memory", 0);

		synth = new_fluid_synth(settings);
		int synth_id = fluid_synth_sfload(synth, soundfont_global_path_char, false);

		player = new_fluid_player(synth);
		fluid_player_add(player, midi_file_path_globalized_char);
		fluid_player_play(player);

		renderer = new_fluid_file_renderer(synth);

		while (fluid_player_get_status(player) == FLUID_PLAYER_PLAYING) {
			if (fluid_file_renderer_process_block(renderer) != FLUID_OK) {
				break;
			}
		}

		fluid_player_stop(player);
		fluid_player_join(player);

		delete_fluid_file_renderer(renderer);

		delete_fluid_player(player);

		fluid_synth_sfunload(synth, synth_id, false);
		delete_fluid_synth(synth);

		delete_fluid_settings(settings);
	}

	call_deferred("midi_fetching_thread_end");
}

void DOOM::append_sounds() {
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

void DOOM::wad_thread_end() {
	if (wad_thread->is_alive()) {
		wad_thread->wait_to_finish();
	}

	start_sound_fetching();
	start_midi_fetching();
}

void DOOM::sound_fetching_thread_end() {
	if (sound_fetching_thread->is_alive()) {
		sound_fetching_thread->wait_to_finish();
	}

	sound_fetch_complete = true;
}

void DOOM::midi_fetching_thread_end() {
	if (sound_fetching_thread->is_alive()) {
		sound_fetching_thread->wait_to_finish();
	}

	midi_fetch_complete = true;
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
	}
}

void DOOM::kill_doom() {
	// UtilityFunctions::print("kill_doom()");
	exiting = true;
	if (!doom_thread.is_null()) {
		doom_thread->wait_to_finish();
	}

	if (!wad_thread.is_null()) {
		wad_thread->wait_to_finish();
	}

	// UtilityFunctions::print(vformat("Removing %s", _shm_id));
	int result = shm_unlink(shm_id);
	if (result < 0) {
		UtilityFunctions::printerr(vformat("ERROR unlinking shm %s: %s", shm_id, strerror(errno)));
	}

	if (spawn_pid > 0) {
		kill(spawn_pid, SIGKILL);
	}
	spawn_pid = 0;
	// UtilityFunctions::print("ending kill doom!");
}

void DOOM::_process(double delta) {
	if (spawn_pid == 0) {
		return;
	}

	// UtilityFunctions::print(vformat("_spawn_pid: %s", _spawn_pid));

	update_screen_buffer();
	update_sounds();
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

void DOOM::_ready() {
	img_texture.instantiate();
	set_texture(img_texture);
}

void DOOM::_enter_tree() {
	// UtilityFunctions::print("_enter_tree()");
}

void DOOM::_exit_tree() {
	if (enabled) {
		kill_doom();
	}

	// texture_rect->queue_free();
}

void DOOM::doom_thread_func() {
	// UtilityFunctions::print("doom_thread START!");

	while (true) {
		// UtilityFunctions::print("Start thread loop");
		if (this->exiting) {
			return;
		}

		// Send the tick signal
		// UtilityFunctions::print(vformat("Send SIGUSR1 signal to %s!", _spawn_pid));
		while (!this->shm->init) {
			if (this->exiting) {
				return;
			}
			// UtilityFunctions::print("thread: not init...");
			// OS::get_singleton()->delay_usec(10);
			OS::get_singleton()->delay_msec(1000);
		}
		// UtilityFunctions::print("thread detects shm is init!");
		kill(spawn_pid, SIGUSR1);

		// OS::get_singleton()->delay_msec(1000);
		shm->ticks_msec = Time::get_singleton()->get_ticks_msec();

		// // Let's wait for the shared memory to be ready
		while (!this->shm->ready) {
			if (this->exiting) {
				return;
			}
			OS::get_singleton()->delay_usec(10);
		}
		// UtilityFunctions::print("thread discovered that shm is ready!");
		if (this->exiting) {
			return;
		}
		// The shared memory is ready
		// Screenbuffer
		memcpy(screen_buffer, shm->screen_buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);
		// Sounds
		for (int i = 0; i < shm->sound_instructions_length; i++) {
			SoundInstructions instructions = shm->sound_instructions[i];
			sound_instructions.append(instructions);
		}
		shm->sound_instructions_length = 0;

		// Let's sleep the time Doom asks
		usleep(this->shm->sleep_ms * 1000);

		// Reset the shared memory
		this->shm->ready = false;
	}
}

void DOOM::init_shm() {
	id = last_id;
	last_id += 1;

	const char *spawn_id = vformat("%s-%06d", GDDOOM_SPAWN_SHM_NAME, id).utf8().get_data();
	strcpy(shm_id, spawn_id);

	// UtilityFunctions::print(vformat("gddoom init_shm unlinking preemptively \"%s\"", _shm_id));
	shm_unlink(shm_id);
	// UtilityFunctions::print(vformat("gddoom init_shm open shm \"%s\"", _shm_id));
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
