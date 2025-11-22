#include "video/graphics.h"
#include "video/VBE.h"
#include "tile.h"
#include "conf.h"
#include "keyboard/keyboard.h"
#include <stdlib.h>
#include <string.h>

extern int current_mode;
extern int current_screen_size;

Note current_note;
const uint8_t NOTE_SCANCODES[NUM_KEYS] = {0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24};

uint8_t* back_buffer;
size_t buffer_size;

bool key_pressed[NUM_KEYS] = {false};

void draw_pixel_to_buffer(uint8_t* buffer, uint16_t x, uint16_t y, uint32_t color) {
    if (x >= mode_info.XResolution || y >= mode_info.YResolution) return;

    unsigned bpp = (mode_info.BitsPerPixel + 7) / 8;
    unsigned int index = (y * mode_info.XResolution + x) * bpp;
    memcpy(&buffer[index], &color, bpp);
}

void draw_rectangle_to_buffer(uint8_t* buffer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            draw_pixel_to_buffer(buffer, x + j, y + i, color);
        }
    }
}

void clear_buffer() {
    uint32_t bg_color = (current_mode == 1) ? GAME_LIGHT_BG_COLOR : GAME_DARK_BG_COLOR; // white for light mode, black for dark mode
    if (mode_info.BitsPerPixel == 16) {
        bg_color = rgb_to_565(bg_color);
    }
    draw_rectangle_to_buffer(back_buffer, 0, 0, mode_info.XResolution, mode_info.YResolution, bg_color);
}


void draw_note_to_buffer(Note* note) {
    if (!note->active || note->height <= 0) return;

    int16_t draw_y = note->y;
    int16_t draw_height = note->height;
    int newX = (current_screen_size == 0) ? note->x + 2 : note->x + 4; // padding of 1 pixel
    int border_thickness = 3;

    if (note->is_long) {
        uint32_t note_color, note_border_color;
        
        if (!note->being_held) {
            note_color = (current_mode == 1) ? GAME_LIGHT_LONG_NOTE_COLOR : GAME_DARK_LONG_NOTE_COLOR;
            note_border_color = (current_mode == 1) ? GAME_LIGHT_LONG_NOTE_BORDER : GAME_DARK_LONG_NOTE_BORDER;
        } else {
            note_color = (current_mode == 1) ? GAME_LIGHT_LONG_NOTE_HELD : GAME_DARK_LONG_NOTE_HELD;
            note_border_color = (current_mode == 1) ? GAME_LIGHT_LONG_NOTE_BORDER : GAME_DARK_LONG_NOTE_BORDER;
        }
       

        if (mode_info.BitsPerPixel == 16) {
            note_color = rgb_to_565(note_color);
            note_border_color = rgb_to_565(note_border_color);
        }

        if (note->being_held) {
            // Shrink from the bottom up (hit line)
            int16_t hit_line_y = mode_info.YResolution - KEY_HEIGHT - NOTE_HEIGHT;
            int16_t visible_y = hit_line_y - draw_height + NOTE_HEIGHT;
            int16_t visible_height = draw_height;
            if (visible_y < 0) {
                visible_height += visible_y;
                visible_y = 0;
            }
            if (visible_y + visible_height > mode_info.YResolution)
                visible_height = mode_info.YResolution - visible_y;
            if (visible_height > 0) {
                // Draw border
                draw_rectangle_to_buffer(back_buffer, newX, visible_y, NOTE_WIDTH - 2, visible_height, note_border_color);
                // Draw inner note
                if (visible_height > 2 * border_thickness) {
                    draw_rectangle_to_buffer(
                        back_buffer,
                        newX + border_thickness,
                        visible_y + border_thickness,
                        NOTE_WIDTH - 2 - 2 * border_thickness,
                        visible_height - 2 * border_thickness,
                        note_color
                    );
                }
            }
        } else {
            // Normal falling logic
            if (draw_y < 0) return; // not visible yet

            if (draw_y < draw_height) {
                // Growing in: draw from top, height = draw_y
                int16_t visible_y = 0;
                int16_t visible_height = draw_y;
                if (visible_height > mode_info.YResolution)
                    visible_height = mode_info.YResolution;
                if (visible_height > 0) {
                    // Draw border
                    draw_rectangle_to_buffer(back_buffer, newX, visible_y, NOTE_WIDTH - 2, visible_height, note_border_color);
                    // Draw inner note
                    if (visible_height > 2 * border_thickness) {
                        draw_rectangle_to_buffer(
                            back_buffer,
                            newX + border_thickness,
                            visible_y + border_thickness,
                            NOTE_WIDTH - 2 - 2 * border_thickness,
                            visible_height - 2 * border_thickness,
                            note_color
                        );
                    }
                }
            } else {
                // Fully loaded: draw at its position
                int16_t visible_y = draw_y - draw_height;
                int16_t visible_height = draw_height;
                if (visible_y + visible_height > mode_info.YResolution)
                    visible_height = mode_info.YResolution - visible_y;
                if (visible_y < mode_info.YResolution && visible_height > 0) {
                    // Draw border
                    draw_rectangle_to_buffer(back_buffer, newX, visible_y, NOTE_WIDTH - 2, visible_height, note_border_color);
                    // Draw inner note
                    if (visible_height > 2 * border_thickness) {
                        draw_rectangle_to_buffer(
                            back_buffer,
                            newX + border_thickness,
                            visible_y + border_thickness,
                            NOTE_WIDTH - 2 - 2 * border_thickness,
                            visible_height - 2 * border_thickness,
                            note_color
                        );
                    }
                }
            }
        }
    } else {
        // Short note: original logic
        if (draw_y >= mode_info.YResolution) return; // completely below

        uint32_t note_color = (current_mode == 1) ? GAME_LIGHT_NOTE_COLOR : GAME_DARK_NOTE_COLOR;
        uint32_t note_border_color = (current_mode == 1) ? GAME_LIGHT_NOTE_BORDER : GAME_DARK_NOTE_BORDER;
        if (mode_info.BitsPerPixel == 16) {
            note_color = rgb_to_565(note_color);
            note_border_color = rgb_to_565(note_border_color);
        }

        // Draw border
        draw_rectangle_to_buffer(back_buffer, newX, note->y, NOTE_WIDTH - 2, NOTE_HEIGHT, note_border_color);

        // Draw inner note (smaller, centered)
        draw_rectangle_to_buffer(
            back_buffer,
            newX + border_thickness,
            note->y + border_thickness,
            NOTE_WIDTH - 2 - 2 * border_thickness,
            NOTE_HEIGHT - 2 * border_thickness,
            note_color
        );
    }
}

