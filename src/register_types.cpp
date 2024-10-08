#include "register_types.h"

#include <gdextension_interface.h>

#include "godot_cpp/core/memory.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

#include "doom.h"
#include "doommus2mid.h"

using namespace godot;

void initialize_godot_doom_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	ClassDB::register_class<DOOM>();
	ClassDB::register_class<DOOMMus2Mid>();
}

void uninitialize_godot_doom_module(ModuleInitializationLevel p_level) {
	if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
		return;
	}

	printf("uninitialize_godot_doom_module\n");
}

extern "C" {
// initialization
GDExtensionBool GDE_EXPORT godot_doom_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
	godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

	init_obj.register_initializer(initialize_godot_doom_module);
	init_obj.register_terminator(uninitialize_godot_doom_module);
	init_obj.set_minimum_library_initialization_level(godot::MODULE_INITIALIZATION_LEVEL_SCENE);

	return init_obj.init();
}
}
