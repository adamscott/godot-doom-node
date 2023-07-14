#include <stdint.h>

inline uint16_t swap_16(uint16_t p_value) {
	return (((p_value & 0x00FF) << 8) |
			((p_value & 0xFF00) >> 8));
}

inline uint32_t swap_32(uint32_t p_value) {
	return (((p_value & 0x000000FF) << 24) |
			((p_value & 0x0000FF00) << 8) |
			((p_value & 0x00FF0000) >> 8) |
			((p_value & 0xFF000000) >> 24));
}
