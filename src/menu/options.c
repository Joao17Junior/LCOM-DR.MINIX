#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdio.h>
#include "options.h"   

int current_mode = 0; // 0 para dark, 1 para light

int current_screen_size = 0; // 0 para 600p, 1 para 1024p

static const char *mode[] = {
    "dark mode",
    "light mode"
};
static int select_mode = 0; // 0 para dark, 1 para light
static const char *tile_type[] = {
    "both tiles",
    "short tiles"
};
int select_tile_type = 0; // 0 para both, 1 para short
static const char *screen_size[] = {
    "600p",
    "1024p"
};
static int select_screen_size = 0; // 0 para 600p, 1 para 1024p
static const char *difficulty[] = {
    "easy",
    "medium",
    "hard"
};
int select_difficulty = 0; // 0 para easy, 1 para medium, 2 para hard
static const char *opt[] = {
    "mode : ",
    "tile type : ",
    "screen size : ",
    "difficulty : ",
    "back"
};
static const int num_opt = 5;
static int select_opt = 0;
extern uint8_t scancode;
int speed = 0; // velocidade do jogo, 0 para fácil, 1 para médio, 2 para difícil
int spawn_interval_preset = 200;
int points_multiplier = 1; // multiplicador de pontos, 1 para fácil, 2 para médio, 3 para difícil


int (light_mode)() {
    current_mode = 1;
    return 0;
}

int (dark_mode)() {
    current_mode = 0;
    return 0;
}

int (set_tile_type)(int type) {return 0;}

int (set_screen_size)(int screen) {
    current_screen_size = screen;

    // 1. Free old back buffer
    if (back_buffer) free(back_buffer);

    // 2. Exit graphics mode
    vg_exit();

    // 3. Set new graphics mode
    uint16_t mode = (screen == 0) ? MODE_0 : MODE_1;
    if (set_graphic_mode(mode) != 0) {
        printf("Error setting screen size\n");
        return 1;
    }

    // 4. Update global mode_info
    if (vbe_get_mode_info(mode, &mode_info) != 0) {
        printf("Error getting mode info\n");
        return 1;
    }

    // 5. Set new framebuffer
    if (set_frame_buffer(mode) != 0) {
        printf("Error setting framebuffer\n");
        return 1;
    }

    // 6. Reallocate back buffer
    if (init_back_buffer() != 0) {
        printf("Error allocating back buffer\n");
        return 1;
    }

    return 0;
}

int (set_difficulty)(int difficulty) {
    select_difficulty = difficulty;
    switch (difficulty) {
        case 0: // easy
            speed = 0;
            spawn_interval_preset = 200;
            points_multiplier = 1;
            break;
        case 1: // medium
            speed = 1;
            spawn_interval_preset = 100; // more frequent
            points_multiplier = 2;
            break;
        case 2: // hard
            speed = 2;
            spawn_interval_preset = 60; // even more frequent
            points_multiplier = 5;
            break;
        default:
            printf("Invalid difficulty level\n");
            return 1;
    }
    return 0;
}

// --------

static int string_pixel_width(const char *str, uint8_t scale) {
    int sz = strlen(str) * 8 * scale;
    return sz;
}

