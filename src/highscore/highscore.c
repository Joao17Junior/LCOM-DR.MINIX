#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "highscore.h"
#include "../game/game.h"
#include "../menu/options.h" // for select_difficulty

int high_score = 0; // pontuação mais alta
int high_diff = 0; // dificuldade da pontuação mais alta
extern int select_difficulty; // dificuldade selecionada do jogo
static const char *difficulty[] = {
    "easy",
    "medium",
    "hard"
};

extern int current_mode;
extern int current_screen_size;
extern vbe_mode_info_t mode_info;
extern uint8_t *back_buffer;
extern uint8_t *frame_buffer;
extern int BytesPerPixel;

static int string_pixel_width(const char *str, uint8_t scale) {
    int sz = strlen(str) * 8 * scale;
    return sz;
}

int (highscore_rec)(int score) {
    if (score > high_score) {
        high_score = score;
        high_diff = select_difficulty;
    }
    return 0;
}

// Draw text into the back-buffer directly (needs implementation in graphics.c)

int (draw_score)(int score) {
    uint32_t text_color = (current_mode == 1) ? HIGHSCORE_LIGHT_SCORE_TEXT : HIGHSCORE_DARK_SCORE_TEXT;

    if (mode_info.BitsPerPixel == 16) {
        text_color = rgb_to_565(text_color);
    }

    uint8_t scale = (current_screen_size == 0) ? 1 : 2;

    char score_str[64];
    snprintf(score_str, sizeof(score_str), "Score: %d", score);
    int score_text_width = string_pixel_width(score_str, scale);
    int x = (mode_info.XResolution - score_text_width) / 2;
    int y = 5; // near top of screen
    int rect_h = (8 * scale) + 4;          // font height + padding
    int rect_w = score_text_width + 8;     // text width + padding

    // Clear score area in back-buffer
    uint32_t bg_color = (current_mode == 1) ? GAME_LIGHT_BG_COLOR : GAME_DARK_BG_COLOR;
    if (mode_info.BitsPerPixel == 16)
        bg_color = rgb_to_565(bg_color);
        
    unsigned BytesPerPixel = (mode_info.BitsPerPixel + 7) / 8;
    for (int dy = 0; dy < rect_h; dy++) {
        for (int dx = 0; dx < rect_w; dx++) {
            int px = x - 4 + dx;
            int py = y - 2 + dy;
            int index = (py * mode_info.XResolution + px) * BytesPerPixel;
            memcpy(&back_buffer[index], &bg_color, BytesPerPixel);
        }
    }

    // Draw score string into back-buffer only
    draw_string_backbuffer(score_str, x, y, text_color, scale);

    // Copy only the score region from back_buffer into frame_buffer
    for (int dy = 0; dy < rect_h; dy++) {
        int py = y - 2 + dy;
        int row_offset = py * mode_info.XResolution * BytesPerPixel;
        memcpy(
            &frame_buffer[row_offset + (x - 4) * BytesPerPixel],
            &back_buffer[row_offset + (x - 4) * BytesPerPixel],
            rect_w * BytesPerPixel
        );
    }

    return 0;
}

int (draw_timer)(int seconds) {
    uint32_t text_color = (current_mode == 1) ? HIGHSCORE_LIGHT_TIMER_TEXT : HIGHSCORE_DARK_TIMER_TEXT;
    if (mode_info.BitsPerPixel == 16) text_color = rgb_to_565(text_color);
    uint8_t scale = (current_screen_size == 0) ? 1 : 2;

    char timer_str[32];
    snprintf(timer_str, sizeof(timer_str), "Time: %02d:%02d", seconds / 60, seconds % 60);

    int timer_text_width = string_pixel_width(timer_str, scale);
    int x = (mode_info.XResolution - timer_text_width) / 2;
    int y = 5 + (8 * scale) + 8; // below the score

    // Optional: clear timer area (similar to draw_score)
    int rect_h = (8 * scale) + 4;
    int rect_w = timer_text_width + 8;
    uint32_t bg_color = (current_mode == 1) ? GAME_LIGHT_BG_COLOR : GAME_DARK_BG_COLOR;
    if (mode_info.BitsPerPixel == 16) bg_color = rgb_to_565(bg_color);
    unsigned BytesPerPixel = (mode_info.BitsPerPixel + 7) / 8;
    for (int dy = 0; dy < rect_h; dy++) {
        for (int dx = 0; dx < rect_w; dx++) {
            int px = x - 4 + dx;
            int py = y - 2 + dy;
            int index = (py * mode_info.XResolution + px) * BytesPerPixel;
            memcpy(&back_buffer[index], &bg_color, BytesPerPixel);
        }
    }

    draw_string_backbuffer(timer_str, x, y, text_color, scale);

    // Copy only the timer region from back_buffer into frame_buffer
    for (int dy = 0; dy < rect_h; dy++) {
        int py = y - 2 + dy;
        int row_offset = py * mode_info.XResolution * BytesPerPixel;
        memcpy(
            &frame_buffer[row_offset + (x - 4) * BytesPerPixel],
            &back_buffer[row_offset + (x - 4) * BytesPerPixel],
            rect_w * BytesPerPixel
        );
    }
    return 0;
}


