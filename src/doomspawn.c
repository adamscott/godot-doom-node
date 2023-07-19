#include "doomspawn.h"

#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include <doomgeneric/doomgeneric.h>

#include "doomcommon.h"
#include "doomshm.h"

boolean terminate = false;
boolean start_loop = false;

void signal_handler(int signal) {
	// printf("signal_handler %d\n", signal);
	switch (signal) {
		case SIGUSR1: {
			// printf("start_loop = true\n");
			start_loop = true;
		} break;

		default: {
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char **argv) {
	printf("Hello, World! from spawn\n");

	if (signal(SIGUSR1, signal_handler) == SIG_ERR) {
		printf("Error while setting the signal handler.\n", stderr);
		return EXIT_FAILURE;
	}

	if (argc < 3) {
		printf("Error, missing arguments.\n");
		return EXIT_FAILURE;
	}

	if (strcmp(argv[1], "-id") == -1) {
		printf("first argument must be `-id`\n");
		return EXIT_FAILURE;
	}

	int new_argc = argc - 2;
	char **new_argv = malloc(sizeof(char *) * (argc - 2));
	new_argv[0] = strdup(argv[0]);
	for (int i = 3; i < argc; i++) {
		new_argv[i - 2] = strdup(argv[i]);
	}

	int err = init_shm(argv[2]);
	if (err < 0) {
		exit(EXIT_FAILURE);
	}

	doomgeneric_Create(new_argc, new_argv);

	shm->init = true;

	while (true) {
		if (terminate) {
			exit(EXIT_SUCCESS);
		}

		if (start_loop) {
			start_loop = false;
			tick();
		}

		usleep(10);
	}

	return EXIT_SUCCESS;
}

void tick() {
	// printf("tick! terminate? %b\n", shm->terminate);
	if (shm->terminate) {
		terminate = true;
		return;
	}
	// printf("doomtick\n");
	doomgeneric_Tick();
	shm->ready = true;
	// printf("after doomtick\n");
}

void DG_Init() {
	// printf("DG_Init()\n");
	// shm->init = true;
}

void DG_DrawFrame() {
	unsigned char screen_buffer[DOOMGENERIC_RESX * DOOMGENERIC_RESY * RGBA];
	memcpy(screen_buffer, DG_ScreenBuffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * RGBA);
	unsigned char buffer[DOOMGENERIC_RESX * DOOMGENERIC_RESY * RGBA];
	for (int i = 0; i < DOOMGENERIC_RESX * DOOMGENERIC_RESY * RGBA; i += RGBA) {
		buffer[i + 0] = screen_buffer[i + 2];
		buffer[i + 1] = screen_buffer[i + 1];
		buffer[i + 2] = screen_buffer[i + 0];
		buffer[i + 3] = 255 - screen_buffer[i + 3];
	}
	memcpy(shm->screen_buffer, buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * RGBA);
}

void DG_SleepMs(uint32_t ms) {
	// shm->init = true;
	// printf("DG_SleepMs(%d)\n", ms);
	shm->sleep_ms = ms;
	usleep(ms * 1000);
}

uint32_t DG_GetTicksMs() {
	// printf("DG_GetTicksMs: %ld\n", shm->ticks_msec);
	// return shm->ticks_msec;

	struct timeval tp;
	struct timezone tzp;

	gettimeofday(&tp, &tzp);

	return (tp.tv_sec * 1000) + (tp.tv_usec / 1000); /* return milliseconds */
}

int DG_GetKey(int *pressed, unsigned char *key) {
	return 0;
}

void DG_SetWindowTitle(const char *title) {
	// printf("DG_SetWindowTitle(%s)\n", title);
	if (sizeof(title) > sizeof(shm->window_title)) {
		printf("WARN: Could not copy window title \"%s\", as it's longer than 255 characters.", title);
		return;
	}
	strcpy(shm->window_title, title);
}
