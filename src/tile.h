#ifndef TILE_H
#define TILE_H

#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdint.h>
#include "menu/options.h"

// --- Light Mode ---
#define GAME_LIGHT_BG_COLOR      0xF4F6FB  // Matches menu light bg
#define GAME_LIGHT_KEY_COLOR     0xE5E7EB  // Light gray for white keys (contrast)
#define GAME_LIGHT_KEY_BORDER    0xA0A4B8  // Soft gray border
#define GAME_LIGHT_BLACK_KEY     0x1A1A1A  // Deep dark gray for black keys
#define GAME_LIGHT_NOTE_COLOR    0x3A86FF  // Vivid blue notes
#define GAME_LIGHT_NOTE_BORDER   0x22223B  // Dark blue border for notes~
#define GAME_LIGHT_LONG_NOTE_COLOR 0xFFA500  // Orange for long notes
#define GAME_LIGHT_LONG_NOTE_BORDER 0xE5CFA6  // Greyish orange border for long notes
#define GAME_LIGHT_LONG_NOTE_HELD 0xE5CFA6  // Greyish orange border for long notes
#define GAME_LIGHT_HIT_LINE      0xFFD700  // Gold/yellow

// --- Dark Mode ---
#define GAME_DARK_BG_COLOR       0x1A1A1A  // Matches menu dark bg
#define GAME_DARK_KEY_COLOR      0xE0E0E0  // Off-white for white keys (piano-like)
#define GAME_DARK_KEY_BORDER     0x606060  // Medium gray border
#define GAME_DARK_BLACK_KEY      0x181A20  // True black/dark blue for black keys
#define GAME_DARK_NOTE_COLOR     0x3A86FF  // Vivid blue notes
#define GAME_DARK_NOTE_BORDER    0xF4F6FB  // Light border for contrast
#define GAME_DARK_LONG_NOTE_COLOR 0xFFA500  // Orange for long notes
#define GAME_DARK_LONG_NOTE_BORDER 0xE5CFA6  // Greyish orange border for long notes
#define GAME_DARK_LONG_NOTE_HELD 0xE5CFA6  // Greyish orange border for long notes
#define GAME_DARK_HIT_LINE       0xA259FF  // Purple

#define NUM_KEYS 7
#define KEY_WIDTH (mode_info.XResolution / NUM_KEYS) // Largura dinâmica das teclas
#define KEY_HEIGHT (mode_info.YResolution / 10) // Altura fixa das teclas
#define NOTE_WIDTH KEY_WIDTH
#define NOTE_HEIGHT (mode_info.YResolution / 10) // Altura fixa das notas
#define HIT_MARGIN 10
#define MAX_NOTES 50

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t height;
    bool active;
    int key_index;
    bool is_long;
    bool being_held; // Para notas longas, se estão sendo mantidas
    bool completed;
    int tilescore;
} Note;

typedef struct {
    Note notes[MAX_NOTES];
    int front;
    int rear;
    int size;
} NoteQueue;

extern const uint8_t NOTE_SCANCODES[NUM_KEYS]; 
extern bool key_pressed[NUM_KEYS]; // Array para rastrear teclas pressionadas

uint16_t get_long_tile_height();
void init_note(Note* note, int key, bool is_long, uint16_t long_height);
void update_note(Note* note);
bool check_note_hit(Note* note, uint8_t scancode);
void draw_piano_keys();
void clear_buffer();
void draw_rectangle_to_buffer(uint8_t* buffer, uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);
void draw_pixel_to_buffer(uint8_t* buffer, uint16_t x, uint16_t y, uint32_t color);
void draw_note_to_buffer(Note* note);
void vg_copy_buffer_to_video();
void init_note_queue(NoteQueue* q);
int enqueue_note(NoteQueue* q, int key, bool is_long, uint16_t long_height);
int update_notes(NoteQueue* q);
int draw_notes(NoteQueue* q);
int check_note_hit_queue(NoteQueue* q, uint8_t scancode);
void draw_hit_line();
bool check_note_miss(Note* note);
bool is_note_scancode(uint8_t scancode);
void reset_key_pressed();
int scancode_to_key_index(uint8_t scancode);
int get_long_note_shrink_speed();
void add_score(int base_score);
bool column_has_spawning_note(NoteQueue* q, int new_note_y, int new_note_height);

#endif /* TILE_H */
