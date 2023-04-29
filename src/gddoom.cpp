#include <godot_cpp/core/class_db.hpp>

#include "gddoom.h"

extern "C" {
#include <doomgeneric/doomgeneric.h>
}

using namespace godot;

// ==============
// GDDoomInstance
// ==============

GDDoomInstance *GDDoomInstance::_singleton = nullptr;

void GDDoomInstance::tick() {
	doomgeneric_Tick();
}

GDDoomInstance::GDDoomInstance() {
	char **argv = static_cast<char **>(malloc(sizeof(char *) * 1));
	doomgeneric_Create(0, argv);
	free(argv);
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

extern "C" {
void DG_Init() {
}

void DG_DrawFrame() {
}

void DG_SleepMs(uint32_t ms) {
}

uint32_t DG_GetTicksMs() {
	return 0;
}

void DG_SetWindowTitle(const char *title) {
}

int DG_GetKey(int *pressed, unsigned char *key) {
	return 0;
}
}
