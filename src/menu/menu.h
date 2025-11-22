//menu com start, options e exit
#ifndef _MENU_H_
#define _MENU_H_

#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdio.h>
#include "options.h"
#include "../video/graphics.h"
#include "../keyboard/keyboard.h"

extern int current_mode; // 0 para dark, 1 para light
extern int current_screen_size; // 0 para 600p, 1 para 1024p

int (draw_menu)();
int (menu_loop)();

#endif // _MENU_H_
