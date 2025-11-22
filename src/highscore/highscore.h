#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdio.h>
#include "../timer/timer.h"
#include "../keyboard/keyboard.h"
#include "../keyboard/KBC.h"
#include "../video/graphics.h"
#include "../video/VBE.h"
#include "../tile.h"
#include "../conf.h"


#define HIGHSCORE_LIGHT_SCORE_TEXT    0x22223B  // Main text color
#define HIGHSCORE_LIGHT_TIMER_TEXT    0x4B4E6D  // Muted dark blue

#define HIGHSCORE_DARK_SCORE_TEXT     0xF4F6FB  // Light text color
#define HIGHSCORE_DARK_TIMER_TEXT     0xA0A4B8  // Muted blue-gray for timer

int (highscore_rec)(int score);
int (draw_score)(int score);
int (draw_timer)(int seconds);
int (draw_highscore)();
int (highscore_loop)();

