
#include <godot_cpp/core/class_db.hpp>

#include "gddoom.h"

extern "C" {
#include <common.h>
}

using namespace godot;

// ==============
// GDDoomInstance
// ==============

GDDoomInstance *GDDoomInstance::_singleton = nullptr;

void GDDoomInstance::tick() {
}

GDDoomInstance::GDDoomInstance() {
}

GDDoomInstance *GDDoomInstance::get_singleton() {
	if (!_singleton) {
		_singleton = memnew(GDDoomInstance);
	}

	return _singleton;
}

// ======
// GDDoom
// ======

void GDDoom::_bind_methods() {
}

void GDDoom::_process(double delta) {
	GDDoomInstance::get_singleton()->tick();
}

void GDDoom::_ready() {
}

GDDoom::GDDoom() {
}

GDDoom::~GDDoom() {
}