int (draw_highscore)() {
    uint32_t h1_color    = (current_mode == 1) ? LIGHT_TITLE_COLOR : DARK_TITLE_COLOR;
    uint32_t bg_color    = (current_mode == 1) ? LIGHT_BG_COLOR : DARK_BG_COLOR;
    uint32_t text_color  = (current_mode == 1) ? LIGHT_TEXT_COLOR : DARK_TEXT_COLOR;
    uint32_t sub_color   = (current_mode == 1) ? LIGHT_SUB_COLOR : DARK_SUB_COLOR;

    if (mode_info.BitsPerPixel == 16) {
        bg_color   = rgb_to_565(bg_color);
        text_color = rgb_to_565(text_color);
        sub_color  = rgb_to_565(sub_color);
    }

    uint8_t scale = (current_screen_size == 0) ? 2 : 3; // scale 2 for 600p, 3 for 1024p

    vg_draw_rectangle(0, 0, mode_info.XResolution, mode_info.YResolution, bg_color); // clear screen

    // --- Draw "Highscore" title ---
    const char *highscore_text = "High Score!";
    int highscore_scale = scale + 3;
    int highscore_text_width = string_pixel_width(highscore_text, highscore_scale);
    int x = (mode_info.XResolution - highscore_text_width) / 2;
    int y = mode_info.YResolution / 6;
    draw_string(highscore_text, x, y, h1_color, highscore_scale);

    // --- Draw high score ---
    char score_text[64];
    snprintf(score_text, sizeof(score_text), "%d Points", high_score);
    int score_scale = scale + 5;
    int score_text_width = string_pixel_width(score_text, score_scale);
    x = (mode_info.XResolution - score_text_width) / 2;
    y = mode_info.YResolution / 2 - (score_scale * 8); // center vertically
    draw_string(score_text, x, y, text_color, score_scale);

    // --- Draw difficulty ---
    char diff_text[64];
    snprintf(diff_text, sizeof(diff_text), "Difficulty: %s", difficulty[high_diff]);
    int diff_scale = scale + 1;
    int diff_text_width = string_pixel_width(diff_text, diff_scale);
    x = (mode_info.XResolution - diff_text_width) / 2;
    y += mode_info.YResolution / 6;
    draw_string(diff_text, x, y, text_color, diff_scale);

    // --- Draw "Press any key to continue..." at the bottom ---
    const char *cont_text = "Press any key to continue...";
    int cont_scale = scale - 1; // smaller font
    if (cont_scale < 1) cont_scale = 1;
    int cont_text_width = string_pixel_width(cont_text, cont_scale);
    x = (mode_info.XResolution - cont_text_width) / 2;
    y = mode_info.YResolution - (cont_scale * 12) - 10; // 10px from bottom
    draw_string(cont_text, x, y, sub_color, cont_scale);

    return 0;
}

int (highscore_loop)() {
    if (draw_highscore() != 0) { // desenhar menu inicial
        printf("Error drawing menu\n");
        return 1;
    } 

    uint8_t keyboard_bit;
    message msg;
    int ipc_status;
    if(keyboard_subscribe_interrupts(&keyboard_bit) != 0) {
        return 1;
    }
    int leave = 0; // flag para sair do loop
    int r = 0;
    while (leave != 1) {
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
                            leave = 1; // sair do loop ao pressionar qualquer tecla
                        }
                    }
                    break;
                default:
                    break;
            }
        }
        
    }

    //limpar (desinscrever interrupçoes e repor modo texto)
    keyboard_unsubscribe_interrupts();

    leave = 0;
    return 0; // iniciar jogo
}
