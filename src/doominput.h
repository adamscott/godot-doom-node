#ifndef DOOMINPUT_H
#define DOOMINPUT_H

// Took from "core/os/keyboard.h"
typedef enum Key {
	NONE = 0,
	// Special key: The strategy here is similar to the one used by toolkits,
	// which consists in leaving the 21 bits unicode range for printable
	// characters, and use the upper 11 bits for special keys and modifiers.
	// This way everything (char/keycode) can fit nicely in one 32-bit
	// integer (the enum's underlying type is `int` by default).
	GDDOOM_KEY_SPECIAL = (1 << 22),
	/* CURSOR/FUNCTION/BROWSER/MULTIMEDIA/MISC KEYS */
	GDDOOM_KEY_ESCAPE = GDDOOM_KEY_SPECIAL | 0x01,
	GDDOOM_KEY_TAB = GDDOOM_KEY_SPECIAL | 0x02,
	GDDOOM_KEY_BACKTAB = GDDOOM_KEY_SPECIAL | 0x03,
	GDDOOM_KEY_BACKSPACE = GDDOOM_KEY_SPECIAL | 0x04,
	GDDOOM_KEY_ENTER = GDDOOM_KEY_SPECIAL | 0x05,
	GDDOOM_KEY_KP_ENTER = GDDOOM_KEY_SPECIAL | 0x06,
	GDDOOM_KEY_INSERT = GDDOOM_KEY_SPECIAL | 0x07,
	GDDOOM_KEY_KEY_DELETE = GDDOOM_KEY_SPECIAL | 0x08, // "DELETE" is a reserved word on Windows.
	GDDOOM_KEY_PAUSE = GDDOOM_KEY_SPECIAL | 0x09,
	GDDOOM_KEY_PRINT = GDDOOM_KEY_SPECIAL | 0x0A,
	GDDOOM_KEY_SYSREQ = GDDOOM_KEY_SPECIAL | 0x0B,
	GDDOOM_KEY_CLEAR = GDDOOM_KEY_SPECIAL | 0x0C,
	GDDOOM_KEY_HOME = GDDOOM_KEY_SPECIAL | 0x0D,
	GDDOOM_KEY_END = GDDOOM_KEY_SPECIAL | 0x0E,
	GDDOOM_KEY_LEFT = GDDOOM_KEY_SPECIAL | 0x0F,
	GDDOOM_KEY_UP = GDDOOM_KEY_SPECIAL | 0x10,
	GDDOOM_KEY_RIGHT = GDDOOM_KEY_SPECIAL | 0x11,
	GDDOOM_KEY_DOWN = GDDOOM_KEY_SPECIAL | 0x12,
	GDDOOM_KEY_PAGEUP = GDDOOM_KEY_SPECIAL | 0x13,
	GDDOOM_KEY_PAGEDOWN = GDDOOM_KEY_SPECIAL | 0x14,
	GDDOOM_KEY_SHIFT = GDDOOM_KEY_SPECIAL | 0x15,
	GDDOOM_KEY_CTRL = GDDOOM_KEY_SPECIAL | 0x16,
	GDDOOM_KEY_META = GDDOOM_KEY_SPECIAL | 0x17,
#if defined(MACOS_ENABLED)
	GDDOOM_KEY_CMD_OR_CTRL = GDDOOM_KEY_META,
#else
	GDDOOM_KEY_CMD_OR_CTRL = GDDOOM_KEY_CTRL,
#endif
	GDDOOM_KEY_ALT = GDDOOM_KEY_SPECIAL | 0x18,
	GDDOOM_KEY_CAPSLOCK = GDDOOM_KEY_SPECIAL | 0x19,
	GDDOOM_KEY_NUMLOCK = GDDOOM_KEY_SPECIAL | 0x1A,
	GDDOOM_KEY_SCROLLLOCK = GDDOOM_KEY_SPECIAL | 0x1B,
	GDDOOM_KEY_F1 = GDDOOM_KEY_SPECIAL | 0x1C,
	GDDOOM_KEY_F2 = GDDOOM_KEY_SPECIAL | 0x1D,
	GDDOOM_KEY_F3 = GDDOOM_KEY_SPECIAL | 0x1E,
	GDDOOM_KEY_F4 = GDDOOM_KEY_SPECIAL | 0x1F,
	GDDOOM_KEY_F5 = GDDOOM_KEY_SPECIAL | 0x20,
	GDDOOM_KEY_F6 = GDDOOM_KEY_SPECIAL | 0x21,
	GDDOOM_KEY_F7 = GDDOOM_KEY_SPECIAL | 0x22,
	GDDOOM_KEY_F8 = GDDOOM_KEY_SPECIAL | 0x23,
	GDDOOM_KEY_F9 = GDDOOM_KEY_SPECIAL | 0x24,
	GDDOOM_KEY_F10 = GDDOOM_KEY_SPECIAL | 0x25,
	GDDOOM_KEY_F11 = GDDOOM_KEY_SPECIAL | 0x26,
	GDDOOM_KEY_F12 = GDDOOM_KEY_SPECIAL | 0x27,
	GDDOOM_KEY_F13 = GDDOOM_KEY_SPECIAL | 0x28,
	GDDOOM_KEY_F14 = GDDOOM_KEY_SPECIAL | 0x29,
	GDDOOM_KEY_F15 = GDDOOM_KEY_SPECIAL | 0x2A,
	GDDOOM_KEY_F16 = GDDOOM_KEY_SPECIAL | 0x2B,
	GDDOOM_KEY_F17 = GDDOOM_KEY_SPECIAL | 0x2C,
	GDDOOM_KEY_F18 = GDDOOM_KEY_SPECIAL | 0x2D,
	GDDOOM_KEY_F19 = GDDOOM_KEY_SPECIAL | 0x2E,
	GDDOOM_KEY_F20 = GDDOOM_KEY_SPECIAL | 0x2F,
	GDDOOM_KEY_F21 = GDDOOM_KEY_SPECIAL | 0x30,
	GDDOOM_KEY_F22 = GDDOOM_KEY_SPECIAL | 0x31,
	GDDOOM_KEY_F23 = GDDOOM_KEY_SPECIAL | 0x32,
	GDDOOM_KEY_F24 = GDDOOM_KEY_SPECIAL | 0x33,
	GDDOOM_KEY_F25 = GDDOOM_KEY_SPECIAL | 0x34,
	GDDOOM_KEY_F26 = GDDOOM_KEY_SPECIAL | 0x35,
	GDDOOM_KEY_F27 = GDDOOM_KEY_SPECIAL | 0x36,
	GDDOOM_KEY_F28 = GDDOOM_KEY_SPECIAL | 0x37,
	GDDOOM_KEY_F29 = GDDOOM_KEY_SPECIAL | 0x38,
	GDDOOM_KEY_F30 = GDDOOM_KEY_SPECIAL | 0x39,
	GDDOOM_KEY_F31 = GDDOOM_KEY_SPECIAL | 0x3A,
	GDDOOM_KEY_F32 = GDDOOM_KEY_SPECIAL | 0x3B,
	GDDOOM_KEY_F33 = GDDOOM_KEY_SPECIAL | 0x3C,
	GDDOOM_KEY_F34 = GDDOOM_KEY_SPECIAL | 0x3D,
	GDDOOM_KEY_F35 = GDDOOM_KEY_SPECIAL | 0x3E,
	GDDOOM_KEY_KP_MULTIPLY = GDDOOM_KEY_SPECIAL | 0x81,
	GDDOOM_KEY_KP_DIVIDE = GDDOOM_KEY_SPECIAL | 0x82,
	GDDOOM_KEY_KP_SUBTRACT = GDDOOM_KEY_SPECIAL | 0x83,
	GDDOOM_KEY_KP_PERIOD = GDDOOM_KEY_SPECIAL | 0x84,
	GDDOOM_KEY_KP_ADD = GDDOOM_KEY_SPECIAL | 0x85,
	GDDOOM_KEY_KP_0 = GDDOOM_KEY_SPECIAL | 0x86,
	GDDOOM_KEY_KP_1 = GDDOOM_KEY_SPECIAL | 0x87,
	GDDOOM_KEY_KP_2 = GDDOOM_KEY_SPECIAL | 0x88,
	GDDOOM_KEY_KP_3 = GDDOOM_KEY_SPECIAL | 0x89,
	GDDOOM_KEY_KP_4 = GDDOOM_KEY_SPECIAL | 0x8A,
	GDDOOM_KEY_KP_5 = GDDOOM_KEY_SPECIAL | 0x8B,
	GDDOOM_KEY_KP_6 = GDDOOM_KEY_SPECIAL | 0x8C,
	GDDOOM_KEY_KP_7 = GDDOOM_KEY_SPECIAL | 0x8D,
	GDDOOM_KEY_KP_8 = GDDOOM_KEY_SPECIAL | 0x8E,
	GDDOOM_KEY_KP_9 = GDDOOM_KEY_SPECIAL | 0x8F,
	GDDOOM_KEY_MENU = GDDOOM_KEY_SPECIAL | 0x42,
	GDDOOM_KEY_HYPER = GDDOOM_KEY_SPECIAL | 0x43,
	GDDOOM_KEY_HELP = GDDOOM_KEY_SPECIAL | 0x45,
	GDDOOM_KEY_BACK = GDDOOM_KEY_SPECIAL | 0x48,
	GDDOOM_KEY_FORWARD = GDDOOM_KEY_SPECIAL | 0x49,
	GDDOOM_KEY_STOP = GDDOOM_KEY_SPECIAL | 0x4A,
	GDDOOM_KEY_REFRESH = GDDOOM_KEY_SPECIAL | 0x4B,
	GDDOOM_KEY_VOLUMEDOWN = GDDOOM_KEY_SPECIAL | 0x4C,
	GDDOOM_KEY_VOLUMEMUTE = GDDOOM_KEY_SPECIAL | 0x4D,
	GDDOOM_KEY_VOLUMEUP = GDDOOM_KEY_SPECIAL | 0x4E,
	GDDOOM_KEY_MEDIAPLAY = GDDOOM_KEY_SPECIAL | 0x54,
	GDDOOM_KEY_MEDIASTOP = GDDOOM_KEY_SPECIAL | 0x55,
	GDDOOM_KEY_MEDIAPREVIOUS = GDDOOM_KEY_SPECIAL | 0x56,
	GDDOOM_KEY_MEDIANEXT = GDDOOM_KEY_SPECIAL | 0x57,
	GDDOOM_KEY_MEDIARECORD = GDDOOM_KEY_SPECIAL | 0x58,
	GDDOOM_KEY_HOMEPAGE = GDDOOM_KEY_SPECIAL | 0x59,
	GDDOOM_KEY_FAVORITES = GDDOOM_KEY_SPECIAL | 0x5A,
	GDDOOM_KEY_SEARCH = GDDOOM_KEY_SPECIAL | 0x5B,
	GDDOOM_KEY_STANDBY = GDDOOM_KEY_SPECIAL | 0x5C,
	GDDOOM_KEY_OPENURL = GDDOOM_KEY_SPECIAL | 0x5D,
	GDDOOM_KEY_LAUNCHMAIL = GDDOOM_KEY_SPECIAL | 0x5E,
	GDDOOM_KEY_LAUNCHMEDIA = GDDOOM_KEY_SPECIAL | 0x5F,
	GDDOOM_KEY_LAUNCH0 = GDDOOM_KEY_SPECIAL | 0x60,
	GDDOOM_KEY_LAUNCH1 = GDDOOM_KEY_SPECIAL | 0x61,
	GDDOOM_KEY_LAUNCH2 = GDDOOM_KEY_SPECIAL | 0x62,
	GDDOOM_KEY_LAUNCH3 = GDDOOM_KEY_SPECIAL | 0x63,
	GDDOOM_KEY_LAUNCH4 = GDDOOM_KEY_SPECIAL | 0x64,
	GDDOOM_KEY_LAUNCH5 = GDDOOM_KEY_SPECIAL | 0x65,
	GDDOOM_KEY_LAUNCH6 = GDDOOM_KEY_SPECIAL | 0x66,
	GDDOOM_KEY_LAUNCH7 = GDDOOM_KEY_SPECIAL | 0x67,
	GDDOOM_KEY_LAUNCH8 = GDDOOM_KEY_SPECIAL | 0x68,
	GDDOOM_KEY_LAUNCH9 = GDDOOM_KEY_SPECIAL | 0x69,
	GDDOOM_KEY_LAUNCHA = GDDOOM_KEY_SPECIAL | 0x6A,
	GDDOOM_KEY_LAUNCHB = GDDOOM_KEY_SPECIAL | 0x6B,
	GDDOOM_KEY_LAUNCHC = GDDOOM_KEY_SPECIAL | 0x6C,
	GDDOOM_KEY_LAUNCHD = GDDOOM_KEY_SPECIAL | 0x6D,
	GDDOOM_KEY_LAUNCHE = GDDOOM_KEY_SPECIAL | 0x6E,
	GDDOOM_KEY_LAUNCHF = GDDOOM_KEY_SPECIAL | 0x6F,

	GDDOOM_KEY_GLOBE = GDDOOM_KEY_SPECIAL | 0x70,
	GDDOOM_KEY_KEYBOARD = GDDOOM_KEY_SPECIAL | 0x71,
	GDDOOM_KEY_JIS_EISU = GDDOOM_KEY_SPECIAL | 0x72,
	GDDOOM_KEY_JIS_KANA = GDDOOM_KEY_SPECIAL | 0x73,

	GDDOOM_KEY_UNKNOWN = GDDOOM_KEY_SPECIAL | 0x7FFFFF,

	/* PRINTABLE LATIN 1 CODES */

	GDDOOM_KEY_SPACE = 0x0020,
	GDDOOM_KEY_EXCLAM = 0x0021,
	GDDOOM_KEY_QUOTEDBL = 0x0022,
	GDDOOM_KEY_NUMBERSIGN = 0x0023,
	GDDOOM_KEY_DOLLAR = 0x0024,
	GDDOOM_KEY_PERCENT = 0x0025,
	GDDOOM_KEY_AMPERSAND = 0x0026,
	GDDOOM_KEY_APOSTROPHE = 0x0027,
	GDDOOM_KEY_PARENLEFT = 0x0028,
	GDDOOM_KEY_PARENRIGHT = 0x0029,
	GDDOOM_KEY_ASTERISK = 0x002A,
	GDDOOM_KEY_PLUS = 0x002B,
	GDDOOM_KEY_COMMA = 0x002C,
	GDDOOM_KEY_MINUS = 0x002D,
	GDDOOM_KEY_PERIOD = 0x002E,
	GDDOOM_KEY_SLASH = 0x002F,
	GDDOOM_KEY_KEY_0 = 0x0030,
	GDDOOM_KEY_KEY_1 = 0x0031,
	GDDOOM_KEY_KEY_2 = 0x0032,
	GDDOOM_KEY_KEY_3 = 0x0033,
	GDDOOM_KEY_KEY_4 = 0x0034,
	GDDOOM_KEY_KEY_5 = 0x0035,
	GDDOOM_KEY_KEY_6 = 0x0036,
	GDDOOM_KEY_KEY_7 = 0x0037,
	GDDOOM_KEY_KEY_8 = 0x0038,
	GDDOOM_KEY_KEY_9 = 0x0039,
	GDDOOM_KEY_COLON = 0x003A,
	GDDOOM_KEY_SEMICOLON = 0x003B,
	GDDOOM_KEY_LESS = 0x003C,
	GDDOOM_KEY_EQUAL = 0x003D,
	GDDOOM_KEY_GREATER = 0x003E,
	GDDOOM_KEY_QUESTION = 0x003F,
	GDDOOM_KEY_AT = 0x0040,
	GDDOOM_KEY_A = 0x0041,
	GDDOOM_KEY_B = 0x0042,
	GDDOOM_KEY_C = 0x0043,
	GDDOOM_KEY_D = 0x0044,
	GDDOOM_KEY_E = 0x0045,
	GDDOOM_KEY_F = 0x0046,
	GDDOOM_KEY_G = 0x0047,
	GDDOOM_KEY_H = 0x0048,
	GDDOOM_KEY_I = 0x0049,
	GDDOOM_KEY_J = 0x004A,
	GDDOOM_KEY_K = 0x004B,
	GDDOOM_KEY_L = 0x004C,
	GDDOOM_KEY_M = 0x004D,
	GDDOOM_KEY_N = 0x004E,
	GDDOOM_KEY_O = 0x004F,
	GDDOOM_KEY_P = 0x0050,
	GDDOOM_KEY_Q = 0x0051,
	GDDOOM_KEY_R = 0x0052,
	GDDOOM_KEY_S = 0x0053,
	GDDOOM_KEY_T = 0x0054,
	GDDOOM_KEY_U = 0x0055,
	GDDOOM_KEY_V = 0x0056,
	GDDOOM_KEY_W = 0x0057,
	GDDOOM_KEY_X = 0x0058,
	GDDOOM_KEY_Y = 0x0059,
	GDDOOM_KEY_Z = 0x005A,
	GDDOOM_KEY_BRACKETLEFT = 0x005B,
	GDDOOM_KEY_BACKSLASH = 0x005C,
	GDDOOM_KEY_BRACKETRIGHT = 0x005D,
	GDDOOM_KEY_ASCIICIRCUM = 0x005E,
	GDDOOM_KEY_UNDERSCORE = 0x005F,
	GDDOOM_KEY_QUOTELEFT = 0x0060,
	GDDOOM_KEY_BRACELEFT = 0x007B,
	GDDOOM_KEY_BAR = 0x007C,
	GDDOOM_KEY_BRACERIGHT = 0x007D,
	GDDOOM_KEY_ASCIITILDE = 0x007E,
	GDDOOM_KEY_YEN = 0x00A5,
	GDDOOM_KEY_SECTION = 0x00A7,
} Key;

#endif /* DOOMINPUT_H */
