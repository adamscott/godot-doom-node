#include <spawn.h>

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// Shared memory:
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include <doomgeneric/doomgeneric.h>

#include <common.h>
#include <sys/time.h>
#include <unistd.h>

bool terminate = false;
int shm_fd = 0;
SharedMemory *shm = NULL;

void signal_handler(int signal) {
	printf("signal_handler %d\n", signal);
	switch (signal) {
		case SIGUSR1: {
			tick();
		} break;

		default: {
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char **argv) {
	printf("main %i\n", argc);
	printf("%s\n", argv[0]);
	printf("%s\n", argv[1]);
	printf("%s\n", argv[2]);
	printf("%s\n", argv[3]);
	printf("%s\n", argv[4]);

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
		printf("assigning: %s\n", argv[i]);
		new_argv[i - 2] = strdup(argv[i]);
	}

	int shm_number_id = atoi(argv[2]);
	char shm_id[256];
	int res = sprintf(shm_id, "%s-%06d", GDDOOM_SPAWN_SHM_NAME, shm_number_id);
	shm_fd = shm_open(shm_id, O_RDWR, S_IRWXU | S_IRWXG);
	if (shm_fd == -1) {
		printf("shm_open failed.\n");
	}
	shm = mmap(0, sizeof(SharedMemory), PROT_WRITE, MAP_SHARED, shm_fd, 0);

	printf("new_argc: %d, new_argv[1]: %s, new_argv[2]: %s\n", new_argc, new_argv[1], new_argv[2]);
	doomgeneric_Create(new_argc, new_argv);
	printf("after doomgeneric_Create\n");

	while (true) {
		if (terminate) {
			exit(EXIT_SUCCESS);
		}
		// usleep(10);

		tick();
	}

	// free(new_argv);
	// shm_fd = 0;
	// shm_unlink(GDDOOM_SPAWN_SHM_NAME);

	return EXIT_SUCCESS;
}

void tick() {
	printf("tick! terminate? %b\n", shm->terminate);
	if (shm->terminate) {
		terminate = true;
		return;
	}
	printf("doomtick\n");
	doomgeneric_Tick();
	shm->ready = true;
	printf("after doomtick\n");
}

void DG_Init() {
	printf("DG_Init()\n");
	// shm->init = true;
}

void DG_DrawFrame() {
	printf("DG_DrawFrame()\n");
	shm->screen_buffer = *DG_ScreenBuffer;
}

void DG_SleepMs(uint32_t ms) {
	shm->init = true;
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
	printf("DG_SetWindowTitle(%s)", title);
}
