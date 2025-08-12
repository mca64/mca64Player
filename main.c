#include <libdragon.h>
#include <stdio.h>      /* asset_fopen / fread / fclose */
#include <stdint.h>
#include <stdbool.h>
#include <system.h>
#include <wav64.h>
#include <math.h>       /* powf, fabsf */
#include <malloc.h>     /* mallinfo() - używane do odczytu zużycia sterty */

/* ========================================================================
 * 0) Ustawienia ogólne
 * ========================================================================
 * 0.1) Parametry globalne i makra konfiguracyjne używane w całym programie.
 */
#define ANALOG_DEADZONE 8                /* martwa strefa analogów */
#define ARENA_SIZE      (128 * 1024)     /* arena 128KB */

/* --------------------------------------------------------------
 * Ustawienia wygładzania — dopasować do gustu
 * --------------------------------------------------------------
 * 0.2) Parametry do wygładzania (EMA / half-life) i progi zmian.
 */
#define CPU_AVG_SAMPLES 60               /* bufor uśredniania CPU (liczba próbek) */
#define FRAME_MS_ALPHA  0.02f            /* EMA dla czasu klatki — mniejsza = wolniejsze reagowanie */
#define VU_HALF_LIFE_MS 800.0f           /* half-life dla miernika VU (ms) — większe = wolniejsze zanikanie */

/* minimalne progi, aby uniknąć drobnych zmian (zapobiega "skakaniu") */
#define FRAME_MS_DISPLAY_THRESHOLD 0.15f /* ms — minimalna zmiana, aby zaktualizować widoczny wynik */
#define VU_MIN_DELTA 2.0f                /* minimalna różnica VU aby wymusić zamalowanie */

/* ========================================================================
 * 0.3) Deklaracje globalne (stan programu, pola wygładzone)
 * ========================================================================
 *  - Zmienne globalne służą do przechowywania stanu UI / audio / pomiarów.
 */
static uint32_t last_frame_ticks = 0;   /* surowy licznik do obliczania FPS */
static uint32_t fps = 0;                /* surowe FPS (ostatnie obliczenie) */
static float volume = 1.0f;             /* bieżąca głośność */
static char message[128] = "";          /* komunikat HUD */
static int message_timer = 0;           /* licznik wyświetlania komunikatu */

static float last_frame_start_ms = 0.0f; /* start poprzedniej klatki (ms) */
static float last_cpu_ms_display = 0.0f; /* wyświetlany (wygładzony) czas klatki */
static float smoothed_frame_ms = 0.0f;   /* EMA czasu klatki */
static float smoothed_fps = 0.0f;        /* wygładzony FPS (EMA) */
static float smoothed_vu_l = 0.0f;       /* wygładzony poziom L (float) */
static float smoothed_vu_r = 0.0f;       /* wygładzony poziom R (float) */

/* ========================================================================
 * 1) Alokator "arena" (bump allocator)
 * ========================================================================
 * Opis:
 *  1.1) Szybki prosty alokator pracujący w obrębie statycznej tablicy.
 *  1.2) Przeznaczenie: małe, krótkotrwałe buforowanie bez konieczności free().
 *  1.3) Uwaga: zwrócone wskaźniki stają się nieważne po wywołaniu arena_reset().
 */
static uint8_t arena_buf[ARENA_SIZE];
static size_t arena_offset = 0;
static size_t mem_used = 0; /* raport wykorzystania (arena) */

static inline void arena_reset(void) {
    /* 1.4) Resetuje wskaźnik alokacji i licznik użycia (cała arena do ponownego użycia). */
    arena_offset = 0;
    mem_used = 0;
}

static void *arena_alloc(size_t size) {
    /* 1.5) Alokacja z wyrównaniem do 8 bajtów. Zwraca NULL gdy brak miejsca. */
    if (size == 0) return NULL;
    const size_t align = 8;
    size_t off = (arena_offset + (align - 1)) & ~(align - 1);
    if (off + size > ARENA_SIZE) return NULL;
    void *p = &arena_buf[off];
    arena_offset = off + size;
    mem_used += size;
    return p;
}

