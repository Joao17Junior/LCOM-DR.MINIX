// light and dark mode, type of tile only (long, short or both), screen size;
#ifndef _OPTIONS_H_
#define _OPTIONS_H_

#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdio.h>
#include "../video/graphics.h"
#include "../keyboard/keyboard.h"
#include "../video/VBE.h"
#include "../conf.h"

#define LIGHT_BG_COLOR    0xF4F6FB  // Very light blue-gray
#define LIGHT_TEXT_COLOR  0x22223B  // Very dark blue
#define LIGHT_SUB_COLOR   0x4B4E6D  // Muted dark blue
#define LIGHT_TITLE_COLOR 0x3A86FF  // Vivid blue
#define LIGHT_UNSEL_COLOR 0xA0A4B8  // Muted blue-gray (for unselected options)
#define DARK_BG_COLOR     0x1A1A1A  // Deep dark gray
#define DARK_TEXT_COLOR   0xF4F6FB  // Very light blueish gray
#define DARK_SUB_COLOR    0xA0A4B8  // Muted blue-gray
#define DARK_TITLE_COLOR  0x3A86FF  // Vivid blue
#define DARK_UNSEL_COLOR  0x606060  // Medium gray (for unselected options)

int (dark_mode) ();
int (light_mode) ();
int (set_tile_type)(int type);
int (set_screen_size)(int screen);
int (set_difficulty)(int difficulty);
int (draw_options)();
int (options_loop)(uint8_t keyboard_bit, message msg, int ipc_status);

#endif // _OPTIONS_H_
