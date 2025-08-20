#pragma once
#include <libdragon.h>
#include <stddef.h>
#define ANALOG_DEADZONE 8
/* Pokazuje komunikat (kopiuje do wewnętrznego bufora i ustawia timer) */
void show_message(const char *text);

/* Rysuje okno komunikatu i zmniejsza timer - wywołuj raz na klatkę */
void hud_draw_message(display_context_t disp, uint32_t frame_color, uint32_t bg_color);

/* Pomocnicze formatery / aktualizator ostatniego przycisku / analogów */
void update_last_button_pressed(char *buf, size_t size, joypad_inputs_t inputs);
void format_analog(char *dst, int x, int y);
void format_time_line(char *dst, unsigned int play_min, unsigned int play_sec,
                      unsigned int total_min, unsigned int total_sec);
