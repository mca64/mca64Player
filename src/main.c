/* --- standard / C library --- */
#include <stdio.h>      /* asset_fopen / fread / fclose */
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>     /* abs, malloc/free if needed */
#include <math.h>       /* powf, fabsf */
#include <malloc.h>     /* mallinfo() */
#include <system.h>     /* jeśli używasz funkcji systemowych libdragon */

/* --- libdragon --- */
#include <libdragon.h>

/* --- projektowe nagłówki (lokalne) --- */
#include "menu.h"
#include "utils.h"
#include "debug.h"
#include "arena.h"
#include "cpu_usage.h"
#include "vu.h"
#include "hud.h"

/* [1] Ustawienia ogólne -------------------------------------------------- */
#define SCREEN_W 640
#define SCREEN_H 288
#define ANALOG_DEADZONE 8

/* [2] Ustawienia wygładzania --------------------------------------------- */
#define FRAME_MS_ALPHA 0.02f
#define VU_HALF_LIFE_MS 800.0f
#define FRAME_MS_DISPLAY_THRESHOLD 0.15f
#define VU_MIN_DELTA 2.0f

/* [3] Deklaracje globalne (stan programu, pola wygładzone) --------------- */
static uint32_t last_frame_ticks = 0;   /* [3.1] Surowy licznik FPS */
static uint32_t fps = 0;                /* [3.2] Surowe FPS (ostatnie obliczenie) */
static float volume = 1.0f;             /* [3.3] Bieżąca głośność */
static float last_frame_start_ms = 0.0f;/* [3.6] Start poprzedniej klatki (ms) */
static float last_cpu_ms_display = 0.0f;/* [3.7] Wyświetlany (wygładzony) czas klatki */
static float smoothed_frame_ms = 0.0f;  /* [3.8] EMA czasu klatki */
static float smoothed_fps = 0.0f;       /* [3.9] Wygładzony FPS (EMA) */
static const resolution_t *current_resolution = NULL; /* [3.12] Wybrana rozdzielczosc (persistent) */





