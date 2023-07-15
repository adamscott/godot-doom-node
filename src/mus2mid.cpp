//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2006 Ben Ryves 2006
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// mus2mid.c - Ben Ryves 2006 - http://benryves.com - benryves@benryves.com
// Use to convert a MUS file into a single track, type 0 MIDI file.
// mus2mid.cpp - Adam Scott 2023 - https://adamscott.studio
// Use Godot 4 PackedByteArray instead of DOOM memory system.

#include "mus2mid.h"

#include "godot_cpp/classes/file_access.hpp"
#include "godot_cpp/core/class_db.hpp"
#include "godot_cpp/variant/packed_byte_array.hpp"
#include "godot_cpp/variant/utility_functions.hpp"
#include "godot_cpp/variant/variant.hpp"
#include <cstdint>
#include <cstring>

#define NUM_CHANNELS 16

#define MIDI_PERCUSSION_CHAN 9
#define MUS_PERCUSSION_CHAN 15

using namespace godot;

void GDDoomMus2Mid::_bind_methods() {
}

bool GDDoomMus2Mid::write_time(uint8_t p_time, PackedByteArray p_midi_output) {
	uint8_t buffer = p_time & 0x7F;
	uint8_t write_val;

	while ((p_time >>= 7) != 0) {
		buffer <<= 8;
		buffer |= ((p_time & 0x7F) | 0x80);
	}

	while (true) {
		write_val = buffer & 0xFF;

		if (!p_midi_output.append(write_val)) {
			return true;
		}
		track_size++;

		if ((buffer & 0x80) != 0) {
			buffer >>= 8;
		} else {
			queued_time = 0;
			return false;
		}
	}
}

bool GDDoomMus2Mid::write_end_track(PackedByteArray p_midi_output) {
	uint8_t end_track[] = { 0xFF, 0x2F, 0x00 };

	if (write_time(queued_time, p_midi_output)) {
		return true;
	}

	if (!p_midi_output.append(end_track[0]) || !p_midi_output.append(end_track[1]) || !p_midi_output.append(end_track[2])) {
		return true;
	}

	track_size += 3;
	return false;
}

bool GDDoomMus2Mid::write_press_key(uint8_t p_channel, uint8_t p_key, uint8_t p_velocity, PackedByteArray p_midi_output) {
	uint8_t working = MidiEvent::MIDI_PRESSKEY | p_channel;

	if (write_time(queued_time, p_midi_output)) {
		return true;
	}

	if (!p_midi_output.append(working)) {
		return true;
	}

	working = p_key & 0x7F;
	if (!p_midi_output.append(working)) {
		return true;
	}

	working = p_velocity & 0x7F;
	if (!p_midi_output.append(working)) {
		return true;
	}

	track_size += 3;

	return false;
}

bool GDDoomMus2Mid::write_release_key(uint8_t p_channel, uint8_t p_key, PackedByteArray p_midi_output) {
	uint8_t working = MidiEvent::MIDI_RELEASEKEY | p_channel;

	if (write_time(queued_time, p_midi_output)) {
		return true;
	}

	if (!p_midi_output.append(working)) {
		return true;
	}

	working = p_key & 0x7F;
	if (!p_midi_output.append(working)) {
		return true;
	}

	working = 0;
	if (!p_midi_output.append(working)) {
		return true;
	}

	track_size += 3;

	return false;
}

bool GDDoomMus2Mid::write_pitch_wheel(uint8_t p_channel, uint16_t p_wheel, PackedByteArray p_midi_output) {
	uint8_t working = MidiEvent::MIDI_PITCHWHEEL | p_channel;

	if (write_time(queued_time, p_midi_output)) {
		return true;
	}

	if (!p_midi_output.append(working)) {
		return true;
	}

	working = p_wheel & 0x7F;
	if (!p_midi_output.append(working)) {
		return true;
	}

	working = (p_wheel >> 7) & 0x7F;
	if (!p_midi_output.append(working)) {
		return true;
	}

	track_size += 3;

	return false;
}

bool GDDoomMus2Mid::write_change_patch(uint8_t p_channel, uint8_t p_patch, PackedByteArray p_midi_output) {
	uint8_t working = MidiEvent::MIDI_CHANGEPATCH | p_channel;

	if (write_time(queued_time, p_midi_output)) {
		return true;
	}

	if (!p_midi_output.append(working)) {
		return true;
	}

	working = p_patch & 0x7F;
	if (!p_midi_output.append(working)) {
		return true;
	}

	track_size += 2;

	return false;
}

