#ifndef __GRAPHICS_H
#define __GRAPHICS_H

#include <lcom/lcf.h>
#include "VBE.h"

extern vbe_mode_info_t mode_info;
extern uint8_t* frame_buffer;
extern uint8_t* back_buffer;
extern size_t buffer_size;

int (init_back_buffer)();

int (set_graphic_mode)(uint16_t submode);
int (set_text_mode)();
int (set_frame_buffer)(uint16_t mode);
int (vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color);
int (vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);
int (print_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y);
int (draw_char)(char c, uint16_t x, uint16_t y, uint32_t color, uint8_t scale);
int (draw_string)(const char *str, uint16_t x, uint16_t y, uint32_t color, uint8_t scale);
int (draw_pixel_to_backbuffer)(uint16_t x, uint16_t y, uint32_t color);
int (draw_string_backbuffer)(const char *str, int x, int y, uint32_t color, uint8_t scale);
int (normalize_color)(uint32_t color, uint32_t *new_color);
uint32_t (rgb_to_565)(uint32_t color);
uint32_t (direct_mode)(uint32_t R, uint32_t G, uint32_t B);
uint32_t (indexed_mode)(uint16_t col, uint16_t row, uint8_t step, uint32_t first, uint8_t n);
uint32_t (Red)(unsigned j, uint8_t step, uint32_t first);
uint32_t (Green)(unsigned i, uint8_t step, uint32_t first);
uint32_t (Blue)(unsigned j, unsigned i, uint8_t step, uint32_t first);
uint32_t (R)(uint32_t first);
uint32_t (G)(uint32_t first);
uint32_t (B)(uint32_t first);

#endif /* __GRAPHICS_H */
