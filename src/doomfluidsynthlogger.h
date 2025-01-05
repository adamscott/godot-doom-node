#ifndef DOOM_FLUIDSYNTH_LOGGER_H
#define DOOM_FLUIDSYNTH_LOGGER_H

#ifdef __cplusplus
extern "C" {
#endif

void doom_custom_log_function(int level, const char *message, void *userdata);

#ifdef __cplusplus
}
#endif

#endif // DOOM_FLUIDSYNTH_LOGGER_H
