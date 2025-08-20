/* [1] hud.c - HUD and message display for mca64Player. All comments in English, suitable for C beginners. */
#include "hud.h"
#include "utils.h"   /* [2] tiny_strlen, int_to_dec, append_uint_zero_pad */
#include <stdint.h>
#include <libdragon.h>

#define HUD_MESSAGE_BUF 128                /* [3] Size of the internal message buffer */
#define HUD_MESSAGE_TIMER_DEFAULT 120      /* [4] Default timer value for message display */

static char hud_message[HUD_MESSAGE_BUF];  /* [5] Message buffer for HUD */
static int hud_message_timer = 0;          /* [6] Timer for message display */

/* [7] Set the message to be displayed on the HUD. Resets the timer. */
void show_message(const char *text) {
    if (!text) return;
    size_t n = (HUD_MESSAGE_BUF > 0) ? (HUD_MESSAGE_BUF - 1) : 0;
    if (n > 0) {
        size_t i;
        for (i = 0; i < n && text[i] != '\0'; i++) {
            hud_message[i] = text[i];
        }
        hud_message[i] = '\0';
    } else if (HUD_MESSAGE_BUF > 0) {
        hud_message[0] = '\0';
    }
    hud_message_timer = HUD_MESSAGE_TIMER_DEFAULT;
}

/* [8] Draw the message box on the screen and decrease the timer. Call once per frame. */
void hud_draw_message(display_context_t disp, uint32_t frame_color, uint32_t bg_color) {
    if (!disp) return;
    if (hud_message_timer <= 0) return;
    if (hud_message[0] == '\0') {
        hud_message_timer = 0;
        return;
    }
    int cur_w = (int)display_get_width();
    int cur_h = (int)display_get_height();
    const int w = 260;
    const int h = 40;
    const int x = (cur_w - w) / 2;
    const int y = (cur_h - h) / 2;
    graphics_draw_box(disp, x, y, w, h, bg_color);
    graphics_draw_line(disp, x - 2, y - 2, x + w + 1, y - 2, frame_color);
    graphics_draw_line(disp, x - 2, y + h + 1, x + w + 1, y + h + 1, frame_color);
    graphics_draw_line(disp, x - 2, y - 2, x - 2, y + h + 1, frame_color);
    graphics_draw_line(disp, x + w + 1, y - 2, x + w + 1, y + h + 1, frame_color);
    uint32_t text_color = graphics_make_color(0, 0, 0, 255);
    graphics_set_color(text_color, 0);
    int txt_w = tiny_strlen(hud_message) * 8;
    int txt_x = x + (w - txt_w) / 2;
    int txt_y = y + (h / 2) - 6;
    graphics_draw_text(disp, txt_x, txt_y, hud_message);
    hud_message_timer--;
}

/* [9] Update the buffer with the last button pressed (as a string). */
void update_last_button_pressed(char *buf, size_t size, joypad_inputs_t inputs) {
    if (!buf || size == 0) return;
    if (inputs.btn.a) { strcpy_s(buf, (int)size, "A"); return; }
    if (inputs.btn.b) { strcpy_s(buf, (int)size, "B"); return; }
    if (inputs.btn.start) { strcpy_s(buf, (int)size, "START"); return; }
    if (inputs.btn.l) { strcpy_s(buf, (int)size, "L"); return; }
    if (inputs.btn.r) { strcpy_s(buf, (int)size, "R"); return; }
    if (inputs.btn.z) { strcpy_s(buf, (int)size, "Z"); return; }
    if (inputs.btn.c_up) { strcpy_s(buf, (int)size, "C-UP"); return; }
    if (inputs.btn.c_down) { strcpy_s(buf, (int)size, "C-DOWN"); return; }
    if (inputs.btn.c_left) { strcpy_s(buf, (int)size, "C-LEFT"); return; }
    if (inputs.btn.c_right) { strcpy_s(buf, (int)size, "C-RIGHT"); return; }
    if (inputs.btn.d_up) { strcpy_s(buf, (int)size, "D-UP"); return; }
    if (inputs.btn.d_down) { strcpy_s(buf, (int)size, "D-DOWN"); return; }
    if (inputs.btn.d_left) { strcpy_s(buf, (int)size, "D-LEFT"); return; }
    if (inputs.btn.d_right) { strcpy_s(buf, (int)size, "D-RIGHT"); return; }
    if (inputs.stick_x > ANALOG_DEADZONE) { strcpy_s(buf, (int)size, "ANALOG-RIGHT"); return; }
    if (inputs.stick_x < -ANALOG_DEADZONE) { strcpy_s(buf, (int)size, "ANALOG-LEFT"); return; }
    if (inputs.stick_y > ANALOG_DEADZONE) { strcpy_s(buf, (int)size, "ANALOG-UP"); return; }
    if (inputs.stick_y < -ANALOG_DEADZONE) { strcpy_s(buf, (int)size, "ANALOG-DOWN"); return; }
    strcpy_s(buf, (int)size, "None");
}

/* [10] Format analog stick X/Y position as a string. */
void format_analog(char *dst, int x, int y) {
    char *p = dst;
    *p++ = 'X'; *p++ = '=';
    p += int_to_dec(p, x);
    *p++ = ' '; *p++ = 'Y'; *p++ = '=';
    p += int_to_dec(p, y);
    *p = '\0';
}

/* [11] Format time as MM:SS / MM:SS string. */
void format_time_line(char *dst, unsigned int play_min, unsigned int play_sec,
                      unsigned int total_min, unsigned int total_sec) {
    char *p = dst;
    const char *label = "Time: ";
    size_t i = 0;
    while (label[i] != '\0') { *p++ = label[i++]; }
    p += append_uint_zero_pad(p, play_min, 2);
    *p++ = ':';
    p += append_uint_zero_pad(p, play_sec, 2);
    *p++ = ' ';
    *p++ = '/';
    *p++ = ' ';
    p += append_uint_zero_pad(p, total_min, 2);
    *p++ = ':';
    p += append_uint_zero_pad(p, total_sec, 2);
    *p = '\0';
}
