#ifndef MUS2MID_H
#define MUS2MID_H

#include "godot_cpp/classes/object.hpp"
#include "godot_cpp/classes/wrapped.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include <cstdint>

#define NUM_CHANNELS 16

#define MIDI_PERCUSSION_CHAN 9
#define MUS_PERCUSSION_CHAN 15

namespace godot {
class GDDoomMus2Mid : public Object {
	GDCLASS(GDDoomMus2Mid, Object);

private:
	static GDDoomMus2Mid *singleton;

	enum MusEvent {
		MUS_RELEASEKEY = 0x00,
		MUS_PRESSKEY = 0x10,
		MUS_PITCHWHEEL = 0x20,
		MUS_SYSTEMEVENT = 0x30,
		MUS_CHANGECONTROLLER = 0x40,
		MUS_SCOREEND = 0x60
	};

	enum MidiEvent {
		MIDI_RELEASEKEY = 0x80,
		MIDI_PRESSKEY = 0x90,
		MIDI_AFTERTOUCHKEY = 0xA0,
		MIDI_CHANGECONTROLLER = 0xB0,
		MIDI_CHANGEPATCH = 0xC0,
		MIDI_AFTERTOUCHCHANNEL = 0xD0,
		MIDI_PITCHWHEEL = 0xE0
	};

	struct MusHeader {
		uint8_t id[4];
		uint16_t score_length;
		uint16_t score_start;
		uint16_t primary_channels;
		uint16_t secondary_channels;
		uint16_t instrument_count;
	};

	// Standard MIDI type 0 header + track header
	static constexpr uint8_t midi_header[] = {
		'M', 'T', 'h', 'd', // Main header
		0x00, 0x00, 0x00, 0x06, // Header size
		0x00, 0x00, // MIDI type (0)
		0x00, 0x01, // Number of tracks
		0x00, 0x46, // Resolution
		'M', 'T', 'r', 'k', // Start of track
		0x00, 0x00, 0x00, 0x00 // Placeholder for track length
	};

	uint8_t channel_velocities[NUM_CHANNELS] = {
		127, 127, 127, 127, 127, 127, 127, 127,
		127, 127, 127, 127, 127, 127, 127, 127
	};

	uint32_t queued_time;
	uint32_t track_size;

	uint8_t controller_map[15] = {
		0x00, 0x20, 0x01, 0x07, 0x0A, 0x0B, 0x5B, 0x5D,
		0x40, 0x43, 0x78, 0x7B, 0x7E, 0x7F, 0x79
	};

	int32_t channel_map[NUM_CHANNELS];

	bool write_time(uint32_t p_time, PackedByteArray &p_midi_output);
	bool write_end_track(PackedByteArray &p_midi_output);
	bool write_press_key(uint8_t p_channel, uint8_t p_key, uint8_t p_velocity, PackedByteArray &p_midi_output);
	bool write_release_key(uint8_t p_channel, uint8_t p_key, PackedByteArray &p_midi_output);
	bool write_pitch_wheel(uint8_t p_channel, uint16_t p_wheel, PackedByteArray &p_midi_output);
	bool write_change_patch(uint8_t p_channel, uint8_t p_patch, PackedByteArray &p_midi_output);
	bool write_change_controller_valued(uint8_t p_channel, uint8_t p_control, uint8_t p_value, PackedByteArray &p_midi_output);
	bool write_change_controller_valueless(uint8_t p_channel, uint8_t p_control, PackedByteArray &p_midi_output);
	int32_t allocate_midi_channel();
	int32_t get_midi_channel(int8_t p_mus_channel, PackedByteArray &p_midi_output);
	bool read_mus_header(PackedByteArray &p_file, MusHeader *header);

protected:
	static void _bind_methods();

public:
	static GDDoomMus2Mid *get_singleton();

	bool mus2mid(PackedByteArray &p_mus_input, PackedByteArray &midi_output);

	GDDoomMus2Mid();
	~GDDoomMus2Mid();
};

} //namespace godot

#endif /* MUS2MID_H */