bool GDDoomMus2Mid::write_change_controller_valued(uint8_t p_channel, uint8_t p_control, uint8_t p_value, PackedByteArray p_midi_output) {
	uint8_t working = MidiEvent::MIDI_CHANGECONTROLLER | p_channel;

	if (write_time(queued_time, p_midi_output)) {
		return true;
	}

	if (!p_midi_output.append(working)) {
		return true;
	}

	working = p_control & 0x7F;
	if (!p_midi_output.append(working)) {
		return true;
	}

	// Quirk in vanilla DOOM? MUS controller values should be
	// 7-bit, not 8-bit.

	working = p_value; // & 0x7F;

	// Fix on said quirk to stop MIDI players from complaining that
	// the value is out of range:

	if (working & 0x80) {
		working = 0x7F;
	}

	if (!p_midi_output.append(working)) {
		return true;
	}

	track_size += 3;

	return false;
}

bool GDDoomMus2Mid::write_change_controller_valueless(uint8_t p_channel, uint8_t p_control, PackedByteArray p_midi_output) {
	return write_change_controller_valued(p_channel, p_control, 0, p_midi_output);
}

int8_t GDDoomMus2Mid::allocate_midi_channel() {
	int8_t result;
	int8_t max;
	int8_t i;

	max = -1;

	for (i = 0; i < NUM_CHANNELS; i++) {
		if (channel_map[i] > max) {
			max = channel_map[i];
		}
	}

	result = max + 1;

	if (result == MIDI_PERCUSSION_CHAN) {
		result++;
	}

	return result;
}

int8_t GDDoomMus2Mid::get_midi_channel(int8_t p_mus_channel, PackedByteArray p_midi_output) {
	if (p_mus_channel == MUS_PERCUSSION_CHAN) {
		return MIDI_PERCUSSION_CHAN;
	}

	if (channel_map[p_mus_channel] == -1) {
		channel_map[p_mus_channel] = allocate_midi_channel();
		write_change_controller_valueless(channel_map[p_mus_channel], 0x7b, p_midi_output);
	}

	return channel_map[p_mus_channel];
}

bool GDDoomMus2Mid::read_mus_header(PackedByteArray p_file, GDDoomMus2Mid::MusHeader *header) {
	if (p_file.size() < sizeof(GDDoomMus2Mid::MusHeader)) {
		return false;
	}

	String header_id = vformat("%c%c%c%c", p_file[0], p_file[1], p_file[2], p_file[3]);
	strcpy((char *)header->id, strdup(header_id.utf8().get_data()));
	header->score_length = p_file.decode_u16(sizeof(header->id));
	header->score_start = p_file.decode_u16(sizeof(header->id) + sizeof(header->score_length));
	header->primary_channels = p_file.decode_u16(sizeof(header->id) + sizeof(header->score_length) + sizeof(header->score_start));
	header->secondary_channels = p_file.decode_u16(sizeof(header->id) + sizeof(header->score_length) + sizeof(header->score_start) + sizeof(header->primary_channels));
	header->instrument_count = p_file.decode_u16(sizeof(header->id) + sizeof(header->score_length) + sizeof(header->score_start) + sizeof(header->primary_channels) + sizeof(header->secondary_channels));

	return true;
}