/* [17] Główna funkcja programu (inicjalizacja + pętla) ----------------- */
int main(void) {
    /* [17.1] Stałe lokalne */
    const int SOUND_CH = 0;
    static const resolution_t PAL = {SCREEN_W, SCREEN_H, false};

    /* [17.2] Inicjalizacja display i joypad */
    display_init(PAL, DEPTH_32_BPP, 2, GAMMA_NONE, ANTIALIAS_OFF);
    joypad_init();

    /* [17.2.1] Ustaw domyślną, trwałą rozdzielczość HUD (obecny tryb wyświetlacza) */
    current_resolution = &PAL;

    /* [17.3] Inicjalizacja systemu plików (DFS) i obsługa błędu */
    if (dfs_init(DFS_DEFAULT_LOCATION) != DFS_ESUCCESS) {
        surface_t *disp = display_get();
        uint32_t black = graphics_make_color(0, 0, 0, 255);
        uint32_t red = graphics_make_color(255, 0, 0, 255);
        graphics_fill_screen(disp, black);
        graphics_set_color(red, 0);
        graphics_draw_text(disp, 10, 10, "Blad: nie mozna zainicjowac DFS");
        display_show(disp);
        return 1;
    }

    /* [17.4] Krótki hexdump nagłówka WAV64 (do HUD) */
    uint8_t manual_compression_level = 0;
    char header_hex_string[64]; header_hex_string[0] = '\0';
    int dfs_bytes_read = -1;
    
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
    

    /* [17.5] Timer / FPS init */
    timer_init();
    last_frame_ticks = timer_ticks();

    /* [17.6] Inicjalizacja kompresji WAV64 */
    int compression_level = (int)manual_compression_level;
    if (compression_level != 0 && compression_level != 1 && compression_level != 3) compression_level = 0;
    wav64_init_compression(compression_level);

    /* [17.7] Otwarcie pliku WAV64 i inicjalizacja audio/mixera */
    const char *filename = "rom:/sound.wav64";
    wav64_t sound;
    fast_memset(&sound, 0, sizeof(sound));
    wav64_open(&sound, filename);
    audio_init((int)sound.wave.frequency, 4);
    mixer_init(32);
    wav64_set_loop(&sound, true);

    /* [17.8] Stan odtwarzania */
    uint32_t current_sample_pos = 0;
    bool is_playing = false;
    int sound_channel = SOUND_CH;

    /* [17.9] Start odtwarzania */
    current_sample_pos = 0;
    wav64_play(&sound, sound_channel);
    mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
    is_playing = true;
    mixer_ch_set_vol(sound_channel, volume, volume);

    /* [17.10] Parametry audio (cache) */
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

    /* [17.11] Bufory w arenie + kolory */
    char *last_button_pressed = (char *)arena_alloc(32);
    if (last_button_pressed) strcpy_s(last_button_pressed, 32, "Brak");
    char *analog_pos = (char *)arena_alloc(32);
    if (analog_pos) strcpy_s(analog_pos, 32, "X=0 Y=0");
    uint32_t bg_color = graphics_make_color(0, 0, 64, 255);
    uint32_t white = graphics_make_color(255, 255, 255, 255);
    uint32_t green = graphics_make_color(0, 255, 0, 255);
    uint32_t box_frame_color = graphics_make_color(0, 200, 0, 255);
    uint32_t box_bg_color = graphics_make_color(0, 200, 0, 255);
    bool loop_enabled = true;
    char *linebuf = (char *)arena_alloc(256);
    char *hex_line1 = (char *)arena_alloc(40);
    char *hex_line2 = (char *)arena_alloc(40);
    char *perfbuf = (char *)arena_alloc(128);
    char *tmpbuf = (char *)arena_alloc(64);

    /* [17.12] Zmienne poziomów miernika (szczyty odczytane w klatce) */
    int max_amp = 0, max_amp_l = 0, max_amp_r = 0;

    /* [17.13] Główna pętla programu */
    while (1) {
        float frame_start_ms = get_ticks_ms();
        float frame_interval_ms = (last_frame_start_ms > 0.0f) ? (frame_start_ms - last_frame_start_ms) : (1000.0f / 60.0f);
        max_amp = max_amp_l = max_amp_r = 0;

        /* [17.14] Odczyt analogów / ostatni wciśnięty przycisk */
        joypad_poll();
        joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
        update_last_button_pressed(last_button_pressed, 32, inputs);
        int ax = inputs.stick_x;
        int ay = inputs.stick_y;
        /* [17.14.1] Martwa strefa analogów — używa abs() dla czytelności */
        if (abs(ax) <= ANALOG_DEADZONE) ax = 0;
        if (abs(ay) <= ANALOG_DEADZONE) ay = 0;
        format_analog(analog_pos, ax, ay);

        /* [17.15] Pobranie przycisków krawędziowych i trzymanych */
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);

        /* [17.16] Miksowanie audio i analiza szczytów */
        while (audio_can_write()) {                  // if
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



        /* [17.18] Auto-restart lub zakończenie utworu */
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

        /* [17.19] Obliczenie aktualnej pozycji do wyświetlenia */
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

        /* [17.20] Rysowanie UI - ekran, tytuł, informacje */
        surface_t *disp = display_get();
        graphics_fill_screen(disp, bg_color);
        graphics_set_color(white, 0);
        const char *title = "mca64Player";
        int title_x = ((int)display_get_width() - tiny_strlen(title) * 8) / 2;
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

        /* [17.21] Pasek postępu */
        int bar_x = 10, bar_y = 58, bar_w = 300, bar_h = 8;
        graphics_draw_box(disp, bar_x, bar_y, bar_w, bar_h, white);
        if (total_seconds > 0) {
            int pw = (int)(((uint64_t)elapsed_sec * (uint64_t)bar_w) / (uint64_t)total_seconds);
            if (pw > bar_w) pw = bar_w;
            graphics_draw_box(disp, bar_x, bar_y, pw, bar_h, green);
        }

        /* [17.22] Parametry audio (tekst) */
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

        /* [17.22.1] ROZDZIELCZOSC: wyświetla aktualnie zapisaną (stałą) rozdzielczość
           [17.22.1.1] Jeśli current_resolution == NULL — wypisuje bieżące wymiary display_get_*. */
        if (current_resolution) {
            strcpy_s(linebuf, 128, "Rozdzielczosc: ");
            off = tiny_strlen(linebuf);
            off += int_to_dec(&linebuf[off], (int)current_resolution->width);
            linebuf[off++] = 'x';
            off += int_to_dec(&linebuf[off], (int)current_resolution->height);
            linebuf[off++] = current_resolution->interlaced ? 'i' : 'p';
            linebuf[off] = '\0';
        } else {
            strcpy_s(linebuf, 128, "Rozdzielczosc: ");
            off = tiny_strlen(linebuf);
            off += int_to_dec(&linebuf[off], display_get_width());
            linebuf[off++] = 'x';
            off += int_to_dec(&linebuf[off], display_get_height());
            linebuf[off] = '\0';
        }
        graphics_draw_text(disp, 10, 148, linebuf);

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

        /* [17.23] Mierniki poziomu */
		
		

		
        {	vu_update(frame_interval_ms, max_amp_l, max_amp_r);
            int vu_base_x = 280;
            int vu_base_y = 200;
            int vu_width = 8;
            int vu_height = 40;
		    int draw_vu_l = vu_get_left();
			int draw_vu_r = vu_get_right();
            if (channels == 2) {
                draw_vu_meter(disp, vu_base_x - 0, vu_base_y, vu_width, vu_height, draw_vu_l, 32768, white, green, "L");
                draw_vu_meter(disp, vu_base_x + vu_width + 10, vu_base_y, vu_width, vu_height, draw_vu_r, 32768, white, green, "R");
            } else {
                int mono_v = (draw_vu_l > draw_vu_r) ? draw_vu_l : draw_vu_r;
                if (mono_v == 0) mono_v = max_amp;
                draw_vu_meter(disp, vu_base_x + 8, vu_base_y, vu_width, vu_height, mono_v, 32768, white, green, "Mono");
            }
        }

        /* [17.24] Nagłówek hex (2 linie) */
        {
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
        }

        /* [17.25] Liczniki wydajności: surowy FPS i wygładzone FPS,  Rysowanie średniego zużycia procesora i czasu klatki */
        {
            uint32_t now_ticks = timer_ticks();
            uint32_t diff_ticks = now_ticks - last_frame_ticks;
            if (last_frame_ticks != 0 && diff_ticks > 0) {
                fps = TICKS_PER_SECOND / diff_ticks;
            }
            last_frame_ticks = now_ticks;
        }
		
		
		        /* [17.xx] Rysowanie średniego zużycia procesora i czasu klatki */
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
		
		
        if (smoothed_fps <= 0.0f) smoothed_fps = (float)fps;
        else smoothed_fps = (1.0f - FRAME_MS_ALPHA) * smoothed_fps + FRAME_MS_ALPHA * (float)fps;

        strcpy_s(perfbuf, 128, "FPS: ");
        off = tiny_strlen(perfbuf);
        off += int_to_dec(&perfbuf[off], (int)(smoothed_fps + 0.5f));
        graphics_draw_text(disp, 10, 230, perfbuf);

        /* 17.2l) Pomiar RAM: użycie arena + mallinfo + obliczenie wolnego RAM w MB
           17.2l.1) ram_total = get_memory_size() w MB
           17.2l.2) used_malloc = mallinfo().uordblks (bajty)
           17.2l.3) used_arena = mem_used (bajty)
           17.2l.4) used_total_mb = (used_malloc + used_arena) / (1024*1024)
           17.2l.5) ram_free = ram_total - used_total_mb (clamp >=0)
        */
        double ram_total = get_memory_size() / (1024.0 * 1024.0); /* w MB */
        struct mallinfo mi = mallinfo();
        double used_malloc = (double)mi.uordblks; /* bajty */
        double used_arena = (double)arena_get_used(); /* bytes */ 
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

        /* [17.27] Obsługa wejścia: otwarcie menu przy START */
        {
            if (pressed.start && !menu_is_open()) {
				menu_set_initial_resolution(current_resolution);
                menu_open();
                pressed.start = 0;
            }
            if (menu_is_open()) {
                const resolution_t *sel = NULL;
                menu_status_t st = menu_update(disp, pressed, held, &sel);
                display_show(disp);
                //wait_ms(1);
                if (st == MENU_STATUS_SELECTED && sel) {
                    /* [17.27.1] Zapisz wybór globalnie, zmień tryb wyświetlacza i pokaż komunikat */
                    current_resolution = sel; /* <- zapamiętanie wyboru */
                    display_close();
                    //wait_ms(1);
                    display_init(*sel, DEPTH_32_BPP, 2, GAMMA_NONE, ANTIALIAS_OFF);
                    menu_close();
                    char buf[64];
                    strcpy_s(buf, 64, "Wybrano: ");
                    int pos = tiny_strlen(buf);
                    pos += int_to_dec(&buf[pos], (int)sel->width);
                    pos = safe_append_str(buf, 64, pos, " x ");
                    pos += int_to_dec(&buf[pos], (int)sel->height);
                    pos = safe_append_str(buf, 64, pos, " ");
                    pos = safe_append_str(buf, 64, pos, sel->interlaced ? "i" : "p");
                    show_message(buf);
                } else if (st == MENU_STATUS_CANCEL) {
                    show_message("Anulowano");
                    menu_close();
                }
                continue;
            }
        }

        /* [17.28] Obsługa przycisków — play/pause/stop/przewijanie */
        if (pressed.a) {
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
        if (pressed.b) {
            if (mixer_ch_playing(sound_channel)) mixer_ch_stop(sound_channel);
            current_sample_pos = 0;
            is_playing = false;
            show_message("Stop");
        }
        if (pressed.l || pressed.d_left) {
            if (compression_level == 0) {
                int64_t delta_samples = (int64_t)5 * (int64_t)sample_rate;
                int64_t newpos = (int64_t)current_sample_pos_display - delta_samples;
                if (newpos < 0) newpos = 0;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie -5s");
            } else show_message("Niedostepne dla tego formatu");
        }
        if (pressed.r || pressed.d_right) {
            if (compression_level == 0) {
                uint64_t delta_samples = (uint64_t)5 * (uint64_t)sample_rate;
                uint64_t newpos = (uint64_t)current_sample_pos_display + delta_samples;
                if (newpos > (uint64_t)total_samples) newpos = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie +5s");
            } else show_message("Niedostepne dla tego formatu");
        }
        if (pressed.c_left) {
            if (compression_level == 0) {
                int64_t delta_samples = (int64_t)30 * (int64_t)sample_rate;
                int64_t newpos = (int64_t)current_sample_pos_display - delta_samples;
                if (newpos < 0) newpos = 0;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie -30s");
            } else show_message("Niedostepne dla tego formatu");
        }
        if (pressed.c_right) {
            if (compression_level == 0) {
                uint64_t delta_samples = (uint64_t)30 * (uint64_t)sample_rate;
                uint64_t newpos = (uint64_t)current_sample_pos_display + delta_samples;
                if (newpos > (uint64_t)total_samples) newpos = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Przewijanie +30s");
            } else show_message("Niedostepne dla tego formatu");
        }

        /* [17.29] Regulacja głośności (d_up/d_down/c_up/c_down) */
        if (pressed.d_up || pressed.c_up) {
            volume += 0.1f;
            if (volume > 1.0f) volume = 1.0f;
            mixer_ch_set_vol(sound_channel, volume, volume);
            strcpy_s(tmpbuf, 64, "Glosnosc: ");
            int len = tiny_strlen(tmpbuf);
            format_float_one_decimal(&tmpbuf[len], volume);
            show_message(tmpbuf);
        }
        if (pressed.d_down || pressed.c_down) {
            volume -= 0.1f;
            if (volume < 0.0f) volume = 0.0f;
            mixer_ch_set_vol(sound_channel, volume, volume);
            strcpy_s(tmpbuf, 64, "Glosnosc: ");
            int len = tiny_strlen(tmpbuf);
            format_float_one_decimal(&tmpbuf[len], volume);
            show_message(tmpbuf);
        }

        /* [17.30] Włączenie/wyłączenie pętli (Z) */
        if (pressed.z) {
            loop_enabled = !loop_enabled;
            wav64_set_loop(&sound, loop_enabled);
            if (loop_enabled) show_message("Pętla: WŁĄCZONA"); else show_message("Pętla: WYŁĄCZONA");
        }

        /* [17.31] Rysowanie okna komunikatu (jeżeli aktywne) */
			hud_draw_message(disp, box_frame_color, box_bg_color);

		debug_info(disp,
           (int)sample_rate,
           last_cpu_ms_display,
           cpu_usage_get_avg(),
           smoothed_fps,
           (int)(ram_free * 1024.0 * 1024.0), /* free RAM jako bajty */
           320, 12);

        /* [17.33] Wyświetlenie bufora (koniec rysowania) */
        display_show(disp);

        /* [17.34] Pomiar końcowy klatki i obliczenie użycia CPU */
        float frame_end_ms = get_ticks_ms();
        float measured_ms = frame_end_ms - frame_start_ms;
        if (last_frame_start_ms > 0.0f) frame_interval_ms = frame_start_ms - last_frame_start_ms;
        if (frame_interval_ms <= 0.0f) frame_interval_ms = (measured_ms > 0.0f) ? measured_ms : (1000.0f / 60.0f);
        float cpu_percent = (frame_interval_ms > 0.0f) ? ((measured_ms / frame_interval_ms) * 100.0f) : 0.0f;
        if (cpu_percent > 100.0f) cpu_percent = 100.0f;
        if (cpu_percent < 0.0f) cpu_percent = 0.0f;
        cpu_usage_add_sample(cpu_percent);

        /* [17.35] Wygładzenie czasu klatki (EMA) */
        if (smoothed_frame_ms <= 0.0f) smoothed_frame_ms = measured_ms;
        else {
            float new_smoothed = (1.0f - FRAME_MS_ALPHA) * smoothed_frame_ms + FRAME_MS_ALPHA * measured_ms;
            if (fabsf(new_smoothed - smoothed_frame_ms) > FRAME_MS_DISPLAY_THRESHOLD) smoothed_frame_ms = new_smoothed;
        }
        last_cpu_ms_display = smoothed_frame_ms;
        /* [17.36] Zachowanie startu klatki i krótki yield */
        last_frame_start_ms = frame_start_ms;
        wait_ms(1);
    }

    /* [17.37] Sprzątanie (raczej nieosiągalne w normalnym przebiegu) */
    if (mixer_ch_playing(sound_channel)) mixer_ch_stop(sound_channel);
    wav64_close(&sound);
    audio_close();
    return 0;
}
