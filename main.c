#include <libdragon.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdbool.h>
#include <system.h>
#include <string.h>
#include <wav64.h>

#define ANALOG_DEADZONE 8

/* [1] Globalne zmienne i cache kolorów / stany wydajności ------------------- */
static uint32_t last_frame_ticks = 0; /* [1] Licznik ticków poprzedniej klatki (FPS). */
static uint32_t fps = 0;              /* [1] Ostatnio policzone FPS. */
static size_t mem_used = 0;           /* [1] Przybliżone użycie pamięci śledzone przez tracker. */

float volume = 1.0f;                  /* [1] Głośność (0.0 .. 1.0) */
char message[128] = "";               /* [1] Obecny komunikat do wyświetlenia */
int message_timer = 0;                /* [1] Licznik ramek pozostałych do wyświetlenia komunikatu */

/* [2] Pokazuje komunikat - ustawia timer w klatkach (60 FPS => 60 => 1s) ---- */
void show_message(const char *text) {
    if (!text) return;
    strncpy(message, text, sizeof(message));
    message[sizeof(message)-1] = '\0';
    message_timer = 120; /* ~2 sekundy przy 60 FPS; przy wait_ms=33 -> ~4s */
}

/* [3] Prosty tracker alokacji (przydatny do debugowania pamięci) ------------ */
typedef struct MemTrack {
    void *ptr;
    size_t size;
    struct MemTrack *next;
} MemTrack;

static MemTrack *mem_list = NULL;

static void memtrack_add(void *ptr, size_t size) {
    if (!ptr || size == 0) return;
    MemTrack *node = (MemTrack *)malloc(sizeof(MemTrack));
    if (!node) return;
    node->ptr  = ptr;
    node->size = size;
    node->next = mem_list;
    mem_list = node;
}

static size_t memtrack_remove(void *ptr) {
    if (!ptr) return 0;
    MemTrack **pp = &mem_list;
    while (*pp) {
        if ((*pp)->ptr == ptr) {
            MemTrack *to_free = *pp;
            size_t size = to_free->size;
            *pp = to_free->next;
            free(to_free);
            return size;
        }
        pp = &((*pp)->next);
    }
    return 0;
}

void *track_malloc(size_t size) {
    if (size == 0) return NULL;
    void *p = malloc(size);
    if (!p) return NULL;
    mem_used += size;
    memtrack_add(p, size);
    return p;
}

void track_free(void *ptr) {
    if (!ptr) return;
    size_t size = memtrack_remove(ptr);
    if (size > 0 && mem_used >= size) mem_used -= size;
    free(ptr);
}

/* [4] Funkcje narzędziowe (strlen, strcpy_s, formaty liczb) ----------------- */
static inline int tiny_strlen(const char *s) {
    if (!s) return 0;
    int l = 0;
    while (s[l]) l++;
    return l;
}

