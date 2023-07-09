#include <spawn.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <errno.h>
#include <string.h>
// Shared memory:
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <doomgeneric/doomgeneric.h>

#include <common.h>
#include <unistd.h>

int shm_fd = 0;
SharedMemory *shm = NULL;

void signal_handler(int signal) {
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
    signal(SIGUSR1, signal_handler);

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
    new_argv[0] = argv[0];
    for (int i = 3; i < argc; i++) {
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

    doomgeneric_Create(new_argc, new_argv);

    while (true) {
        usleep(10);
    }

    free(new_argv);
    shm_fd = 0;
    shm_unlink(GDDOOM_SPAWN_SHM_NAME);

    return EXIT_SUCCESS;
}

void tick() {
    doomgeneric_Tick();
}

void DG_Init() {

}

void DG_DrawFrame() {
    // DG_ScreenBuffer
}

void DG_SleepMs(uint32_t ms) {

}

uint32_t DG_GetTicksMs() {
    return 0;
}

int DG_GetKey(int* pressed, unsigned char* key) {
    return 0;
}

void DG_SetWindowTitle(const char * title) {

}
