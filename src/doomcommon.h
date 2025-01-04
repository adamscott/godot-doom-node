#ifndef DOOMCOMMON_H
#define DOOMCOMMON_H

#include <cstdint>

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/variant/string.hpp>

#define RGBA 4

namespace godot {

class DOOMSoundInstruction : public RefCounted {
	GDCLASS(DOOMSoundInstruction, RefCounted);

protected:
	static void _bind_methods() {};

public:
	enum DOOMSoundInstructionType {
		SOUND_INSTRUCTION_TYPE_EMPTY,
		SOUND_INSTRUCTION_TYPE_PRECACHE_SOUND,
		SOUND_INSTRUCTION_TYPE_START_SOUND,
		SOUND_INSTRUCTION_TYPE_STOP_SOUND,
		SOUND_INSTRUCTION_TYPE_UPDATE_SOUND_PARAMS,
		SOUND_INSTRUCTION_TYPE_SHUTDOWN_SOUND,
		SOUND_INSTRUCTION_TYPE_MAX
	};

	DOOMSoundInstructionType type;
	String name;
	int32_t channel;
	int32_t volume;
	int32_t sep;
	int32_t pitch;
	int32_t priority;
	int32_t usefulness;
};

class DOOMMusicInstruction : public RefCounted {
	GDCLASS(DOOMMusicInstruction, RefCounted);

protected:
	static void _bind_methods() {};

public:
	enum DOOMMusicInstructionType {
		MUSIC_INSTRUCTION_TYPE_EMPTY,
		MUSIC_INSTRUCTION_TYPE_INIT,
		MUSIC_INSTRUCTION_TYPE_SHUTDOWN_MUSIC,
		MUSIC_INSTRUCTION_TYPE_SET_MUSIC_VOLUME,
		MUSIC_INSTRUCTION_TYPE_PAUSE_SONG,
		MUSIC_INSTRUCTION_TYPE_RESUME_SONG,
		MUSIC_INSTRUCTION_TYPE_REGISTER_SONG,
		MUSIC_INSTRUCTION_TYPE_UNREGISTER_SONG,
		MUSIC_INSTRUCTION_TYPE_PLAY_SONG,
		MUSIC_INSTRUCTION_TYPE_STOP_SONG,
	};

	DOOMMusicInstructionType type;
	String lump_sha1_hex;
	int32_t volume;
	bool looping = false;
};

} //namespace godot

#endif // DOOMCOMMON_H