int (draw_options)() {
    uint32_t bg_color = (current_mode == 1) ? LIGHT_BG_COLOR : DARK_BG_COLOR;
    uint32_t text_color = (current_mode == 1) ? LIGHT_TEXT_COLOR : DARK_TEXT_COLOR;
    uint32_t title_color = (current_mode == 1) ? LIGHT_TITLE_COLOR : DARK_TITLE_COLOR;
    uint32_t subtitle_color = (current_mode == 1) ? LIGHT_SUB_COLOR : DARK_SUB_COLOR; 

    if (mode_info.BitsPerPixel == 16) {
        bg_color = rgb_to_565(bg_color);
        text_color = rgb_to_565(text_color);
        title_color = rgb_to_565(title_color);
        subtitle_color = rgb_to_565(subtitle_color);
    }

    uint8_t scale = (current_screen_size == 0) ? 2 : 3; // scale 1 for 600p, scale 2 for 1024p
    
    vg_draw_rectangle(0, 0, mode_info.XResolution, mode_info.YResolution, bg_color); // limpar tela

    // --- Draw "Dr.Minix" title ---
    const char *title = "Dr.Minix";
    int title_scale = scale + 5;
    int title_width = string_pixel_width(title, title_scale);
    int title_x = (mode_info.XResolution - title_width) / 2;
    int title_y = mode_info.YResolution / 6; 
    draw_string(title, title_x, title_y, title_color, title_scale);

    // --- Draw "Tiles LAB - Options" subtitle ---
    const char *subtitle = "Tiles LAB - Options";
    int subtitle_scale = scale + 1;
    int subtitle_width = string_pixel_width(subtitle, subtitle_scale);
    int subtitle_x = (mode_info.XResolution - subtitle_width) / 2;
    int subtitle_y = mode_info.YResolution / 4 + 10;
    draw_string(subtitle, subtitle_x, subtitle_y, subtitle_color, subtitle_scale);
     
    // --- Draw menu options ---
    int y = mode_info.YResolution / 2; // posição vertical do menu
    for (int i = 0; i < num_opt; i++) {
        uint32_t color;
        if (current_mode == 0) { // dark mode
            color = (i == select_opt) ? text_color : DARK_UNSEL_COLOR;
        } else {
            color = (i == select_opt) ? text_color : LIGHT_UNSEL_COLOR;
        }

        if (mode_info.BitsPerPixel == 16 && i != select_opt) {
            color = rgb_to_565(color);
        }

        char display[64];
        
        if (i == 0 && select_opt == 0) {
            snprintf(display, sizeof(display), ">%s%s<", opt[i], mode[select_mode]);
        } else if (i == 0 && select_opt != 0) {
            snprintf(display, sizeof(display), "%s%s", opt[i], mode[select_mode]);
        } else if (i == 1 && select_opt == 1) {
            snprintf(display, sizeof(display), ">%s%s<", opt[i], tile_type[select_tile_type]);
        } else if (i == 1 && select_opt != 1) {
            snprintf(display, sizeof(display), "%s%s", opt[i], tile_type[select_tile_type]);
        } else if (i == 2 && select_opt == 2) {
            snprintf(display, sizeof(display), ">%s%s<", opt[i], screen_size[select_screen_size]);
        } else if (i == 2 && select_opt != 2) {
            snprintf(display, sizeof(display), "%s%s", opt[i], screen_size[select_screen_size]);
        } else if (i == 3 && select_opt == 3) {
            snprintf(display, sizeof(display), ">%s%s<", opt[i], difficulty[select_difficulty]);
        } else if (i == 3 && select_opt != 3) {
            snprintf(display, sizeof(display), "%s%s", opt[i], difficulty[select_difficulty]);
        } else if (i == 4 && select_opt == 4) {
            snprintf(display, sizeof(display), ">%s<", opt[i]);
        } else {
            snprintf(display, sizeof(display), "%s", opt[i]);
        }

        int text_width = string_pixel_width(display, scale);
        int x = (mode_info.XResolution - text_width) / 2;
        draw_string(display, x, y, color, scale);
        y += (mode_info.YResolution / 2) / (num_opt + 1); // espaçamento entre opções
    }
    return 0;
}

int (options_loop)(uint8_t keyboard_bit, message msg, int ipc_status) {
    if (draw_options() != 0) { // desenhar menu inicial
        printf("Error drawing menu\n");
        vg_exit();
        return 1;
    } 

    int r = 0;
    while (1) {
        r = driver_receive(ANY, &msg, &ipc_status);
        if (r != 0) {
            printf("driver_receive failed with: %d\n", r);
            continue;
        }
        if(is_ipc_notify(ipc_status)){
            switch(_ENDPOINT_P(msg.m_source)){
                case HARDWARE:
                    if(msg.m_notify.interrupts & BIT(keyboard_bit)){
                        kbc_ih(); 
                        printf("Key pressed: 0x%X\n", scancode); // ler scancode
                        if ((scancode & 0x80) == 0) { // Tecla pressionada
                            if (scancode == 0x01) { // ESC
                                return 0;
                            }
                            else if (scancode == 0x11 || scancode == 0x48) { // W key or Up Arrow
                                if (select_opt > 0) select_opt--;
                                draw_options();
                            }
                            else if (scancode == 0x1F || scancode == 0x50) { // S key or Down Arrow
                                if (select_opt < num_opt - 1) select_opt++;
                                draw_options();
                            }
                            else if (scancode == 0x1C) { // Enter key
                                if (select_opt == 0) {
                                    select_mode = (select_mode + 1) % 2; // alternar entre light e dark
                                    if (select_mode == 1) light_mode();
                                    else dark_mode();
                                    draw_options();
                
                                } else if (select_opt == 1) {
                                    select_tile_type = (select_tile_type + 1) % 2; // alternar entre short e both tiles
                                    //set_tile_type(select_tile_type);
                                    draw_options();

                                } else if (select_opt == 2) {
                                    select_screen_size = (select_screen_size + 1) % 2; // alternar entre 800x600 e 1280x720
                                    if (select_screen_size == 0) set_screen_size(0); // 600p
                                    else set_screen_size(1); // 1024p
                                    draw_options();

                                } else if (select_opt == 3) {
                                    select_difficulty = (select_difficulty + 1) % 3; // alternar entre easy, medium e hard
                                    set_difficulty(select_difficulty);
                                    draw_options();
                                } else if (select_opt == 4) {
                                    return 0; // voltar ao menu principal
                                }
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }
    }

    return 0;
}