/* ========================================================================
 * 2) Bufor uśredniania zużycia procesora
 * ========================================================================
 * 2.1) Circular buffer przechowujący ostatnie CPU_AVG_SAMPLES próbek.
 * 2.2) Umożliwia uzyskanie wygładzonego (średniego) wykorzystania CPU.
 */
static float cpu_usage_samples[CPU_AVG_SAMPLES];
static int cpu_sample_index = 0;
static int cpu_sample_count = 0;

static void cpu_usage_add_sample(float usage) {
    /* 2.3) Dodaje nową próbkę do bufora, nadpisując najstarszą. */
    cpu_usage_samples[cpu_sample_index] = usage;
    cpu_sample_index = (cpu_sample_index + 1) % CPU_AVG_SAMPLES;
    if (cpu_sample_count < CPU_AVG_SAMPLES) cpu_sample_count++;
}

static float cpu_usage_get_avg(void) {
    /* 2.4) Zwraca średnią z aktualnych próbek (0 gdy brak próbek). */
    if (cpu_sample_count == 0) return 0.0f;
    float sum = 0.0f;
    for (int i = 0; i < cpu_sample_count; i++) sum += cpu_usage_samples[i];
    return sum / cpu_sample_count;
}

/* ========================================================================
 * 3) Minimalne funkcje pamięciowe/tekstowe (zamiast string.h/memcpy)
 * ========================================================================
 * 3.1) Małe, szybkie implementacje memset/memcpy/strlen/strcpy_s stosowane
 *      aby uniknąć zależności lub mieć kontrolę nad zachowaniem wbudowanych
 *      funkcji.
 */
static inline void *fast_memset(void *dst, int val, size_t n) {
    unsigned char *p = (unsigned char *)dst;
    while (n--) *p++ = (unsigned char)val;
    return dst;
}

static inline void *fast_memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

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

/* ========================================================================
 * 4) Konwersje liczb na tekst
 * ========================================================================
 * 4.1) Funkcje zamieniające int/uint/float na ASCII bez użycia sprintf
 *      (często cięższe i wolniejsze na małych platformach).
 */
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

/* ========================================================================
 * 5) Pomocnik: bajt -> hex + spacja
 * ========================================================================
 * 5.1) Zamienia jeden bajt na 2 znaki hex + spację (np. "7F ").
 */
static inline void u8_to_hex_sp(char *out, uint8_t v) {
    static const char hex[] = "0123456789ABCDEF";
    out[0] = hex[(v >> 4) & 0xF];
    out[1] = hex[v & 0xF];
    out[2] = ' ';
}

/* ========================================================================
 * 6) Formatowanie liczb zmiennoprzecinkowych (1 i 2 miejsca po przecinku)
 * ========================================================================
 * 6.1) Formatowanie bez sprintf, kontrola zaokrągleń i znaków minus.
 */
static int format_float_two_decimals(char *out, double val) {
    if (!out) return 0;
    bool neg = false;
    if (val < 0.0) { neg = true; val = -val; }
    unsigned int v = (unsigned int)(val * 100.0 + 0.5);
    unsigned int whole = v / 100;
    unsigned int frac = v % 100;
    char tmp[16];
    int pos = 0;
    if (neg) tmp[pos++] = '-';
    if (whole == 0) tmp[pos++] = '0'; else {
        char buf[12];
        int wlen = 0;
        unsigned int t = whole;
        while (t) { buf[wlen++] = '0' + (t % 10); t /= 10; }
        for (int i = wlen - 1; i >= 0; i--) tmp[pos++] = buf[i];
    }
    tmp[pos++] = '.';
    tmp[pos++] = '0' + (frac / 10);
    tmp[pos++] = '0' + (frac % 10);
    tmp[pos] = '\0';
    int i = 0;
    while (tmp[i]) { out[i] = tmp[i]; i++; }
    out[i] = '\0';
    return i;
}

static int format_float_one_decimal(char *out, float val) {
    if (!out) return 0;
    bool neg = false;
    if (val < 0.0f) { neg = true; val = -val; }
    int v = (int)(val * 10.0f + 0.5f);
    int whole = v / 10;
    int frac = v % 10;
    char tmp[12];
    int pos = 0;
    if (neg) tmp[pos++] = '-';
    pos += int_to_dec(&tmp[pos], whole);
    tmp[pos++] = '.';
    tmp[pos++] = '0' + frac;
    tmp[pos] = '\0';
    int i = 0;
    while (tmp[i]) { out[i] = tmp[i]; i++; }
    out[i] = '\0';
    return i;
}

