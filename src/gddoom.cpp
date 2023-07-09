#include <signal.h>
#include <cstring>

#include "godot_cpp/classes/os.hpp"
#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/core/class_db.hpp"
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
}

void GDDoom::_process(double delta) {
}

void GDDoom::_ready() {
}

void GDDoom::_enter_tree() {
	_init_shm();

	String path = ProjectSettings::get_singleton()->globalize_path("res://bin/gddoom-spawn.linux.template_debug.x86_64");
	String doom1_wad = ProjectSettings::get_singleton()->globalize_path("res://doom/DOOM1.WAD");

	_spawn_pid = fork();
	if (_spawn_pid == 0) {
		char *args[] = {
			const_cast<char *>(path.utf8().get_data()),
			const_cast<char *>(String("-id").utf8().get_data()),
			const_cast<char *>(_shm_id),
			const_cast<char *>(String("-iwad").utf8().get_data()),
			const_cast<char *>(doom1_wad.utf8().get_data()),
			NULL
		};

		int val = execvp(args[0], args);
		if (val != EXIT_SUCCESS) {
			UtilityFunctions::printerr("execvp returned %d", val);
		}
		return;
	}

	_thread.instantiate();

	Callable func = Callable(this, "_thread_func");
	UtilityFunctions::print("Calling _thread.start()");
	_thread->start(func);
}

void GDDoom::_exit_tree() {
	UtilityFunctions::print("_exit_tree()");
	_exiting = true;
	if (!_thread.is_null()) {
		_thread->wait_to_finish();
	}

	UtilityFunctions::print(vformat("Removing %s", _shm_id).utf8().get_data());
	int result = shm_unlink(_shm_id);
	if (result < 0) {
		UtilityFunctions::printerr(vformat("ERROR unlinking shm %s: %s", _shm_id, strerror(errno)).utf8().get_data());
	}
}

void GDDoom::_thread_func() {
	UtilityFunctions::print("_thread_func START!");
	while (true) {
		if (this->_exiting) {
			break;
		}

		// UtilityFunctions::print(vformat("me? %s %s %p", this->get_instance_id(), _spawn_pid, this->_shm));
		UtilityFunctions::print("thread");
		OS::get_singleton()->delay_msec(1000);
		// Send the tick signal
		kill(_spawn_pid, SIGUSR1);

		// // Let's wait for the shared memory to be ready
		// while (!this->_shm->ready) {
		// 	OS::get_singleton()->delay_usec(100);
		// }

		// // The shared memory is ready

		// // Let's sleep the time Doom asks
		// usleep(this->_shm->sleep_ms * 1000);

		// // Reset the shared memory
		// this->_shm->ready = false;
	}
}

void GDDoom::_init_shm() {
	_id = _last_id;
	_last_id += 1;

	const char *spawn_id = vformat("%s-%06d", GDDOOM_SPAWN_SHM_NAME, _id).utf8().get_data();
	UtilityFunctions::print(vformat("my spawn id = %s", spawn_id));
	strcpy(_shm_id, spawn_id);
	UtilityFunctions::print("after strcpy");
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

GDDoom::GDDoom() {
}

GDDoom::~GDDoom() {
}

int GDDoom::_last_id = 0;