void draw_piano_keys() {
    uint16_t x_start = (current_screen_size == 0) ? 2 : 4;
    uint16_t y_start = mode_info.YResolution - KEY_HEIGHT;
    uint16_t white_key_width = mode_info.XResolution / NUM_KEYS;
    int border_thickness = 2;

    // Draw white keys (base)
    for (int i = 0; i < NUM_KEYS; i++) {
        uint32_t key_color, border_color;
        if (key_pressed[i]) {
            key_color = 0x808080; // grey when pressed
            border_color = 0x606060;
        } else {
            key_color = (current_mode == 1) ? GAME_LIGHT_KEY_COLOR : GAME_DARK_KEY_COLOR;
            border_color = (current_mode == 1) ? GAME_LIGHT_KEY_BORDER : GAME_DARK_KEY_BORDER;
        }
        if (mode_info.BitsPerPixel == 16) {
            key_color = rgb_to_565(key_color);
            border_color = rgb_to_565(border_color);
        }
        uint16_t key_x = x_start + i * white_key_width;
        // Draw border
        draw_rectangle_to_buffer(back_buffer, key_x, y_start, white_key_width - 2, KEY_HEIGHT, border_color);
        // Draw inner key
        draw_rectangle_to_buffer(
            back_buffer,
            key_x + border_thickness,
            y_start + border_thickness,
            white_key_width - 2 - 2 * border_thickness,
            KEY_HEIGHT - 2 * border_thickness,
            key_color
        );
    }

    // Draw black keys (overlay)
    // For 7 keys: black keys between 0-1, 1-2, 3-4, 4-5, 5-6 (skip between 2-3 and 6-0)
    y_start += KEY_HEIGHT / 3; // Adjust y_start for black keys
    int black_key_indices[] = {0, 1, 3, 4, 5};
    int num_black_keys = sizeof(black_key_indices) / sizeof(int);
    uint16_t black_key_width = white_key_width * 2 / 3;
    uint16_t black_key_height = KEY_HEIGHT * 2 / 3;
    uint32_t black_color = (mode_info.BitsPerPixel == 16) ? rgb_to_565(0x22223B) : 0x22223B;
    uint32_t black_border = (mode_info.BitsPerPixel == 16) ? rgb_to_565(0x606060) : 0x606060;

    for (int i = 0; i < num_black_keys; i++) {
        int idx = black_key_indices[i];
        uint16_t key_x = x_start + (idx + 1) * white_key_width - black_key_width / 2;
        // Draw border
        draw_rectangle_to_buffer(back_buffer, key_x, y_start, black_key_width, black_key_height, black_border);
        // Draw inner black key
        draw_rectangle_to_buffer(
            back_buffer,
            key_x + 1,
            y_start + 1,
            black_key_width - 2,
            black_key_height - 2,
            black_color
        );
    }
}

