#include <cstring>

#include "godot_cpp/classes/project_settings.hpp"
#include "godot_cpp/variant/string.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>
#include <godot_cpp/variant/variant.hpp>

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
}

void GDDoom::_process(double delta) {
}

void GDDoom::_ready() {
}

void GDDoom::init_shm() {
	_shm_fd = shm_open(GDDOOM_SPAWN_SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
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
	init_shm();

	String path = ProjectSettings::get_singleton()->globalize_path("res://bin/gddoom-spawn.linux.template_debug.x86_64");
	String doom1_wad = ProjectSettings::get_singleton()->globalize_path("res://doom/DOOM1.WAD");

	if (fork() == 0) {
		char *args[] = {
			const_cast<char *>(path.utf8().get_data()),
			const_cast<char *>(String("-iwad").utf8().get_data()),
			const_cast<char *>(doom1_wad.utf8().get_data()),
			NULL
		};

		execvp(args[0], args);
		return;
	}
}

GDDoom::~GDDoom() {
}
