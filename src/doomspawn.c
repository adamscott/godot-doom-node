#include "doomspawn.h"

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <inttypes.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "doomgeneric/d_event.h"
#include "doomgeneric/d_mode.h"
#include "doomgeneric/doomgeneric.h"
#include "doomgeneric/doomkeys.h"
#include "doomgeneric/g_game.h"

#include "doomcommon.h"
#include "doominput.h"
#include "doommutex.h"
#include "doomshm.h"

boolean terminate = false;

boolean left_mouse_button_pressed = false;
boolean middle_mouse_button_pressed = false;
boolean right_mouse_button_pressed = false;

int main(int argc, char **argv) {
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

	mutex_lock(shm);
	shm->init = true;
	mutex_unlock(shm);

	while (true) {
		if (terminate) {
			exit(EXIT_SUCCESS);
		}

		if (shm->tick) {
			mutex_lock(shm);
			shm->tick = false;
			mutex_unlock(shm);
			tick();
		}

		usleep(10);
	}

	return EXIT_SUCCESS;
}

void tick() {
	mutex_lock(shm);
	if (shm->terminate) {
		mutex_unlock(shm);
		terminate = true;
		return;
	}
	mutex_unlock(shm);

	doomgeneric_Tick();

	if (shm->terminate) {
		while (true) {
			usleep(10);
		}
	}

	mutex_lock(shm);
	shm->ready = true;
	mutex_unlock(shm);
}

void DG_Init() {}

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

	mutex_lock(shm);
	memcpy(shm->screen_buffer, buffer, DOOMGENERIC_RESX * DOOMGENERIC_RESY * RGBA);
	mutex_unlock(shm);
}

void DG_SleepMs(uint32_t ms) {
	mutex_lock(shm);
	shm->sleep_ms = ms;
	mutex_unlock(shm);

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
	if (shm->keys_pressed_length > 0) {
		uint32_t key_pressed = shm->keys_pressed[shm->keys_pressed_length - 1];
		*pressed = key_pressed >> 31;
		*key = convert_to_doom_key(key_pressed & ~(1 << 31));

		printf("key %d, pressed: %d\n", *(uint8_t *)key, *pressed);

		mutex_lock(shm);
		shm->keys_pressed_length -= 1;
		mutex_unlock(shm);
		return true;
	}

	mutex_lock(shm);
	shm->keys_pressed_length = 0;
	mutex_unlock(shm);
	return false;
}

void DG_GetMouseState(int *mouse_x, int *mouse_y, int *mouse_button_bitfield) {
	*mouse_x = shm->mouse_x;
	*mouse_y = shm->mouse_y;

	mutex_lock(shm);
	shm->mouse_x = 0;
	shm->mouse_y = 0;
	mutex_unlock(shm);

	boolean pressed = false;
	int8_t index = 0;

	if (shm->mouse_buttons_pressed_length > 0) {
		uint32_t mouse_button_pressed = shm->mouse_buttons_pressed[shm->mouse_buttons_pressed_length - 1];
		pressed = mouse_button_pressed >> 31;
		index = ~(1 << 31) & mouse_button_pressed;

		switch (index) {
			// left
			case 1: {
				left_mouse_button_pressed = pressed;
			} break;

			// right
			case 2: {
				right_mouse_button_pressed = pressed;
			} break;

			// middle
			case 3: {
				middle_mouse_button_pressed = pressed;
			} break;

			default: {
			}
		}

		mutex_lock(shm);
		shm->mouse_buttons_pressed_length -= 1;
		mutex_unlock(shm);
	}
	mutex_lock(shm);
	shm->mouse_buttons_pressed_length = 0;
	mutex_unlock(shm);

	*mouse_button_bitfield = left_mouse_button_pressed | right_mouse_button_pressed << 1 | middle_mouse_button_pressed << 2;
}

void DG_SetWindowTitle(const char *title) {
	// printf("DG_SetWindowTitle(%s)\n", title);
	if (sizeof(title) > sizeof(shm->window_title)) {
		printf("WARN: Could not copy window title \"%s\", as it's longer than 255 characters.", title);
		return;
	}
	mutex_lock(shm);
	strcpy(shm->window_title, title);
	mutex_unlock(shm);
}

