#include <lcom/lcf.h>
#include <stdbool.h>
#include <stdio.h>
#include "game.h"
#include "../menu/options.h" // for select_difficulty
#include "../highscore/highscore.h"

#define HIGHSCORE_FILE "game/highscore.txt"

int game_timer_counter = 0;
bool game_over = false;
vbe_mode_info_t mode_info;
NoteQueue note_queue;
extern int spawn_interval_preset; // intervalo de spawn de notas
extern int select_difficulty; // dificuldade selecionada do jogo
extern int points_multiplier; // multiplicador de pontos baseado na dificuldade
extern int select_tile_type; // tipo de tile selecionado (0 for both, 1 for short)
int spawn_interval; // intervalo de spawn de notas
int score = 0; // pontuação do jogo

extern int high_score; 
extern int high_diff; 


int (game_loop)(void) {

    spawn_interval = spawn_interval_preset; // intervalo de spawn de notas

    reset_key_pressed(); // Reseta o estado das teclas pressionadas
    init_note_queue(&note_queue); // inicializar fila de notas

    // Subscrever interrupções do teclado e do timer
    uint8_t timer_bit = 0;
    uint8_t keyboard_bit;

    if (timer_subscribe_int(&timer_bit) != 0) {
        return 1;
    }
    
    if (keyboard_subscribe_interrupts(&keyboard_bit) != 0) {
        timer_unsubscribe_int();
        return 1;
    }

    int r;
    message msg;
    int ipc_status;

    clear_buffer(); // limpa o buffer com a cor correta (branco no light mode)
    draw_piano_keys(); // desenha as teclas no buffer
    draw_hit_line(); // desenha a linha de acerto
    draw_score(score); // desenhar score na tela
    draw_timer(game_timer_counter/GAME_FREQUENCY); // desenhar temporizador na tela
    vg_copy_buffer_to_video(); // copia o buffer para o vídeo


    //loop principal do jogo
    while (!game_over) {
        if ((r = driver_receive(ANY, &msg, &ipc_status)) != 0) {
            printf("driver_receive failed with: %d", r);
            continue;
        }
        if (is_ipc_notify(ipc_status)) {
            switch (_ENDPOINT_P(msg.m_source)) {
                case HARDWARE:
                    if (msg.m_notify.interrupts & BIT(timer_bit)) {
                        game_timer_counter++;

                        // Decrease spawn interval every 15 seconds, but not below a minimum (e.g., 40)
                        if (game_timer_counter % (20 * GAME_FREQUENCY) == 0 && spawn_interval > 40) {
                            spawn_interval -= 30;
                            if (spawn_interval < 40) spawn_interval = 40;
                        }

                        if (game_timer_counter % 5 == 0) { // atualizar todas as notas periodicamente
                            update_notes(&note_queue);
                            for (int i = 0; i < note_queue.size; i++) {
                                int idx = (note_queue.front + i) % MAX_NOTES;
                                if (check_note_miss(&note_queue.notes[idx])) {
                                    game_over = true;
                                }
                            }

                        }

                        clear_buffer();
                        draw_notes(&note_queue);
                        draw_hit_line();
                        draw_piano_keys();
                        draw_score(score);
                        draw_timer(game_timer_counter / GAME_FREQUENCY);
                        vg_copy_buffer_to_video();

                        if (game_timer_counter % spawn_interval == 0) {
                            int random_key = rand() % 7;
                            bool is_long = false;
                            if (select_tile_type == 0) {
                                if ((rand() % 100) < 20) { // 20% chance of being a long note
                                    is_long = true;
                                }
                            }
                            int new_note_y = 0;
                            uint16_t new_note_height = is_long ? get_long_tile_height() : NOTE_HEIGHT;
                            if (!column_has_spawning_note(&note_queue, new_note_y, new_note_height)) {
                                enqueue_note(&note_queue, random_key, is_long, new_note_height);
                            }
                        }
                    }
                    if (msg.m_notify.interrupts & BIT(keyboard_bit)) {
                        kbc_ih();
                        printf("Key pressed: 0x%X\n", scancode);
                        if ((scancode & 0x80) == 0) { // Tecla pressionada
                            for (int i = 0; i < NUM_KEYS; i++) {
                                if (scancode == NOTE_SCANCODES[i]) key_pressed[i] = true;
                            }
                            if (scancode == 0x01) { // ESC
                                game_over = true;
                            } else if (is_note_scancode(scancode)) {
                                int hit = check_note_hit_queue(&note_queue, scancode);
                                if (hit) {
                                    Note *note = &note_queue.notes[note_queue.front];
                                    if (!note->is_long) {
                                        add_score(10);
                                    }
                                } else {
                                    // Check if any long note is being held for this key
                                    bool holding_long_note = false;
                                    for (int i = 0; i < note_queue.size; i++) {
                                        int idx = (note_queue.front + i) % MAX_NOTES;
                                        Note *note = &note_queue.notes[idx];
                                        if (note->active && note->is_long && note->key_index == scancode_to_key_index(scancode) && note->being_held) {
                                            holding_long_note = true;
                                            break;
                                        }
                                    }
                                    if (!holding_long_note) {
                                        game_over = true;
                                        break;
                                    }
                                    // else: ignore, since we're holding a long note
                                }
                            }
                            // else: ignore all other keys
                        } else { // Key released
                            uint8_t released = scancode & 0x7F;
                            for (int i = 0; i < NUM_KEYS; i++) {
                                if (released == NOTE_SCANCODES[i]) key_pressed[i] = false;
                            }
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
    timer_unsubscribe_int();

    // Finalizar jogo, calcular score e salvar highscore
    highscore_rec(score);

    spawn_interval = 150;
    game_over = false;
    game_timer_counter = 0;
    score = 0;

    return 0; 
}

