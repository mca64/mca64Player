#pragma once
#include <libdragon.h>
#include <stddef.h>
#define ANALOG_DEADZONE 8

/* [1] Show a message (copies to internal buffer and sets timer) */
void show_message(const char *text);

/* [2] Draw the message box and decrease timer - call once per frame */
void hud_draw_message(display_context_t disp, uint32_t frame_color, uint32_t bg_color);

/* [3] Helper formatters / last button updater / analog formatters */
void update_last_button_pressed(char *buf, size_t size, joypad_inputs_t inputs);
void format_analog(char *dst, int x, int y);
void format_time_line(char *dst, unsigned int play_min, unsigned int play_sec,
                      unsigned int total_min, unsigned int total_sec);
