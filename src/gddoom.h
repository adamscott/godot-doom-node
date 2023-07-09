#ifndef GDDOOM_H
#define GDDOOM_H

#include <godot_cpp/classes/control.hpp>
#include <godot_cpp/core/class_db.hpp>

extern "C" {
#include "common.h"
#include "spawn.h"
}

namespace godot {

class GDDoom : public Control {
	GDCLASS(GDDoom, Control);

private:
	SharedMemory *_shm;
	int _shm_fd;

	void init_shm();

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
