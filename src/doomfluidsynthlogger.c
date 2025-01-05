#include "doomfluidsynthlogger.h"

#include <fluidsynth.h>

void doom_custom_log_function(int level, const char *message, void *userdata) {
	if (level < FLUID_ERR) {
		return;
	}

	fprintf(stderr, "FluidSynth Error: %s\n", message);
}
