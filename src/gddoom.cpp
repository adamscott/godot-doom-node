#include <signal.h>
#include <cstdint>
#include <cstring>

#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomtype.h"
#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/classes/global_constants.hpp"
#include "godot_cpp/classes/image_texture.hpp"
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

#include "gddoom.h"

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
#include <common.h>
}

using namespace godot;

// ======
// GDDoom
// ======

void GDDoom::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_thread_func"), &GDDoom::_thread_func);

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

	String path = ProjectSettings::get_singleton()->globalize_path("res://bin/gddoom-spawn.linux.template_debug.x86_64");
	String doom1_wad = ProjectSettings::get_singleton()->globalize_path(wad_path);
	const char *shm_test = vformat("%s", _shm_id).utf8().get_data();
	const char *doom1_wad_char = doom1_wad.utf8().get_data();

	// UtilityFunctions::print(vformat("doom1_wad: %s %s", doom1_wad, doom1_wad_char));

	_spawn_pid = fork();
	if (_spawn_pid == 0) {
		// This is the forked child
		UtilityFunctions::print("Hi! From the forked child!");

		char *args[] = {
			strdup(path.utf8().get_data()),
			strdup("-id"),
			strdup(shm_test),
			strdup("-iwad"),
			strdup(doom1_wad_char),
			NULL
		};

		char *envp[] = {
			NULL
		};

		UtilityFunctions::print("launching execve");
		execve(args[0], args, envp);
		return;
	}

	_thread.instantiate();

	Callable func = Callable(this, "_thread_func");
	UtilityFunctions::print("Calling _thread.start()");
	_thread->start(func);
}

void GDDoom::kill_doom() {
	if (_spawn_pid == 0) {
		return;
	}

	UtilityFunctions::print("kill_doom()");
	_exiting = true;
	if (!_thread.is_null()) {
		_thread->wait_to_finish();
	}

	UtilityFunctions::print(vformat("Removing %s", _shm_id));
	int result = shm_unlink(_shm_id);
	if (result < 0) {
		UtilityFunctions::printerr(vformat("ERROR unlinking shm %s: %s", _shm_id, strerror(errno)));
	}

	if (_spawn_pid > 0) {
		kill(_spawn_pid, SIGKILL);
	}
	_spawn_pid = 0;
	UtilityFunctions::print("ending kill doom!");
}

void GDDoom::_process(double delta) {
	if (_spawn_pid == 0) {
		return;
	}

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

void GDDoom::_ready() {
	img_texture.instantiate();
	set_texture(img_texture);
}

void GDDoom::_enter_tree() {
	UtilityFunctions::print("_enter_tree()");
}

void GDDoom::_exit_tree() {
	if (enabled) {
		kill_doom();
	}

	// texture_rect->queue_free();
}

void GDDoom::_thread_func() {
	UtilityFunctions::print("_thread_func START!");

	while (true) {
		UtilityFunctions::print("Start thread loop");
		if (this->_exiting) {
			return;
		}

		// Send the tick signal
		// UtilityFunctions::print(vformat("Send SIGUSR1 signal to %s!", _spawn_pid));
		while (!this->_shm->init) {
			if (this->_exiting) {
				return;
			}
			UtilityFunctions::print("thread: not init...");
			// OS::get_singleton()->delay_usec(10);
			OS::get_singleton()->delay_msec(1000);
		}
		UtilityFunctions::print("thread detects shm is init!");
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
		UtilityFunctions::print("thread discovered that shm is ready!");
		if (this->_exiting) {
			return;
		}
		// The shared memory is ready
		memcpy(screen_buffer, _shm->screen_buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

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

	UtilityFunctions::print(vformat("gddoom init_shm unlinking preemptively \"%s\"", _shm_id));
	shm_unlink(_shm_id);
	UtilityFunctions::print(vformat("gddoom init_shm open shm \"%s\"", _shm_id));
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
}

GDDoom::~GDDoom() {
}

int GDDoom::_last_id = 0;