static inline void strcpy_s(char *dst, int n, const char *src) {
    if (n <= 0 || !dst) return;
    if (!src) { dst[0] = '\0'; return; }
    int i = 0;
    while (i < n - 1 && src[i]) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static int int_to_dec(char *out, int val) {
    char tmp[12];
    int pos = 0;
    unsigned int v;
    if (val < 0) {
        out[0] = '-';
        out++;
        v = (unsigned int)(-(long)val);
    } else v = (unsigned int)val;
    if (v == 0) { out[0] = '0'; out[1] = '\0'; return (val < 0) ? 2 : 1; }
    while (v) { tmp[pos++] = '0' + (v % 10); v /= 10; }
    for (int i = 0; i < pos; i++) out[i] = tmp[pos - 1 - i];
    out[pos] = '\0';
    return (val < 0) ? (pos + 1) : pos;
}

static int append_uint_zero_pad(char *dst, unsigned int val, int width) {
    char tmp[16];
    int pos = 0;
    if (val == 0) {
        for (int i = 0; i < width; i++) dst[i] = '0';
        dst[width] = '\0';
        return width;
    }
    while (val) { tmp[pos++] = '0' + (val % 10); val /= 10; }
    int pad = width - pos;
    int idx = 0;
    while (pad-- > 0) dst[idx++] = '0';
    for (int i = 0; i < pos; i++) dst[idx++] = tmp[pos - 1 - i];
    dst[idx] = '\0';
    return idx;
}

static inline void u8_to_hex_sp(char *out, uint8_t v) {
    static const char hex[] = "0123456789ABCDEF";
    out[0] = hex[(v >> 4) & 0xF];
    out[1] = hex[v & 0xF];
    out[2] = ' ';
}

/* [5] Wejście / obsługa przycisków oraz formatowanie analogów ------------- */
void update_last_button_pressed_optimized(char *buf, size_t size, joypad_inputs_t inputs) {
    if (inputs.btn.a) strcpy_s(buf, (int)size, "A");
    else if (inputs.btn.b) strcpy_s(buf, (int)size, "B");
    else if (inputs.btn.z) strcpy_s(buf, (int)size, "Z");
    else if (inputs.btn.start) strcpy_s(buf, (int)size, "START");
    else if (inputs.btn.l) strcpy_s(buf, (int)size, "L");
    else if (inputs.btn.r) strcpy_s(buf, (int)size, "R");
    else if (inputs.btn.d_up) strcpy_s(buf, (int)size, "UP");
    else if (inputs.btn.d_down) strcpy_s(buf, (int)size, "DOWN");
    else if (inputs.btn.d_left) strcpy_s(buf, (int)size, "LEFT");
    else if (inputs.btn.d_right) strcpy_s(buf, (int)size, "RIGHT");
    else if (inputs.btn.c_up) strcpy_s(buf, (int)size, "C-UP");
    else if (inputs.btn.c_down) strcpy_s(buf, (int)size, "C-DOWN");
    else if (inputs.btn.c_left) strcpy_s(buf, (int)size, "C-LEFT");
    else if (inputs.btn.c_right) strcpy_s(buf, (int)size, "C-RIGHT");
    else if (inputs.stick_x > ANALOG_DEADZONE) strcpy_s(buf, (int)size, "ANALOG-RIGHT");
    else if (inputs.stick_x < -ANALOG_DEADZONE) strcpy_s(buf, (int)size, "ANALOG-LEFT");
    else if (inputs.stick_y > ANALOG_DEADZONE) strcpy_s(buf, (int)size, "ANALOG-UP");
    else if (inputs.stick_y < -ANALOG_DEADZONE) strcpy_s(buf, (int)size, "ANALOG-DOWN");
    else strcpy_s(buf, (int)size, "Brak");
}

static void format_analog(char *dst, int x, int y) {
    char *p = dst;
    *p++ = 'X'; *p++ = '=';
    p += int_to_dec(p, x);
    *p++ = ' '; *p++ = 'Y'; *p++ = '=';
    p += int_to_dec(p, y);
    *p = '\0';
}

/* [6] Formatowanie czasu i pasek postępu -------------------------------- */
static void format_time_line(char *dst, unsigned int play_min, unsigned int play_sec,
                             unsigned int total_min, unsigned int total_sec) {
    char *p = dst;
    const char *label = "Czas: ";
    strcpy_s(p, 80, label);
    p += tiny_strlen(label);
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

/* [7] Pomocnicze funkcje rysowania VU (unifikacja mono/stereo) ------------ */
static void draw_vu_meter(display_context_t disp, int base_x, int base_y, int width, int height,
                          int value, int max_val, uint32_t box_color, uint32_t fill_color, const char *label) {
    if (max_val <= 0) max_val = 1;
    graphics_draw_box(disp, base_x - 1, base_y - height - 1, width + 2, height + 2, box_color);
    int h = (value * height) / max_val;
    if (h > height) h = height;
    graphics_draw_box(disp, base_x, base_y - h, width, h, fill_color);
    int tx = base_x + (width / 2) - ((int)tiny_strlen(label) * 4);
    graphics_draw_text(disp, tx, base_y + 4, label);
}

/* =========================================================================
 *  [8] Funkcja: draw_message_box
 *  Opis:
 *      Rysuje prostokątny komunikat na środku ekranu z ramką i tekstem.
 *      Wypełnienie jest jednolite w kolorze `bg_color`, a ramka w kolorze `frame_color`.
 * ========================================================================= */
static void draw_message_box(display_context_t disp, const char *msg, uint32_t frame_color, uint32_t bg_color)
{
    /* [8.1] Sprawdzenie, czy jest co rysować */
    if (!msg || msg[0] == '\0')
        return;

    /* [8.2] Wymiary boxa (260x40) i pozycja na środku ekranu 320x240 */
    const int w = 260;
    const int h = 40;
    const int x = (320 - w) / 2;
    const int y = (240 - h) / 2;

    /* [8.3] Rysowanie tła – pełny prostokąt w kolorze bg_color */
    graphics_draw_box(disp, x, y, w, h, bg_color);

    /* [8.4] Rysowanie ramki (pełny kolor frame_color, obrys prostokąta) */
    graphics_draw_line(disp, x - 2, y - 2, x + w + 1, y - 2, frame_color); // góra
    graphics_draw_line(disp, x - 2, y + h + 1, x + w + 1, y + h + 1, frame_color); // dół
    graphics_draw_line(disp, x - 2, y - 2, x - 2, y + h + 1, frame_color); // lewa
    graphics_draw_line(disp, x + w + 1, y - 2, x + w + 1, y + h + 1, frame_color); // prawa

    /* [8.5] Rysowanie tekstu na środku boxa */
    uint32_t text_color = graphics_make_color(0, 0, 0, 255);  /* czarny kolor tekstu */
    graphics_set_color(text_color, 0);

    int txt_w = tiny_strlen(msg) * 8;                         /* szerokość napisu w pikselach */
    int txt_x = x + (w - txt_w) / 2;                          /* pozycja X wyśrodkowana */
    int txt_y = y + (h / 2) - 6;                              /* pozycja Y lekko przesunięta w górę */

    graphics_draw_text(disp, txt_x, txt_y, msg);

    /* [8.6] Po wyjściu z funkcji, kolor rysowania pozostaje ustawiony na kolor tekstu.
       Caller powinien ustawić go z powrotem, jeśli jest to wymagane. */
}



/* [9] Główna funkcja programu (inicjalizacja i pętla) --------------------- */
int main(void) {
    const int SOUND_CH = 0; /* [9] Stały kanał dźwiękowy (po prostu 0) */

    /* [9.1] Inicjalizacja wyświetlacza i kontrolera */
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
    joypad_init();

    /* [9.2] Inicjalizacja systemu plików (DFS) - jeśli nie działa, wypisz błąd i wyjdź */
    if (dfs_init(DFS_DEFAULT_LOCATION) != DFS_ESUCCESS) {
        surface_t *disp = display_get();
        uint32_t black = graphics_make_color(0, 0, 0, 255);
        uint32_t red   = graphics_make_color(255, 0, 0, 255);
        graphics_fill_screen(disp, black);
        graphics_set_color(red, 0);
        graphics_draw_text(disp, 10, 10, "Blad: nie mozna zainicjowac DFS");
        display_show(disp);
        return 1;
    }

    /* [9.3] Wczytanie nagłówka WAV64 (krótki hexdump) by wykryć manual_compression_level */
    uint8_t manual_compression_level = 0;
    char header_hex_string[64]; header_hex_string[0] = '\0';
    int dfs_bytes_read = -1;
    {
        char header_buf[16];
        int file_size = 0;
        FILE *f = asset_fopen("rom:/sound.wav64", &file_size);
        if (f) {
            dfs_bytes_read = (int)fread(header_buf, 1, (size_t)sizeof(header_buf), f);
            if (dfs_bytes_read >= 6) manual_compression_level = (uint8_t)header_buf[5];
            int ptr = 0;
            for (int i = 0; i < dfs_bytes_read && (ptr + 3) < (int)sizeof(header_hex_string); i++) {
                u8_to_hex_sp(&header_hex_string[ptr], (uint8_t)header_buf[i]);
                ptr += 3;
            }
            if (ptr < (int)sizeof(header_hex_string)) header_hex_string[ptr] = '\0';
            fclose(f);
        } else {
            strcpy_s(header_hex_string, (int)sizeof(header_hex_string), "Read error");
        }
    }

    /* [9.4] Timer (FPS) init */
    timer_init();
    last_frame_ticks = timer_ticks();

    /* [9.5] Ustawienie i inicjalizacja kompresji (jeśli potrzeba) */
    int compression_level = (int)manual_compression_level;
    if (compression_level != 0 && compression_level != 1 && compression_level != 3) compression_level = 0;
    wav64_init_compression(compression_level);

    /* [9.6] Otwieranie pliku i inicjalizacja audio/miksera */
    const char *filename = "rom:/sound.wav64";
    wav64_t sound;
    memset(&sound, 0, sizeof(sound));
    wav64_open(&sound, filename);
    audio_init((int)sound.wave.frequency, 4);
    mixer_init(32);
    wav64_set_loop(&sound, true); /* biblioteka też może mieć loop, ale program steruje restartem */

    /* [9.7] Zarządzanie pozycji odtwarzania (tylko jedna zmienna potrzebna) */
    uint32_t current_sample_pos = 0; /* [9.7] Pozycja w próbkach używana przy pauzie/seek */
    bool is_playing = false;
    int sound_channel = SOUND_CH;

    /* [9.8] Start odtwarzania od 0 */
    current_sample_pos = 0;
    wav64_play(&sound, sound_channel);
    mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
    is_playing = true;
    mixer_ch_set_vol(sound_channel, volume, volume);

    /* [9.9] Parametry audio (pobrane z wav64_t) */
    uint32_t sample_rate = (uint32_t)(sound.wave.frequency + 0.5f);
    uint8_t channels = sound.wave.channels;
    uint32_t total_samples = (uint32_t)sound.wave.len;
    uint8_t bits_per_sample = sound.wave.bits;
    uint32_t total_seconds = (sample_rate && total_samples) ? (uint32_t)(total_samples / sample_rate) : 0;
    uint32_t total_minutes = total_seconds / 60;
    uint32_t total_secs_rem = total_seconds % 60;
    int bitrate_bps = wav64_get_bitrate(&sound);
    uint32_t bitrate_kbps = (bitrate_bps > 0) ? (uint32_t)(bitrate_bps / 1000) :
                         (uint32_t)((sample_rate * (uint32_t)channels * (uint32_t)bits_per_sample) / 1000U);

    /* [9.10] Bufory i kolory (cache) */
    char last_button_pressed[32]; strcpy_s(last_button_pressed, (int)sizeof(last_button_pressed), "Brak");
    char analog_pos[32]; strcpy_s(analog_pos, (int)sizeof(analog_pos), "X=0 Y=0");

    uint32_t bg_color = graphics_make_color(0, 0, 64, 255);
    uint32_t white    = graphics_make_color(255, 255, 255, 255);
    uint32_t green    = graphics_make_color(0, 255, 0, 255);
    uint32_t box_frame_color = graphics_make_color(0, 200, 0, 255); /* ciemniejsza zieleń */
    uint32_t box_bg_color    = graphics_make_color(0, 200, 0, 255); /* używana w szachownicy (bez alfowania) */

    bool loop_enabled = true; /* [9.10] Sterowanie automatycznym restartem */

    /* [9.11] Główna pętla aplikacji --------------------------------------- */
    while (1) {
        int max_amp = 0, max_amp_l = 0, max_amp_r = 0;

        /* [9.11.1] Poll kontrolerów i aktualizacja sygnałów wejścia */
        joypad_poll();
        joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
        update_last_button_pressed_optimized(last_button_pressed, sizeof(last_button_pressed), inputs);

        /* [9.11.2] Formatowanie pozycji analoga (martwa strefa) */
        int ax = inputs.stick_x;
        int ay = inputs.stick_y;
        if ((unsigned)(ax + ANALOG_DEADZONE) < (ANALOG_DEADZONE * 2)) ax = 0;
        if ((unsigned)(ay + ANALOG_DEADZONE) < (ANALOG_DEADZONE * 2)) ay = 0;
        format_analog(analog_pos, ax, ay);

        /* [9.11.3] Odtwarzanie audio i wyliczanie VU (jeśli bufor audio wymaga danych) */
        if (audio_can_write()) {
            short *outbuf = audio_write_begin();
            int buf_len = audio_get_buffer_length();
            mixer_poll(outbuf, buf_len); /* [9.11.3] Mixer wypełnia bufor wyjściowy */
            if (channels == 2) {
                for (int i = 0; i + 1 < buf_len; i += 2) {
                    int l = abs((int)outbuf[i]);
                    if (l > max_amp_l) max_amp_l = l;
                    int r = abs((int)outbuf[i + 1]);
                    if (r > max_amp_r) max_amp_r = r;
                }
            } else {
                for (int i = 0; i < buf_len; i++) {
                    int v = abs((int)outbuf[i]);
                    if (v > max_amp) max_amp = v;
                }
            }
            audio_write_end();
        }

        /* [9.11.4] Zarządzanie końcem odtwarzania i automatyczny restart (loop) */
        if (!mixer_ch_playing(sound_channel)) {
            if (loop_enabled) {
                /* [9.11.4.a] Zrestartuj odtwarzanie od początku; SEEK nigdy nie jest wykonywane
                   automatycznie dla VADPCM/Opus - zawsze start od 0. */
                wav64_play(&sound, sound_channel);
                mixer_ch_set_pos(sound_channel, 0.0f);
                mixer_ch_set_vol(sound_channel, volume, volume);
                is_playing = true;
                current_sample_pos = 0;
                show_message("Restart (loop)");
            } else {
                /* [9.11.4.b] Jeśli pętla wyłączona, ustaw stan zakończenia raz. */
                if (is_playing) {
                    is_playing = false;
                    current_sample_pos = total_samples;
                    show_message("Koniec");
                }
            }
        }

        /* [9.11.5] Obliczanie pozycji do wyświetlenia - tylko gdy kanał aktualnie gra */
        uint32_t current_sample_pos_display = current_sample_pos;
        if (is_playing && mixer_ch_playing(sound_channel)) {
            float pos_f = mixer_ch_get_pos(sound_channel);
            if (pos_f < 0.0f) pos_f = 0.0f;
            uint64_t pos_u = (uint64_t)(pos_f + 0.5f);
            if (pos_u > (uint64_t)total_samples) pos_u = (uint64_t)total_samples;
            current_sample_pos_display = (uint32_t)pos_u;
        }

        /* [9.11.6] Konwersja na minuty/sekundy */
        uint32_t elapsed_sec = (sample_rate) ? (current_sample_pos_display / sample_rate) : 0;
        uint32_t play_min = elapsed_sec / 60U;
        uint32_t play_sec = elapsed_sec % 60U;

        /* [9.11.7] Przygotowanie ekranu */
        surface_t *disp = display_get();
        graphics_fill_screen(disp, bg_color);
        graphics_set_color(white, 0);

        /* [9.11.8] Rysowanie HUD i tekstów */
        char linebuf[128];
        const char *title = "mca64Player";
        int title_x = (320 - tiny_strlen(title) * 8) / 2;
        graphics_draw_text(disp, title_x, 4, title);
        graphics_draw_text(disp, 4, 4, last_button_pressed);
        graphics_draw_text(disp, 4, 16, analog_pos);

        strcpy_s(linebuf, (int)sizeof(linebuf), "Plik: ");
        int off = tiny_strlen(linebuf);
        int remaining = (int)sizeof(linebuf) - off;
        strcpy_s(&linebuf[off], remaining, filename);
        graphics_draw_text(disp, 10, 28, linebuf);

        format_time_line(linebuf, (unsigned)play_min, (unsigned)play_sec,
                         (unsigned)total_minutes, (unsigned)total_secs_rem);
        graphics_draw_text(disp, 10, 46, linebuf);

        /* [9.11.9] Pasek postępu */
        int bar_x = 10, bar_y = 58, bar_w = 300, bar_h = 8;
        graphics_draw_box(disp, bar_x, bar_y, bar_w, bar_h, white);
        if (total_seconds > 0) {
            int pw = (int)(((uint64_t)elapsed_sec * (uint64_t)bar_w) / (uint64_t)total_seconds);
            if (pw > bar_w) pw = bar_w;
            graphics_draw_box(disp, bar_x, bar_y, pw, bar_h, green);
        }

        /* [9.11.10] Parametry audio (tekst) */
        strcpy_s(linebuf, (int)sizeof(linebuf), "Probkowanie: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)sample_rate);
        strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, " Hz");
        graphics_draw_text(disp, 10, 76, linebuf);

        strcpy_s(linebuf, (int)sizeof(linebuf), "Bitrate: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)bitrate_kbps);
        strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, " kbps");
        graphics_draw_text(disp, 10, 94, linebuf);

        if (channels == 1) strcpy_s(linebuf, (int)sizeof(linebuf), "Kanaly: Mono (1)");
        else strcpy_s(linebuf, (int)sizeof(linebuf), "Kanaly: Stereo (2)");
        graphics_draw_text(disp, 10, 112, linebuf);

        strcpy_s(linebuf, (int)sizeof(linebuf), "Bity: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)bits_per_sample);
        strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, " bit");
        graphics_draw_text(disp, 10, 130, linebuf);

        strcpy_s(linebuf, (int)sizeof(linebuf), "Kompresja (z lib): ");
        off = tiny_strlen(linebuf);
        if (compression_level == 0) strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, "PCM");
        else if (compression_level == 1) strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, "VADPCM");
        else if (compression_level == 3) strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, "Opus");
        else strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, "Nieznany");
        graphics_draw_text(disp, 10, 148, linebuf);

        strcpy_s(linebuf, (int)sizeof(linebuf), "Kompresja (manual): ");
        off = tiny_strlen(linebuf);
        if (manual_compression_level == 0) strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, "PCM");
        else if (manual_compression_level == 1) strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, "VADPCM");
        else if (manual_compression_level == 3) strcpy_s(&linebuf[off], (int)sizeof(linebuf) - off, "Opus");
        else { off += int_to_dec(&linebuf[off], (int)manual_compression_level); }
        graphics_draw_text(disp, 10, 166, linebuf);

        strcpy_s(linebuf, (int)sizeof(linebuf), "Probek: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], total_samples);
        graphics_draw_text(disp, 10, 184, linebuf);

        /* [9.11.11] VU-metry */
        int vu_base_x = 280;
        int vu_base_y = 200;
        int vu_width = 8;
        int vu_height = 40;
        if (channels == 2) {
            draw_vu_meter(disp, vu_base_x - 0, vu_base_y, vu_width, vu_height, max_amp_l, 32768, white, green, "L");
            draw_vu_meter(disp, vu_base_x + vu_width + 10, vu_base_y, vu_width, vu_height, max_amp_r, 32768, white, green, "R");
        } else {
            draw_vu_meter(disp, vu_base_x + 8, vu_base_y, vu_width, vu_height, max_amp, 32768, white, green, "Mono");
        }

        /* [9.11.12] Hex dump nagłówka (2 linie) */
        char hex_line1[40] = {0}, hex_line2[40] = {0};
        int hex_len = tiny_strlen(header_hex_string);
        int groups = hex_len / 3;
        int split_groups = groups / 2;
        int split_pos = split_groups * 3;
        int i = 0, idx = 0;
        for (i = 0; i < split_pos && i < (int)sizeof(hex_line1) - 1; i++) hex_line1[i] = header_hex_string[i];
        hex_line1[i] = '\0';
        idx = 0;
        for (i = split_pos; i < hex_len && idx < (int)sizeof(hex_line2) - 1; i++) hex_line2[idx++] = header_hex_string[i];
        hex_line2[idx] = '\0';
        graphics_draw_text(disp, 10, 202, hex_line1);
        graphics_draw_text(disp, 10, 212, hex_line2);

        /* [9.11.13] Inline: update_perf_counters() i draw_perf_hud(disp) */
        {
            uint32_t now_ticks = timer_ticks();
            uint32_t diff_ticks = now_ticks - last_frame_ticks;
            if (last_frame_ticks != 0 && diff_ticks > 0) {
                fps = TICKS_PER_SECOND / diff_ticks;
            }
            last_frame_ticks = now_ticks;
        }
        {
            char perfbuf[64];
            snprintf(perfbuf, sizeof(perfbuf), "FPS: %lu", (unsigned long)fps);
            graphics_draw_text(disp, 10, 230, perfbuf);
            double ram_total = get_memory_size() / (1024.0 * 1024.0);
            struct mallinfo mi = mallinfo();
            double used_heap = mi.uordblks / (1024.0 * 1024.0);
            double ram_free = ram_total - used_heap;
            snprintf(perfbuf, sizeof(perfbuf), "RAM: %.2f MB", ram_total);
            graphics_draw_text(disp, 74, 230, perfbuf);
            snprintf(perfbuf, sizeof(perfbuf), "Free: %.2f MB", ram_free);
            graphics_draw_text(disp, 200, 230, perfbuf);
        }

        /* [9.11.14] OBSŁUGA PRZYCISKÓW (rising edge) */
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);

        /* START/A - pauza / wznowienie (toggle) */
        if (keys.start || keys.a) {
            if (is_playing && mixer_ch_playing(sound_channel)) {
                /* [9.11.14.a] Pauza: zapamiętuje pozycję i zatrzymuje kanał */
                float pos_f = mixer_ch_get_pos(sound_channel);
                if (pos_f < 0.0f) pos_f = 0.0f;
                uint64_t pos_u = (uint64_t)(pos_f + 0.5f);
                if (pos_u > (uint64_t)total_samples) pos_u = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)pos_u;
                mixer_ch_stop(sound_channel);
                is_playing = false;
                show_message("Pauza");
            } else {
                /* [9.11.14.b] Wznowienie: jeśli format nie wspiera seek i jest offset -> start od 0 */
                if (compression_level != 0 && current_sample_pos != 0) {
                    current_sample_pos = 0;
                    show_message("Seek niedostepny - start od poczatku");
                }
                wav64_play(&sound, sound_channel);
                mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                mixer_ch_set_vol(sound_channel, volume, volume);
                is_playing = true;
                show_message("Wznowiono");
            }
        }

        /* B - stop i reset do początku */
        if (keys.b) {
            if (mixer_ch_playing(sound_channel)) mixer_ch_stop(sound_channel);
            current_sample_pos = 0;
            is_playing = false;
            show_message("Stop");
        }

        /* Seek przyciskami - tylko dla PCM (compression_level == 0) */
        if (keys.l || keys.d_left) {
            if (compression_level == 0) {
                int64_t delta_samples = (int64_t)5 * (int64_t)sample_rate;
                int64_t newpos = (int64_t)current_sample_pos_display - delta_samples;
                if (newpos < 0) newpos = 0;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie -5s");
            } else show_message("Seek niedostepny dla tego formatu");
        }
        if (keys.r || keys.d_right) {
            if (compression_level == 0) {
                uint64_t delta_samples = (uint64_t)5 * (uint64_t)sample_rate;
                uint64_t newpos = (uint64_t)current_sample_pos_display + delta_samples;
                if (newpos > (uint64_t)total_samples) newpos = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie +5s");
            } else show_message("Seek niedostepny dla tego formatu");
        }
        if (keys.c_left) {
            if (compression_level == 0) {
                int64_t delta_samples = (int64_t)30 * (int64_t)sample_rate;
                int64_t newpos = (int64_t)current_sample_pos_display - delta_samples;
                if (newpos < 0) newpos = 0;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie -30s");
            } else show_message("Seek niedostepny dla tego formatu");
        }
        if (keys.c_right) {
            if (compression_level == 0) {
                uint64_t delta_samples = (uint64_t)30 * (uint64_t)sample_rate;
                uint64_t newpos = (uint64_t)current_sample_pos_display + delta_samples;
                if (newpos > (uint64_t)total_samples) newpos = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie +30s");
            } else show_message("Seek niedostepny dla tego formatu");
        }

        /* [9.11.15] Głośność (D-Up / D-Down / C-Up / C-Down) */
        if (keys.d_up || keys.c_up) {
            volume += 0.1f;
            if (volume > 1.0f) volume = 1.0f;
            mixer_ch_set_vol(sound_channel, volume, volume);
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "Glosnosc: %.1f", volume);
            show_message(tmp);
        }
        if (keys.d_down || keys.c_down) {
            volume -= 0.1f;
            if (volume < 0.0f) volume = 0.0f;
            mixer_ch_set_vol(sound_channel, volume, volume);
            char tmp[64];
            snprintf(tmp, sizeof(tmp), "Glosnosc: %.1f", volume);
            show_message(tmp);
        }

        /* [9.11.16] Toggle loop on/off (Z) */
        if (keys.z) {
            loop_enabled = !loop_enabled;
            wav64_set_loop(&sound, loop_enabled); /* informuje bibliotekę */
            if (loop_enabled) show_message("Petla: ON"); else show_message("Petla: OFF");
        }

        /* [9.11.17] Rysowanie komunikatu (okno) - zielone, "półprzezroczyste" (szachownica) */
        if (message_timer > 0) {
            draw_message_box(disp, message, box_frame_color, box_bg_color);
            message_timer--;
            /* Przywróć kolor tekstu dla następnych elementów (gdyby były) */
            graphics_set_color(white, 0);
        }

        /* [9.11.18] Wyświetlenie bufora i odczekanie ~33 ms (~30FPS) */
        display_show(disp);
        wait_ms(33);
    }

    /* [9.12] Sprzątanie (nigdy tu nie wejdzie w normalnym przebiegu) */
    if (mixer_ch_playing(sound_channel)) mixer_ch_stop(sound_channel);
    wav64_close(&sound);
    audio_close();
    return 0;
}