bool GDDoomMus2Mid::mus2mid(PackedByteArray p_mus_input, PackedByteArray p_midi_output) {
	uint32_t seek = 0;

	MusHeader mus_file_header;

	uint8_t event_descriptor;
	int8_t channel;
	MusEvent event;

	uint8_t key;
	uint8_t controller_number;
	uint8_t controller_value;

	uint8_t track_size_buffer[4];

	int8_t hit_score_end = 0;

	uint8_t working;

	uint8_t time_delay;

	for (channel = 0; channel < NUM_CHANNELS; channel++) {
		channel_map[channel] = -1;
	}

	if (!read_mus_header(p_mus_input, &mus_file_header)) {
		return true;
	}

	if (mus_file_header.id[0] != 'M' || mus_file_header.id[1] != 'U' || mus_file_header.id[2] != 'S' || mus_file_header.id[3] != 0x1A) {
		return true;
	}

	seek = mus_file_header.score_start;

	if (p_mus_input.size() < mus_file_header.score_start) {
		return true;
	}

	// Write mus_file_header to the output
	uint8_t buffer[sizeof(MusHeader)];
	memcpy(&buffer, &mus_file_header, sizeof(MusHeader));
	for (int i = 0; i < sizeof(MusHeader); i++) {
		p_midi_output.append(buffer[i]);
	}

	track_size = 0;

	while (!hit_score_end) {
		while (!hit_score_end) {
			if (p_mus_input.size() < seek + sizeof(event_descriptor)) {
				return true;
			}
			event_descriptor = p_mus_input.decode_u8(seek);
			seek += sizeof(event_descriptor);

			channel = get_midi_channel(event_descriptor & 0x0F, p_midi_output);
			event = (MusEvent)(event_descriptor & 0x70);

			switch (event) {
				case MusEvent::MUS_RELEASEKEY: {
					if (p_mus_input.size() < seek + sizeof(key)) {
						return true;
					}
					key = p_mus_input.decode_u8(seek);
					seek += sizeof(key);

					if (write_release_key(channel, key, p_midi_output)) {
						return true;
					}
				} break;

				case MusEvent::MUS_PRESSKEY: {
					if (p_mus_input.size() < seek + sizeof(key)) {
						return true;
					}
					key = p_mus_input.decode_u8(seek);
					seek += sizeof(key);

					if (key & 0x80) {
						if (p_mus_input.size() < seek + sizeof(channel_velocities[channel])) {
							return true;
						}
						channel_velocities[channel] = p_mus_input.decode_u8(seek);
						seek += sizeof(channel_velocities[channel]);

						channel_velocities[channel] &= 0x7F;
					}

					if (write_press_key(channel, key, channel_velocities[channel], p_midi_output)) {
						return true;
					}
				} break;

				case MusEvent::MUS_PITCHWHEEL: {
					if (p_mus_input.size() < seek + sizeof(key)) {
						return true;
					}
					key = p_mus_input.decode_u8(seek);
					seek += sizeof(key);

					if (write_pitch_wheel(channel, key * 64, p_midi_output)) {
						return true;
					}
				} break;

				case MusEvent::MUS_SYSTEMEVENT: {
					if (p_mus_input.size() < seek + sizeof(controller_number)) {
						return true;
					}
					controller_number = p_mus_input.decode_u8(seek);
					seek += sizeof(controller_number);

					if (controller_number < 10 || controller_number > 14) {
						return true;
					}

					if (write_change_controller_valueless(channel, controller_map[controller_number], p_midi_output)) {
						return true;
					}
				} break;

				case MusEvent::MUS_CHANGECONTROLLER: {
					if (p_mus_input.size() < seek + sizeof(controller_number)) {
						return true;
					}
					controller_number = p_mus_input.decode_u8(seek);
					seek += sizeof(controller_number);

					if (p_mus_input.size() < seek + sizeof(controller_value)) {
						return true;
					}
					controller_value = p_mus_input.decode_u8(seek);
					seek += sizeof(controller_value);

					if (controller_number == 0) {
						if (write_change_patch(channel, controller_value, p_midi_output)) {
							return true;
						}
					} else {
						if (controller_number < 1 || controller_number > 9) {
							return true;
						}

						if (write_change_controller_valued(channel, controller_map[controller_number], controller_value, p_midi_output)) {
							return true;
						}
					}
				} break;

				case MusEvent::MUS_SCOREEND: {
					hit_score_end = true;
				} break;

				default: {
					return true;
				}
			}

			if (event_descriptor & 0x80) {
				break;
			}
		}

		if (!hit_score_end) {
			time_delay = 0;
			while (true) {
				if (p_mus_input.size() < seek + sizeof(working)) {
					return true;
				}
				working = p_mus_input.decode_u8(seek);
				seek += sizeof(working);

				time_delay = time_delay * 128 + (working & 0x7F);
				if ((working & 0x80) == 0) {
					break;
				}
			}
			queued_time += time_delay;
		}
	}

	// End of track
	if (write_end_track(p_midi_output)) {
		return true;
	}

	track_size_buffer[0] = (track_size >> 24) & 0xFF;
	track_size_buffer[1] = (track_size >> 16) & 0xFF;
	track_size_buffer[2] = (track_size >> 8) & 0xFF;
	track_size_buffer[3] = track_size & 0xFF;

	p_midi_output.set(18, track_size_buffer[0]);
	p_midi_output.set(19, track_size_buffer[1]);
	p_midi_output.set(20, track_size_buffer[2]);
	p_midi_output.set(21, track_size_buffer[3]);

	return false;
}

GDDoomMus2Mid *GDDoomMus2Mid::get_singleton() {
	if (singleton == nullptr) {
		memnew(GDDoomMus2Mid);
	}

	return singleton;
}

GDDoomMus2Mid::GDDoomMus2Mid() {
	singleton = this;
}

GDDoomMus2Mid::~GDDoomMus2Mid() {
	singleton = nullptr;
}

GDDoomMus2Mid *GDDoomMus2Mid::singleton = nullptr;