bool check_note_hit(Note* note, uint8_t scancode) {
    if (!note->active) return false;

    uint16_t hit_line_y = mode_info.YResolution - KEY_HEIGHT - NOTE_HEIGHT;

    if (!note->is_long) {
        if (note->y <= hit_line_y && note->y + note->height >= hit_line_y) {
            if (scancode == NOTE_SCANCODES[note->key_index]) {
                note->active = false;
                return true;
            }
        }
    } else {
        // LONG NOTE HIT START
        if (note->y > hit_line_y && note->y < (mode_info.YResolution - KEY_HEIGHT+10) && !note->being_held) {
            if (scancode == NOTE_SCANCODES[note->key_index]) {
                note->being_held = true;  // Start shrinking
                return true;
            }
            // else {note->being_held = false; // Reset if wrong key pressed
            // }
        }
    }

    return false;
}

bool check_note_miss(Note* note) {
    if (!note->active) return false;

    uint16_t hit_line_y = mode_info.YResolution - KEY_HEIGHT - NOTE_HEIGHT;

    if (note->is_long) {
        // Ignore if completed
        if (note->completed) return false;

        // If user stops holding before the note has completely passed the hit line — miss
        uint16_t note_bottom = note->y + note->height;
        if (note->being_held && !key_pressed[note->key_index] && note_bottom < hit_line_y + note->height) {
            note->active = false;
            return true;
        }

        // If the note passes hit area without being held — miss
        if (note->y > mode_info.YResolution - KEY_HEIGHT && !note->being_held) {
            note->active = false;
            return true;
        }

        return false;
    } else {
        // Short note: miss if passes
        if (note->y > hit_line_y) {
            note->active = false;
            return true;
        }
        return false;
    }
}



uint16_t get_long_tile_height() {
    uint16_t base = mode_info.YResolution / 5;
    uint16_t max = 3 * base; // 100% bigger
    return base + (rand() % (max - base + 1));
}

void init_note(Note* note, int key, bool is_long, uint16_t long_height) {
    uint16_t dynamic_key_width = mode_info.XResolution / NUM_KEYS;
    note->x = key * dynamic_key_width;
    note->y = 0;
    note->active = true;
    note->key_index = key;
    note->is_long = is_long;
    note->height = is_long ? long_height : NOTE_HEIGHT;
    note->being_held = false;
    note->completed = false;
    // Set score for the note
    if (is_long) {
        int scaled = (note->height * 10) / NOTE_HEIGHT;
        note->tilescore = (scaled < 20) ? 20 : scaled;
    } else {
        note->tilescore = 10;
    }
}

extern int speed;
int get_long_note_shrink_speed() {
    switch (speed) {
        case 0: return NOTE_HEIGHT / 5; // easy
        case 1: return NOTE_HEIGHT / 3;  // medium
        case 2: return NOTE_HEIGHT / 2;  // hard
        default: return NOTE_HEIGHT / 10;
    }
}

