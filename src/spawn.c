#include <spawn.h>

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <doomgeneric/doomgeneric.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0


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

    doomgeneric_Create(argc, argv);

    while (true) {}

    return EXIT_SUCCESS;
}

void tick() {
    doomgeneric_Tick();
}

void DG_Init() {

}

void DG_DrawFrame() {

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
