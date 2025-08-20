#include "hud.h"
#include "utils.h"   // 1: tiny_strlen, int_to_dec, append_uint_zero_pad
#include <stdint.h>
#include <libdragon.h>

/* 2: Rozmiar wewnętrznego bufora wiadomości i timer */
#define HUD_MESSAGE_BUF 128
#define HUD_MESSAGE_TIMER_DEFAULT 120

static char hud_message[HUD_MESSAGE_BUF]; // 3: Bufor wiadomości
static int hud_message_timer = 0;         // 4: Timer wyświetlania

/* 5: PUBLIC: ustawienie wiadomości do wyświetlenia */
void show_message(const char *text) {
    if (!text) return;                     // 5.1: brak tekstu → nic nie robimy

    size_t n = (HUD_MESSAGE_BUF > 0) ? (HUD_MESSAGE_BUF - 1) : 0; // 5.2
    if (n > 0) {
        size_t i;
        for (i = 0; i < n && text[i] != '\0'; i++) { // 5.3: kopiowanie własną pętlą
            hud_message[i] = text[i];
        }
        hud_message[i] = '\0';            // 5.4: zakończenie zerem
    } else if (HUD_MESSAGE_BUF > 0) {
        hud_message[0] = '\0';            // 5.5: zabezpieczenie
    }

    hud_message_timer = HUD_MESSAGE_TIMER_DEFAULT; // 5.6: ustaw timer
}

/* 6: PUBLIC: rysowanie wiadomości na ekranie i zmniejszanie timera */
void hud_draw_message(display_context_t disp, uint32_t frame_color, uint32_t bg_color) {
    if (!disp) return;                     // 6.1: brak kontekstu → nic nie robimy
    if (hud_message_timer <= 0) return;   // 6.2: timer = 0 → nic nie rysujemy
    if (hud_message[0] == '\0') {         // 6.3: pusty tekst → zerujemy timer
        hud_message_timer = 0;
        return;
    }

    int cur_w = (int)display_get_width();  // 6.4: szerokość ekranu
    int cur_h = (int)display_get_height(); // 6.5: wysokość ekranu
    const int w = 260;                      // 6.6: szerokość boxa
    const int h = 40;                       // 6.7: wysokość boxa
    const int x = (cur_w - w) / 2;         // 6.8: x boxa
    const int y = (cur_h - h) / 2;         // 6.9: y boxa

    graphics_draw_box(disp, x, y, w, h, bg_color); // 6.10: tło
    graphics_draw_line(disp, x - 2, y - 2, x + w + 1, y - 2, frame_color); // 6.11: ramka góra
    graphics_draw_line(disp, x - 2, y + h + 1, x + w + 1, y + h + 1, frame_color); // 6.12: ramka dół
    graphics_draw_line(disp, x - 2, y - 2, x - 2, y + h + 1, frame_color); // 6.13: ramka lewo
    graphics_draw_line(disp, x + w + 1, y - 2, x + w + 1, y + h + 1, frame_color); // 6.14: ramka prawo

    uint32_t text_color = graphics_make_color(0, 0, 0, 255); // 6.15: kolor tekstu
    graphics_set_color(text_color, 0);

    int txt_w = tiny_strlen(hud_message) * 8; // 6.16: szerokość tekstu
    int txt_x = x + (w - txt_w) / 2;          // 6.17: wyśrodkowanie X
    int txt_y = y + (h / 2) - 6;              // 6.18: wyśrodkowanie Y
    graphics_draw_text(disp, txt_x, txt_y, hud_message); // 6.19: rysowanie tekstu

    hud_message_timer--;                       // 6.20: dekrementacja timera
}

/* 7: PUBLIC: zapis ostatniego wciśniętego przycisku */
void update_last_button_pressed(char *buf, size_t size, joypad_inputs_t inputs) {
    if (!buf || size == 0) return;           // 7.1: brak bufora
    if (inputs.btn.a) { strcpy_s(buf, (int)size, "A"); return; }
    if (inputs.btn.b) { strcpy_s(buf, (int)size, "B"); return; }
    if (inputs.btn.start) { strcpy_s(buf, (int)size, "START"); return; }
    if (inputs.btn.l) { strcpy_s(buf, (int)size, "L"); return; }
    if (inputs.btn.r) { strcpy_s(buf, (int)size, "R"); return; }
    if (inputs.btn.z) { strcpy_s(buf, (int)size, "Z"); return; }
    if (inputs.btn.c_up) { strcpy_s(buf, (int)size, "C-GÓRA"); return; }
    if (inputs.btn.c_down) { strcpy_s(buf, (int)size, "C-DÓŁ"); return; }
    if (inputs.btn.c_left) { strcpy_s(buf, (int)size, "C-LEWO"); return; }
    if (inputs.btn.c_right) { strcpy_s(buf, (int)size, "C-PRAWO"); return; }
    if (inputs.btn.d_up) { strcpy_s(buf, (int)size, "GÓRA"); return; }
    if (inputs.btn.d_down) { strcpy_s(buf, (int)size, "DÓŁ"); return; }
    if (inputs.btn.d_left) { strcpy_s(buf, (int)size, "LEWO"); return; }
    if (inputs.btn.d_right) { strcpy_s(buf, (int)size, "PRAWO"); return; }
    if (inputs.stick_x > ANALOG_DEADZONE) { strcpy_s(buf, (int)size, "ANALOG-PRAWO"); return; }
    if (inputs.stick_x < -ANALOG_DEADZONE) { strcpy_s(buf, (int)size, "ANALOG-LEWO"); return; }
    if (inputs.stick_y > ANALOG_DEADZONE) { strcpy_s(buf, (int)size, "ANALOG-GÓRA"); return; }
    if (inputs.stick_y < -ANALOG_DEADZONE) { strcpy_s(buf, (int)size, "ANALOG-DÓŁ"); return; }
    strcpy_s(buf, (int)size, "Brak");
}

/* 8: PUBLIC: format analogów X/Y */
void format_analog(char *dst, int x, int y) {
    char *p = dst;
    *p++ = 'X'; *p++ = '=';
    p += int_to_dec(p, x);
    *p++ = ' '; *p++ = 'Y'; *p++ = '=';
    p += int_to_dec(p, y);
    *p = '\0';
}

/* 9: PUBLIC: format czasu MM:SS / MM:SS */
void format_time_line(char *dst, unsigned int play_min, unsigned int play_sec,
                      unsigned int total_min, unsigned int total_sec) {
    char *p = dst;
    const char *label = "Czas: ";
    size_t i = 0;
    while (label[i] != '\0') { *p++ = label[i++]; } // 9.1: kopiowanie własną pętlą

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