unsigned char convert_to_doom_key(Key p_doom_key) {
	switch (p_doom_key) {
		case GDDOOM_KEY_A:
		case GDDOOM_KEY_B:
		case GDDOOM_KEY_C:
		case GDDOOM_KEY_D:
		case GDDOOM_KEY_E:
		case GDDOOM_KEY_F:
		case GDDOOM_KEY_G:
		case GDDOOM_KEY_H:
		case GDDOOM_KEY_I:
		case GDDOOM_KEY_J:
		case GDDOOM_KEY_K:
		case GDDOOM_KEY_L:
		case GDDOOM_KEY_M:
		case GDDOOM_KEY_N:
		case GDDOOM_KEY_O:
		case GDDOOM_KEY_P:
		case GDDOOM_KEY_Q:
		case GDDOOM_KEY_R:
		case GDDOOM_KEY_S:
		case GDDOOM_KEY_T:
		case GDDOOM_KEY_U:
		case GDDOOM_KEY_V:
		case GDDOOM_KEY_W:
		case GDDOOM_KEY_X:
		case GDDOOM_KEY_Y:
		case GDDOOM_KEY_Z: {
			return toupper(p_doom_key);
		}

		case GDDOOM_KEY_F1: {
			return KEY_F1;
		}
		case GDDOOM_KEY_F2: {
			return KEY_F2;
		}
		case GDDOOM_KEY_F3: {
			return KEY_F3;
		}
		case GDDOOM_KEY_F4: {
			return KEY_F4;
		}
		case GDDOOM_KEY_F5: {
			return KEY_F5;
		}
		case GDDOOM_KEY_F6: {
			return KEY_F6;
		}
		case GDDOOM_KEY_F7: {
			return KEY_F7;
		}
		case GDDOOM_KEY_F8: {
			return KEY_F8;
		}
		case GDDOOM_KEY_F9: {
			return KEY_F9;
		}
		case GDDOOM_KEY_F10: {
			return KEY_F10;
		}
		case GDDOOM_KEY_F11: {
			return KEY_F11;
		}
		case GDDOOM_KEY_F12: {
			return KEY_F12;
		}

		case GDDOOM_KEY_ENTER: {
			return KEY_ENTER;
		}
		case GDDOOM_KEY_ESCAPE: {
			return KEY_ESCAPE;
		}
		case GDDOOM_KEY_TAB: {
			return KEY_TAB;
		}
		case GDDOOM_KEY_LEFT: {
			return KEY_LEFTARROW;
		}
		case GDDOOM_KEY_UP: {
			return KEY_UPARROW;
		}
		case GDDOOM_KEY_DOWN: {
			return KEY_DOWNARROW;
		}
		case GDDOOM_KEY_RIGHT: {
			return KEY_RIGHTARROW;
		}
		case GDDOOM_KEY_BACKSPACE: {
			return KEY_BACKSPACE;
		}
		case GDDOOM_KEY_PAUSE: {
			return KEY_PAUSE;
		}

		case GDDOOM_KEY_SHIFT: {
			return KEY_RSHIFT;
		}
		case GDDOOM_KEY_ALT: {
			return KEY_RALT;
		}
		case GDDOOM_KEY_CTRL: {
			return KEY_FIRE;
		}
		case GDDOOM_KEY_COMMA: {
			return KEY_STRAFE_L;
		}
		case GDDOOM_KEY_PERIOD: {
			return KEY_STRAFE_R;
		}
		case GDDOOM_KEY_SPACE: {
			return KEY_USE;
		}
		case GDDOOM_KEY_PLUS:
		case GDDOOM_KEY_MINUS: {
			return KEY_MINUS;
		}
		case GDDOOM_KEY_EQUAL: {
			return KEY_EQUALS;
		}

		case GDDOOM_KEY_KEY_0:
		case GDDOOM_KEY_KEY_1:
		case GDDOOM_KEY_KEY_2:
		case GDDOOM_KEY_KEY_3:
		case GDDOOM_KEY_KEY_4:
		case GDDOOM_KEY_KEY_5:
		case GDDOOM_KEY_KEY_6:
		case GDDOOM_KEY_KEY_7:
		case GDDOOM_KEY_KEY_8:
		case GDDOOM_KEY_KEY_9:
		default: {
			return p_doom_key;
		}
	}
}