void update_note(Note* note) {
    if (!note->active) return;

    if (note->is_long && note->being_held) {
        int shrink_speed = get_long_note_shrink_speed(); // Use dynamic shrink speed
        // Shrink the note
        if (note->height > 0) {
            if (note->height > shrink_speed)
                note->height -= shrink_speed;
            else
                note->height = 0;
        }
        // If fully shrunk, mark as completed and deactivate
        if (note->height == 0) {
            key_pressed[note->key_index] = false; // Release the key BEFORE marking inactive
            note->completed = true;
            note->being_held = false; // Prevent miss logic from triggering
            note->active = false;
            // Add score for completed long note
            add_score(note->tilescore);
        }
    } else {
        // Only move if NOT being held
        if (!(note->is_long && note->being_held)) {
            switch (speed) {
                case 0: note->y += NOTE_HEIGHT / 5; break;
                case 1: note->y += NOTE_HEIGHT / 3; break;
                case 2: note->y += NOTE_HEIGHT / 2; break;
            }
        }

        if (!note->is_long && note->y >= mode_info.YResolution) {
            note->active = false;
        }
    }
}

void vg_copy_buffer_to_video() {
    memcpy(frame_buffer, back_buffer, buffer_size);
}

void init_note_queue(NoteQueue* q) {
    q->front = 0;
    q->rear = 0;
    q->size = 0;
}

extern int select_tile_type;
int enqueue_note(NoteQueue* q, int key, bool is_long, uint16_t long_height) {
    if (q->size == MAX_NOTES) return -1; // queue full
    init_note(&q->notes[q->rear], key, is_long, long_height);
    q->rear = (q->rear + 1) % MAX_NOTES;
    q->size++;
    return 0;
}




int update_notes(NoteQueue* q) {

    int removed = 0;
    int count = q->size;
    for (int i = 0; i < count; i++) {
        int idx = (q->front + i) % MAX_NOTES;
        update_note(&q->notes[idx]);
    }
    // Remove inactive notes from the front
    while (q->size > 0 && !q->notes[q->front].active) {
        q->front = (q->front + 1) % MAX_NOTES;
        q->size--;
        removed++;
    }
    return removed;
}

int draw_notes(NoteQueue* q) {
    int count = q->size;
    for (int i = 0; i < count; i++) {
        int idx = (q->front + i) % MAX_NOTES;
        draw_note_to_buffer(&q->notes[idx]);
    }
    return 0;
}

int check_note_hit_queue(NoteQueue* q, uint8_t scancode) {
    if (q->size == 0) return 0;
    Note* note = &q->notes[q->front];
    if (check_note_hit(note, scancode)) {
        if (!note->is_long) note->active = false; // Only deactivate short notes here
        return 1;
    }
    return 0;
}




void draw_hit_line() {
    uint16_t y = mode_info.YResolution - KEY_HEIGHT - NOTE_HEIGHT;
    uint32_t line_color = (current_mode == 1) ? GAME_LIGHT_HIT_LINE : GAME_DARK_HIT_LINE; // red color
    if (mode_info.BitsPerPixel == 16) {
        line_color = rgb_to_565(line_color);
    }
    draw_rectangle_to_buffer(back_buffer, 0, y, mode_info.XResolution, 2, line_color); // red line, 2px height
}

bool is_note_scancode(uint8_t scancode) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (scancode == NOTE_SCANCODES[i]) return true;
    }
    return false;
}

void reset_key_pressed() {
    for (int i = 0; i < NUM_KEYS; i++) key_pressed[i] = false;
}

int scancode_to_key_index(uint8_t scancode) {
    for (int i = 0; i < NUM_KEYS; i++) {
        if (scancode == NOTE_SCANCODES[i]) return i;
    }
    return -1; // Not found
}

void add_score(int base_score) {
    extern int score;
    extern int points_multiplier;
    score += base_score * points_multiplier;
}

// Returns true if there is any note in this column that is still spawning in (top not visible)
bool column_has_spawning_note(NoteQueue* q, int new_note_y, int new_note_height) {
    int count = q->size;
    int new_note_bottom = new_note_y + new_note_height;
    for (int i = 0; i < count; i++) {
        int idx = (q->front + i) % MAX_NOTES;
        Note* note = &q->notes[idx];
        if (!note->active) continue;
        int note_top = note->y;
        int note_bottom = note->y + note->height;
        // Check for overlap
        if (!(new_note_bottom <= note_top - KEY_HEIGHT || new_note_y >= note_bottom)) {
            return true;
        }
    }
    return false;
}
