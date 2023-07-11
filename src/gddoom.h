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
	char _shm_id[255];

	bool _exiting = false;

	SharedMemory *_shm;
	__pid_t _spawn_pid;
	Ref<Thread> _thread = nullptr;
	int _shm_fd;

	Vector<String> _uuids;

	bool enabled = false;
	String wad_path;

	void _init_shm();
	void _thread_func();
	void init_doom();
	void kill_doom();

	void update_doom();

protected:
	static void _bind_methods();

public:
	GDDoom();
	~GDDoom();

	bool get_enabled();
	void set_enabled(bool p_enabled);
	String get_wad_path();
	void set_wad_path(String p_wad_path);

	void _enter_tree() override;
	void _exit_tree() override;
	void _ready() override;
	void _process(double delta) override;
};

} // namespace godot

#endif // GDDOOM_H