/* ========================================================================
 * 7) Pokazuje komunikat na HUD (ustawia timer)
 * ========================================================================
 * 7.1) show_message - ustawia tekst i resetuje timer wyświetlania (~2s).
 */
void show_message(const char *text) {
    if (!text) return;
    strcpy_s(message, (int)sizeof(message), text);
    message_timer = 120; /* ~2s przy 60Hz */
}

/* ========================================================================
 * 8) Aktualizacja ostatnio wciśniętego przycisku (priorytetowe ify)
 * ========================================================================
 * 8.1) update_last_button_pressed - zapisuje w bufie nazwę ostatniego przycisku
 *       (priorytetowo: A > B > START > ... > analog).
 */
static void update_last_button_pressed(char *buf, size_t size, joypad_inputs_t inputs) {
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

/* ========================================================================
 * 9) Format pozycji analogów "X=.. Y=.."
 * ========================================================================
 * 9.1) format_analog - proste złożenie tekstu "X=123 Y=-45".
 */
static void format_analog(char *dst, int x, int y) {
    char *p = dst;
    *p++ = 'X'; *p++ = '=';
    p += int_to_dec(p, x);
    *p++ = ' '; *p++ = 'Y'; *p++ = '=';
    p += int_to_dec(p, y);
    *p = '\0';
}

/* ========================================================================
 * 10) Format czasu "Czas: MM:SS / MM:SS"
 * ========================================================================
 * 10.1) format_time_line - buduje linię z czasem odtwarzania i totalem.
 */
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

/* ========================================================================
 * 11) Rysowanie miernika poziomu (wartości wygładzone przekazywane do funkcji)
 * ========================================================================
 * 11.1) draw_vu_meter - rysuje obramowanie + wypełnienie (vertikalny pasek).
 *        value/max_val => wysokość wypełnienia.
 */
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

/* ========================================================================
 * 12) Okno komunikatu (środkowe)
 * ========================================================================
 * 12.1) draw_message_box - rysuje prostokąt z ramką i centrowanym tekstem.
 */
static void draw_message_box(display_context_t disp, const char *msg, uint32_t frame_color, uint32_t bg_color) {
    if (!msg || msg[0] == '\0') return;
    const int w = 260;
    const int h = 40;
    const int x = (320 - w) / 2;
    const int y = (240 - h) / 2;
    graphics_draw_box(disp, x, y, w, h, bg_color);
    graphics_draw_line(disp, x - 2, y - 2, x + w + 1, y - 2, frame_color);
    graphics_draw_line(disp, x - 2, y + h + 1, x + w + 1, y + h + 1, frame_color);
    graphics_draw_line(disp, x - 2, y - 2, x - 2, y + h + 1, frame_color);
    graphics_draw_line(disp, x + w + 1, y - 2, x + w + 1, y + h + 1, frame_color);
    uint32_t text_color = graphics_make_color(0, 0, 0, 255);
    graphics_set_color(text_color, 0);
    int txt_w = tiny_strlen(msg) * 8;
    int txt_x = x + (w - txt_w) / 2;
    int txt_y = y + (h / 2) - 6;
    graphics_draw_text(disp, txt_x, txt_y, msg);
}

/* ========================================================================
 * 13) Główna funkcja programu (inicjalizacja + pętla)
 * ========================================================================
 * 13.1) Inicjalizacja: display, joypad, DFS, audio, otwarcie pliku WAV64.
 * 13.2) Pętla główna: poll kontrolerów, miks audio, analiza szczytów,
 *       wygładzanie VU, obsługa wejścia, rysowanie UI, pomiary wydajności.
 */
int main(void) {
    const int SOUND_CH = 0;

    /* 13.1a) Inicjalizacja wyświetlacza i kontrolera */
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);
    joypad_init();

    /* 13.1b) Inicjalizacja systemu plików (DFS) - obsługa błędu */
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

    /* 13.1c) Krótki hexdump nagłówka WAV64 (do HUD) */
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
            strcpy_s(header_hex_string, (int)sizeof(header_hex_string), "Blad odczytu");
        }
    }

    /* 13.1d) Timer / FPS init */
    timer_init();
    last_frame_ticks = timer_ticks();

    /* 13.1e) Inicjalizacja kompresji WAV64 (bezpieczne ustawienie wartości) */
    int compression_level = (int)manual_compression_level;
    if (compression_level != 0 && compression_level != 1 && compression_level != 3) compression_level = 0;
    wav64_init_compression(compression_level);

    /* 13.1f) Otwarcie pliku WAV64 i inicjalizacja audio/mixera */
    const char *filename = "rom:/sound.wav64";
    wav64_t sound;
    fast_memset(&sound, 0, sizeof(sound));
    wav64_open(&sound, filename);
    audio_init((int)sound.wave.frequency, 4);
    mixer_init(32);
    wav64_set_loop(&sound, true);

    /* 13.1g) Stan odtwarzania */
    uint32_t current_sample_pos = 0;
    bool is_playing = false;
    int sound_channel = SOUND_CH;

    /* 13.1h) Start odtwarzania */
    current_sample_pos = 0;
    wav64_play(&sound, sound_channel);
    mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
    is_playing = true;
    mixer_ch_set_vol(sound_channel, volume, volume);

    /* 13.1i) Parametry audio (cache) */
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

    /* 13.1j) Bufory w arenie + kolory (alokowane z arena_alloc) */
    char *last_button_pressed = (char *)arena_alloc(32);
    if (last_button_pressed) strcpy_s(last_button_pressed, 32, "Brak");
    char *analog_pos = (char *)arena_alloc(32);
    if (analog_pos) strcpy_s(analog_pos, 32, "X=0 Y=0");
    uint32_t bg_color = graphics_make_color(0, 0, 64, 255);
    uint32_t white    = graphics_make_color(255, 255, 255, 255);
    uint32_t green    = graphics_make_color(0, 255, 0, 255);
    uint32_t box_frame_color = graphics_make_color(0, 200, 0, 255);
    uint32_t box_bg_color    = graphics_make_color(0, 200, 0, 255);
    bool loop_enabled = true;
    char *linebuf = (char *)arena_alloc(256);
    char *hex_line1 = (char *)arena_alloc(40);
    char *hex_line2 = (char *)arena_alloc(40);
    char *perfbuf = (char *)arena_alloc(128);
    char *tmpbuf = (char *)arena_alloc(64);

    /* 13.1k) Zmienne poziomów miernika (szczyty odczytane w klatce) */
    int max_amp = 0, max_amp_l = 0, max_amp_r = 0;

    /* 13.2) Główna pętla - niekończąca się (do momentu resetu/wyłączenia) */
    while (1) {
        float frame_start_ms = get_ticks_ms();
        float frame_interval_ms = (last_frame_start_ms > 0.0f) ? (frame_start_ms - last_frame_start_ms) : (1000.0f / 60.0f);

        max_amp = max_amp_l = max_amp_r = 0;

        /* 13.2a) Odczyt kontrolerów */
        joypad_poll();
        joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
        update_last_button_pressed(last_button_pressed, 32, inputs);

        int ax = inputs.stick_x;
        int ay = inputs.stick_y;
        if ((unsigned)(ax + ANALOG_DEADZONE) < (ANALOG_DEADZONE * 2)) ax = 0;
        if ((unsigned)(ay + ANALOG_DEADZONE) < (ANALOG_DEADZONE * 2)) ay = 0;
        format_analog(analog_pos, ax, ay);

        /* 13.2b) Miksowanie audio i analiza szczytów (jeśli audio możliwe do zapisu) */
        if (audio_can_write()) {
            short *outbuf = audio_write_begin();
            int buf_len = audio_get_buffer_length();
            mixer_poll(outbuf, buf_len);
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

        /* 13.2c) Wygładzanie miernika poziomu (połowiczny zanik) - algorytm half-life */
        {
            float decay_factor = powf(0.5f, frame_interval_ms / VU_HALF_LIFE_MS);
            float peak_l = (float)max_amp_l;
            float cur_l = smoothed_vu_l * decay_factor;
            if (peak_l > cur_l) cur_l = peak_l;
            if (fabsf(cur_l - smoothed_vu_l) > VU_MIN_DELTA) smoothed_vu_l = cur_l;

            float peak_r = (float)max_amp_r;
            float cur_r = smoothed_vu_r * decay_factor;
            if (peak_r > cur_r) cur_r = peak_r;
            if (fabsf(cur_r - smoothed_vu_r) > VU_MIN_DELTA) smoothed_vu_r = cur_r;
        }

        /* 13.2d) Auto restart / koniec utworu (zależnie od loop_enabled) */
        if (!mixer_ch_playing(sound_channel)) {
            if (loop_enabled) {
                wav64_play(&sound, sound_channel);
                mixer_ch_set_pos(sound_channel, 0.0f);
                mixer_ch_set_vol(sound_channel, volume, volume);
                is_playing = true;
                current_sample_pos = 0;
                show_message("Restart (petla)");
            } else {
                if (is_playing) {
                    is_playing = false;
                    current_sample_pos = total_samples;
                    show_message("Koniec");
                }
            }
        }

        /* 13.2e) Obliczenie pozycji aktualnej do wyświetlenia */
        uint32_t current_sample_pos_display = current_sample_pos;
        if (is_playing && mixer_ch_playing(sound_channel)) {
            float pos_f = mixer_ch_get_pos(sound_channel);
            if (pos_f < 0.0f) pos_f = 0.0f;
            uint64_t pos_u = (uint64_t)(pos_f + 0.5f);
            if (pos_u > (uint64_t)total_samples) pos_u = (uint64_t)total_samples;
            current_sample_pos_display = (uint32_t)pos_u;
        }

        uint32_t elapsed_sec = (sample_rate) ? (current_sample_pos_display / sample_rate) : 0;
        uint32_t play_min = elapsed_sec / 60U;
        uint32_t play_sec = elapsed_sec % 60U;

        /* 13.2f) Rysowanie UI - ekran, tytuł, informacje */
        surface_t *disp = display_get();
        graphics_fill_screen(disp, bg_color);
        graphics_set_color(white, 0);

        const char *title = "mca64Player";
        int title_x = (320 - tiny_strlen(title) * 8) / 2;
        graphics_draw_text(disp, title_x, 4, title);
        graphics_draw_text(disp, 4, 4, last_button_pressed);
        graphics_draw_text(disp, 4, 16, analog_pos);

        strcpy_s(linebuf, 128, "Plik: ");
        int off = tiny_strlen(linebuf);
        int remaining = (int)128 - off;
        strcpy_s(&linebuf[off], remaining, filename);
        graphics_draw_text(disp, 10, 28, linebuf);

        format_time_line(linebuf, (unsigned)play_min, (unsigned)play_sec,
                         (unsigned)total_minutes, (unsigned)total_secs_rem);
        graphics_draw_text(disp, 10, 46, linebuf);

        /* 13.2g) Pasek postępu */
        int bar_x = 10, bar_y = 58, bar_w = 300, bar_h = 8;
        graphics_draw_box(disp, bar_x, bar_y, bar_w, bar_h, white);
        if (total_seconds > 0) {
            int pw = (int)(((uint64_t)elapsed_sec * (uint64_t)bar_w) / (uint64_t)total_seconds);
            if (pw > bar_w) pw = bar_w;
            graphics_draw_box(disp, bar_x, bar_y, pw, bar_h, green);
        }

        /* 13.2h) Parametry audio (tekst) */
        strcpy_s(linebuf, 128, "Probkowanie: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)sample_rate);
        strcpy_s(&linebuf[off], 128 - off, " Hz");
        graphics_draw_text(disp, 10, 76, linebuf);

        strcpy_s(linebuf, 128, "Bitrate: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)bitrate_kbps);
        strcpy_s(&linebuf[off], 128 - off, " kbps");
        graphics_draw_text(disp, 10, 94, linebuf);

        if (channels == 1) strcpy_s(linebuf, 128, "Kanaly: Mono (1)");
        else strcpy_s(linebuf, 128, "Kanaly: Stereo (2)");
        graphics_draw_text(disp, 10, 112, linebuf);

        strcpy_s(linebuf, 128, "Bity: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)bits_per_sample);
        strcpy_s(&linebuf[off], 128 - off, " bit");
        graphics_draw_text(disp, 10, 130, linebuf);

        strcpy_s(linebuf, 128, "Kompresja: ");
        off = tiny_strlen(linebuf);
        if (manual_compression_level == 0) strcpy_s(&linebuf[off], 128 - off, "PCM");
        else if (manual_compression_level == 1) strcpy_s(&linebuf[off], 128 - off, "VADPCM");
        else if (manual_compression_level == 3) strcpy_s(&linebuf[off], 128 - off, "Opus");
        else { off += int_to_dec(&linebuf[off], (int)manual_compression_level); }
        graphics_draw_text(disp, 10, 166, linebuf);

        strcpy_s(linebuf, 128, "Probek: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)total_samples);
        graphics_draw_text(disp, 10, 184, linebuf);

        /* 13.2i) Mierniki poziomu */
        int vu_base_x = 280;
        int vu_base_y = 200;
        int vu_width = 8;
        int vu_height = 40;
        int draw_vu_l = (int)smoothed_vu_l;
        int draw_vu_r = (int)smoothed_vu_r;
        if (channels == 2) {
            draw_vu_meter(disp, vu_base_x - 0, vu_base_y, vu_width, vu_height, draw_vu_l, 32768, white, green, "L");
            draw_vu_meter(disp, vu_base_x + vu_width + 10, vu_base_y, vu_width, vu_height, draw_vu_r, 32768, white, green, "R");
        } else {
            int mono_v = (draw_vu_l > draw_vu_r) ? draw_vu_l : draw_vu_r;
            if (mono_v == 0) mono_v = max_amp;
            draw_vu_meter(disp, vu_base_x + 8, vu_base_y, vu_width, vu_height, mono_v, 32768, white, green, "Mono");
        }

        /* 13.2j) Nagłówek hex (2 linie) */
        int hex_len = tiny_strlen(header_hex_string);
        int groups = hex_len / 3;
        int split_groups = groups / 2;
        int split_pos = split_groups * 3;
        int i = 0, idx = 0;
        for (i = 0; i < split_pos && i < (int)40 - 1; i++) hex_line1[i] = header_hex_string[i];
        hex_line1[i] = '\0';
        idx = 0;
        for (i = split_pos; i < hex_len && idx < (int)40 - 1; i++) hex_line2[idx++] = header_hex_string[i];
        hex_line2[idx] = '\0';
        graphics_draw_text(disp, 10, 202, hex_line1);
        graphics_draw_text(disp, 10, 212, hex_line2);

        /* 13.2k) Liczniki wydajności: surowy FPS i wygładzenie do wyświetlenia */
        {
            uint32_t now_ticks = timer_ticks();
            uint32_t diff_ticks = now_ticks - last_frame_ticks;
            if (last_frame_ticks != 0 && diff_ticks > 0) {
                fps = TICKS_PER_SECOND / diff_ticks;
            }
            last_frame_ticks = now_ticks;
        }
        if (smoothed_fps <= 0.0f) smoothed_fps = (float)fps;
        else smoothed_fps = (1.0f - FRAME_MS_ALPHA) * smoothed_fps + FRAME_MS_ALPHA * (float)fps;

        strcpy_s(perfbuf, 128, "FPS: ");
        off = tiny_strlen(perfbuf);
        off += int_to_dec(&perfbuf[off], (int)(smoothed_fps + 0.5f));
        graphics_draw_text(disp, 10, 230, perfbuf);

        /* 13.2l) Pomiar RAM: użycie arena + mallinfo + obliczenie wolnego RAM w MB
           13.2l.1) ram_total = get_memory_size() w MB
           13.2l.2) used_malloc = mallinfo().uordblks (bajty)
           13.2l.3) used_arena = mem_used (bajty)
           13.2l.4) used_total_mb = (used_malloc + used_arena) / (1024*1024)
           13.2l.5) ram_free = ram_total - used_total_mb (clamp >=0)
        */
        double ram_total = get_memory_size() / (1024.0 * 1024.0); /* w MB */
        struct mallinfo mi = mallinfo();
        double used_malloc = (double)mi.uordblks; /* bajty */
        double used_arena = (double)mem_used; /* bajty */
        double used_total_mb = (used_malloc + used_arena) / (1024.0 * 1024.0);
        double ram_free = ram_total - used_total_mb;
        if (ram_free < 0.0) ram_free = 0.0; /* zabezpieczenie */

        /* 13.2m) Rysuj RAM i Free (formatowane dwoma miejscami po przecinku) */
        strcpy_s(perfbuf, 128, "RAM: ");
        format_float_two_decimals(&perfbuf[tiny_strlen(perfbuf)], ram_total);
        off = tiny_strlen(perfbuf);
        strcpy_s(&perfbuf[off], 128 - off, " MB");
        graphics_draw_text(disp, 74, 230, perfbuf);

        strcpy_s(perfbuf, 128, "Free: ");
        format_float_two_decimals(&perfbuf[tiny_strlen(perfbuf)], ram_free);
        off = tiny_strlen(perfbuf);
        strcpy_s(&perfbuf[off], 128 - off, " MB");
        graphics_draw_text(disp, 200, 230, perfbuf);

        /* 13.2n) Obsługa wejścia (krawędź narastająca) - odtwarzanie i przewijanie */
        joypad_buttons_t keys = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        if (keys.start || keys.a) {
            if (is_playing && mixer_ch_playing(sound_channel)) {
                float pos_f = mixer_ch_get_pos(sound_channel);
                if (pos_f < 0.0f) pos_f = 0.0f;
                uint64_t pos_u = (uint64_t)(pos_f + 0.5f);
                if (pos_u > (uint64_t)total_samples) pos_u = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)pos_u;
                mixer_ch_stop(sound_channel);
                is_playing = false;
                show_message("Pauza");
            } else {
                if (compression_level != 0 && current_sample_pos != 0) {
                    current_sample_pos = 0;
                    show_message("Niedostepne - start od poczatku");
                }
                wav64_play(&sound, sound_channel);
                mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                mixer_ch_set_vol(sound_channel, volume, volume);
                is_playing = true;
                show_message("Wznowiono");
            }
        }
        if (keys.b) {
            if (mixer_ch_playing(sound_channel)) mixer_ch_stop(sound_channel);
            current_sample_pos = 0;
            is_playing = false;
            show_message("Stop");
        }
        if (keys.l || keys.d_left) {
            if (compression_level == 0) {
                int64_t delta_samples = (int64_t)5 * (int64_t)sample_rate;
                int64_t newpos = (int64_t)current_sample_pos_display - delta_samples;
                if (newpos < 0) newpos = 0;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie -5s");
            } else show_message("Niedostepne dla tego formatu");
        }
        if (keys.r || keys.d_right) {
            if (compression_level == 0) {
                uint64_t delta_samples = (uint64_t)5 * (uint64_t)sample_rate;
                uint64_t newpos = (uint64_t)current_sample_pos_display + delta_samples;
                if (newpos > (uint64_t)total_samples) newpos = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie +5s");
            } else show_message("Niedostepne dla tego formatu");
        }
        if (keys.c_left) {
            if (compression_level == 0) {
                int64_t delta_samples = (int64_t)30 * (int64_t)sample_rate;
                int64_t newpos = (int64_t)current_sample_pos_display - delta_samples;
                if (newpos < 0) newpos = 0;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie -30s");
            } else show_message("Niedostepne dla tego formatu");
        }
        if (keys.c_right) {
            if (compression_level == 0) {
                uint64_t delta_samples = (uint64_t)30 * (uint64_t)sample_rate;
                uint64_t newpos = (uint64_t)current_sample_pos_display + delta_samples;
                if (newpos > (uint64_t)total_samples) newpos = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie +30s");
            } else show_message("Niedostepne dla tego formatu");
        }

        /* 13.2o) Regulacja głośności */
        if (keys.d_up || keys.c_up) {
            volume += 0.1f;
            if (volume > 1.0f) volume = 1.0f;
            mixer_ch_set_vol(sound_channel, volume, volume);
            strcpy_s(tmpbuf, 64, "Glosnosc: ");
            int len = tiny_strlen(tmpbuf);
            format_float_one_decimal(&tmpbuf[len], volume);
            show_message(tmpbuf);
        }
        if (keys.d_down || keys.c_down) {
            volume -= 0.1f;
            if (volume < 0.0f) volume = 0.0f;
            mixer_ch_set_vol(sound_channel, volume, volume);
            strcpy_s(tmpbuf, 64, "Glosnosc: ");
            int len = tiny_strlen(tmpbuf);
            format_float_one_decimal(&tmpbuf[len], volume);
            show_message(tmpbuf);
        }

        /* 13.2p) Włączenie/wyłączenie pętli (klawisz Z) */
        if (keys.z) {
            loop_enabled = !loop_enabled;
            wav64_set_loop(&sound, loop_enabled);
            if (loop_enabled) show_message("Pętla: WŁĄCZONA"); else show_message("Pętla: WYŁĄCZONA");
        }

        /* 13.2q) Rysowanie okna komunikatu (jeśli jest aktywny) */
        if (message_timer > 0) {
            draw_message_box(disp, message, box_frame_color, box_bg_color);
            message_timer--;
            graphics_set_color(white, 0);
        }

        /* 13.2r) Rysowanie średniego zużycia procesora i czasu klatki */
        {
            int cpu_avg_int = (int)(cpu_usage_get_avg() + 0.5f);
            strcpy_s(perfbuf, 128, "CPU: ");
            int p = tiny_strlen(perfbuf);
            p += int_to_dec(&perfbuf[p], cpu_avg_int);
            perfbuf[p++] = '%';
            perfbuf[p++] = ' ';
            format_float_two_decimals(&perfbuf[p], (double)last_cpu_ms_display);
            p = tiny_strlen(perfbuf);
            strcpy_s(&perfbuf[p], 128 - p, "ms");
            graphics_draw_text(disp, 180, 16, perfbuf);
        }

        /* 13.2s) Koniec rysowania - wyświetlenie bufora */
        display_show(disp);

        /* 13.2t) Pomiar końcowy klatki, obliczenie measured_ms i procentu CPU */
        float frame_end_ms = get_ticks_ms();
        float measured_ms = frame_end_ms - frame_start_ms;

        /* 13.2u) Poprawka interwału między startami (bezpieczny fallback) */
        if (last_frame_start_ms > 0.0f) frame_interval_ms = frame_start_ms - last_frame_start_ms;
        if (frame_interval_ms <= 0.0f) frame_interval_ms = (measured_ms > 0.0f) ? measured_ms : (1000.0f / 60.0f);

        float cpu_percent = (frame_interval_ms > 0.0f) ? ((measured_ms / frame_interval_ms) * 100.0f) : 0.0f;
        if (cpu_percent > 100.0f) cpu_percent = 100.0f;
        if (cpu_percent < 0.0f) cpu_percent = 0.0f;
        cpu_usage_add_sample(cpu_percent);

        /* 13.2v) Wygładzenie czasu klatki (EMA) - bardzo łagodne */
        if (smoothed_frame_ms <= 0.0f) smoothed_frame_ms = measured_ms;
        else {
            float new_smoothed = (1.0f - FRAME_MS_ALPHA) * smoothed_frame_ms + FRAME_MS_ALPHA * measured_ms;
            if (fabsf(new_smoothed - smoothed_frame_ms) > FRAME_MS_DISPLAY_THRESHOLD) smoothed_frame_ms = new_smoothed;
        }
        last_cpu_ms_display = smoothed_frame_ms;

        /* 13.2w) Zachowanie startu klatki dla następnej iteracji */
        last_frame_start_ms = frame_start_ms;

        /* 13.2x) Krótki oddech (yield) - zapobiega spin-loop */
        wait_ms(2);
    }

    /* 13.3) Sprzątanie (zwykle nieosiągalne w tej wersji programu) */
    if (mixer_ch_playing(sound_channel)) mixer_ch_stop(sound_channel);
    wav64_close(&sound);
    audio_close();
    return 0;
}
