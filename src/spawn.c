#include <spawn.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <errno.h>
// Shared memory:
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <doomgeneric/doomgeneric.h>

#include <common.h>

// #define EXIT_FAILURE 1
// #define EXIT_SUCCESS 0

int shm_fd = 0;

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

    shm_fd = shm_open(GDDOOM_SPAWN_SHM_NAME, O_RDWR, S_IRWXU | S_IRWXG);
    if (shm_fd == -1) {
        printf("This failed.");
    }

    doomgeneric_Create(argc, argv);

    while (true) {}

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
