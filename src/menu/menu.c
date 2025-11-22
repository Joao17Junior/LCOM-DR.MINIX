#include "menu.h"
#include "options.h"
#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdio.h>

int select_opt = 0;
static const char *opt[] = {
    "start game",
    "options",
    "exit"
};
static const int num_opt = 3;
extern uint8_t scancode;
static int leave = 0;
static int game_start = 0;

static int string_pixel_width(const char *str, uint8_t scale) {
    int sz = strlen(str) * 8 * scale;
    return sz;
}

int (draw_menu)() {
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

    // --- Draw "Tiles LAB" subtitle ---
    const char *subtitle = "Tiles LAB";
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
        if (i == select_opt) {
            snprintf(display, sizeof(display), ">%s<", opt[i]);
        } else {
            snprintf(display, sizeof(display), "%s", opt[i]);
        }
        int text_width = string_pixel_width(display, scale);
        int x = (mode_info.XResolution - text_width) / 2;
        draw_string(display, x, y, color, scale);
        y += (mode_info.YResolution / 2) / (num_opt + 1);
    }
    return 0;
}

int (menu_loop)() {
    if (draw_menu() != 0) { // desenhar menu inicial
        printf("Error drawing menu\n");
        return 2;
    } 

    uint8_t keyboard_bit;
    message msg;
    int ipc_status;
    if(keyboard_subscribe_interrupts(&keyboard_bit) != 0) {
        return 2;
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
                                leave = 1;
                                break; // sair do loop do menu
                            }
                            else if (scancode == 0x11 || scancode == 0x48) { // W key or Up Arrow
                                if (select_opt > 0) select_opt--;
                                draw_menu();
                            }
                            else if (scancode == 0x1F || scancode == 0x50) { // S key or Down Arrow
                                if (select_opt < num_opt - 1) select_opt++;
                                draw_menu();
                            }
                            else if (scancode == 0x1C) { // Enter key
                                if (select_opt == 0) {
                                    game_start = 1; // iniciar jogo
                                    break; // sair do loop do menu
                                } else if (select_opt == 1) {
                                    if (options_loop(keyboard_bit, msg, ipc_status) != 0) { // abrir menu de opções
                                        break;
                                    } 
                                    draw_menu(); // redesenhar menu ao voltar
                                } else if (select_opt == 2) {
                                    leave = 1; // sair do jogo
                                    break; // sair do loop do menu
                                }
                            }
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        if (leave||game_start) break;
    }

    //limpar (desinscrever interrupçoes e repor modo texto)
    keyboard_unsubscribe_interrupts();
    
    if (leave == 1) {
        printf("Exiting menu...\n");
        leave = 0; // resetar leave para o próximo uso
        return 1; // sair do menu
    }
    leave = 0;
    game_start = 0; // resetar game_start para o próximo uso
    return 0; // iniciar jogo
}
