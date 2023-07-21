#ifndef DOOM_GENERIC
#define DOOM_GENERIC

#include <stdint.h>
#include <stdlib.h>

#define DOOMGENERIC_RESX 640
#define DOOMGENERIC_RESY 400

extern uint32_t *DG_ScreenBuffer;

void doomgeneric_Create(int argc, char **argv);
void doomgeneric_Tick();

//Implement below functions for your platform
void DG_Init();
void DG_DrawFrame();
void DG_SleepMs(uint32_t ms);
uint32_t DG_GetTicksMs();
int DG_GetKey(int *pressed, unsigned char *key);
void DG_SetWindowTitle(const char *title);

// GODOT EDIT START
void DG_GetMouseState(int *mouse_x, int *mouse_y, int *mouse_button_bitfield);
// GODOT EDIT END

#endif //DOOM_GENERIC
