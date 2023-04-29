#ifndef GDDOOM_H
#define GDDOOM_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/class_db.hpp>

extern "C" {
#include "doomgeneric/doomgeneric.h"
}

namespace godot {

class GDDoomInstance {
private:
	static GDDoomInstance *_singleton;

public:
	static GDDoomInstance *get_singleton();

	void tick();

	GDDoomInstance();
	~GDDoomInstance();
};

class GDDoom : public Control {
	GDCLASS(GDDoom, Control);

protected:
	static void _bind_methods();

public:
	GDDoom();
	~GDDoom();

	void _ready() override;
	void _process(double delta) override;
};

} // namespace godot

#endif // GDDOOM_H
