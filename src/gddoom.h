#ifndef GDDOOM_H
#define GDDOOM_H

#include <sys/types.h>

#include "godot_cpp/classes/control.hpp"
#include "godot_cpp/classes/thread.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/templates/vector.hpp"

extern "C" {
#include "common.h"
#include "spawn.h"
}

namespace godot {

class GDDoom : public Control {
	GDCLASS(GDDoom, Control);

private:
	static int _last_id;
	int _id;
	char _shm_id[sizeof(GDDOOM_SPAWN_SHM_ID)] = GDDOOM_SPAWN_SHM_ID;

	bool _exiting = false;

	SharedMemory *_shm;
	__pid_t _spawn_pid;
	Ref<Thread> _thread = nullptr;
	int _shm_fd;

	Vector<String> _uuids;

	void _init_shm();
	void _thread_func();

protected:
	static void _bind_methods();

public:
	GDDoom();
	~GDDoom();

	void _enter_tree() override;
	void _exit_tree() override;
	void _ready() override;
	void _process(double delta) override;
};

} // namespace godot

#endif // GDDOOM_H
