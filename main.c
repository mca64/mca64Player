#include <libdragon.h>
#include <stdio.h>
#include <stdint.h>

/* ---------------------------------------------------------
 *  [1] Funkcja oblicza długość łańcucha znaków
 * --------------------------------------------------------- */
int custom_strlen(const char *str) {
    int len = 0;
    while (str[len] != '\0') len++;
    return len;
}

/* ---------------------------------------------------------
 *  [2] Funkcja zwraca nazwę kompresji dla podanego poziomu
 * --------------------------------------------------------- */
const char* get_compression_name(int level) {
    switch (level) {
        case 0: return "PCM (brak kompresji)";
        case 1: return "VADPCM (domyslny)";
        case 3: return "Opus";
        default: return "Nieznany / nieobslugiwany";
    }
}

int main(void) {
    /* [3] Inicjalizuje ekran */
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, ANTIALIAS_RESAMPLE);

    /* [4] Inicjalizuje DFS i sprawdza błąd */
    if (dfs_init(DFS_DEFAULT_LOCATION) != DFS_ESUCCESS) {
        surface_t *disp = display_get();
        graphics_fill_screen(disp, graphics_make_color(0, 0, 0, 255));
        graphics_set_color(graphics_make_color(255, 0, 0, 255), 0);
        graphics_draw_text(disp, 10, 10, "Blad: nie mozna zainicjowac DFS");
        display_show(disp);
        return 1;
    }

    /* [5] Odczytuje nagłówek WAV64 i pobiera poziom kompresji */
    uint8_t manual_compression_level = 0;
    char header_hex_string[64] = "Read error";
    int dfs_fd = -1, dfs_bytes_read = -1;
    {
        char header_buf[16];
        int file_size = 0;
        FILE *f = asset_fopen("rom:/sound.wav64", &file_size);
        if (f) {
            dfs_fd = 0;
            dfs_bytes_read = fread(header_buf, 1, sizeof(header_buf), f);
            if (dfs_bytes_read > 0 && dfs_bytes_read >= 15) {
                manual_compression_level = (uint8_t)header_buf[5];
            }
            char *ptr = header_hex_string;
            ptr[0] = '\0';
            for (int i = 0; i < dfs_bytes_read; i++) {
                snprintf(ptr, 4, "%02X ", (uint8_t)header_buf[i]);
                ptr += 3;
            }
            fclose(f);
        }
    }

    /* [6] Inicjalizuje timer */
    timer_init();

    /* [7] Ustawia poziom kompresji na wartość wykrytą w nagłówku */
    int compression_level = manual_compression_level;
    if (compression_level != 0 && compression_level != 1 && compression_level != 3) {
        compression_level = 0; // PCM jako domyślny
    }
    wav64_init_compression(compression_level);

    /* [8] Otwiera plik WAV64 i konfiguruje audio */
    const char *filename = "rom:/sound.wav64";
    wav64_t sound;
    wav64_open(&sound, filename);
    audio_init((int)sound.wave.frequency, 4);
    mixer_init(32);
    wav64_set_loop(&sound, true);
    wav64_play(&sound, 0);
    mixer_ch_set_vol(0, 1.0f, 1.0f);

    /* [9] Pobiera parametry audio */
    uint32_t sample_rate = (uint32_t)(sound.wave.frequency + 0.5f);
    uint8_t channels = sound.wave.channels;
    int total_samples = sound.wave.len;
    uint8_t bits_per_sample = sound.wave.bits;

    uint32_t total_seconds = (sample_rate > 0 && total_samples > 0)
                             ? total_samples / sample_rate : 0;
    uint32_t total_minutes = total_seconds / 60;
    uint32_t total_secs_rem = total_seconds % 60;

    int bitrate_bps = wav64_get_bitrate(&sound);
    uint32_t bitrate_kbps = (bitrate_bps > 0)
                            ? (bitrate_bps / 1000)
                            : (sample_rate * channels * bits_per_sample) / 1000;

    /* [10] Zapisuje czas startu odtwarzania */
    uint32_t start_ticks = timer_ticks();

    /* [11] Pętla główna programu */
    while (1) {
        int max_amp = 0, max_amp_l = 0, max_amp_r = 0;

        /* [12] Przetwarza bufor audio i oblicza amplitudy */
        if (audio_can_write()) {
            short *outbuf = audio_write_begin();
            mixer_poll(outbuf, audio_get_buffer_length());
            int samples = audio_get_buffer_length();
            if (channels == 2) {
                for (int i = 0; i < samples; i += 2) {
                    int amp_l = abs(outbuf[i]);
                    if (amp_l > max_amp_l) max_amp_l = amp_l;
                    int amp_r = abs(outbuf[i+1]);
                    if (amp_r > max_amp_r) max_amp_r = amp_r;
                }
            } else {
                for (int i = 0; i < samples; i++) {
                    int amp = abs(outbuf[i]);
                    if (amp > max_amp) max_amp = amp;
                }
            }
            audio_write_end();
        }

        /* [13] Oblicza czas odtwarzania */
        uint32_t elapsed_ms = TICKS_TO_MS(timer_ticks() - start_ticks);
        uint32_t elapsed_sec = elapsed_ms / 1000;
        uint32_t play_min = elapsed_sec / 60;
        uint32_t play_sec = elapsed_sec % 60;

        /* [14] Czyści ekran i ustawia kolor rysowania */
        surface_t *disp = display_get();
        graphics_fill_screen(disp, graphics_make_color(0, 0, 64, 255));
        graphics_set_color(graphics_make_color(255, 255, 255, 255), 0);

        char linebuf[80];

        /* [15] Rysuje tytuł programu */
        const char *title = "mca64Player";
        int title_x = (320 - custom_strlen(title) * 8) / 2;
        graphics_draw_text(disp, title_x, 4, title);

        /* [16] Wyświetla dane o pliku i czasie */
        snprintf(linebuf, sizeof(linebuf), "Plik: %s", filename);
        graphics_draw_text(disp, 10, 28, linebuf);
        snprintf(linebuf, sizeof(linebuf), "Czas: %02lu:%02lu / %02lu:%02lu",
                 (unsigned long)play_min, (unsigned long)play_sec,
                 (unsigned long)total_minutes, (unsigned long)total_secs_rem);
        graphics_draw_text(disp, 10, 46, linebuf);

        /* [17] Rysuje pasek postępu odtwarzania z ramką */
        int bar_x = 10, bar_y = 58, bar_w = 300, bar_h = 8;
        graphics_draw_box(disp, bar_x, bar_y, bar_w, bar_h,
                          graphics_make_color(255, 255, 255, 255));
        if (total_seconds > 0) {
            int pw = (int)((float)elapsed_sec / total_seconds * bar_w);
            graphics_draw_box(disp, bar_x, bar_y, pw, bar_h,
                              graphics_make_color(0, 255, 0, 255));
        }

        /* [18] Wyświetla parametry techniczne audio */
        snprintf(linebuf, sizeof(linebuf), "Probkowanie: %lu Hz", (unsigned long)sample_rate);
        graphics_draw_text(disp, 10, 76, linebuf);
        snprintf(linebuf, sizeof(linebuf), "Bitrate: %lu kbps", (unsigned long)bitrate_kbps);
        graphics_draw_text(disp, 10, 94, linebuf);
        snprintf(linebuf, sizeof(linebuf), "Kanaly: %s (%u)",
                 (channels == 1 ? "Mono" : "Stereo"), (unsigned)channels);
        graphics_draw_text(disp, 10, 112, linebuf);
        snprintf(linebuf, sizeof(linebuf), "Bity: %u bit", (unsigned)bits_per_sample);
        graphics_draw_text(disp, 10, 130, linebuf);
        snprintf(linebuf, sizeof(linebuf), "Kompresja (z lib): %s",
                 get_compression_name(compression_level));
        graphics_draw_text(disp, 10, 148, linebuf);
        snprintf(linebuf, sizeof(linebuf), "Kompresja (manual): %s",
                 get_compression_name(manual_compression_level));
        graphics_draw_text(disp, 10, 166, linebuf);
        snprintf(linebuf, sizeof(linebuf), "Probek: %d", total_samples);
        graphics_draw_text(disp, 10, 184, linebuf);

        /* [19] Rysuje pionowe VU meter z ramką */
        int vu_base_x = 280;
        int vu_base_y = 200;
        int vu_width  = 8;
        int vu_height = 40;

        if (channels == 2) {
            // Lewy
            graphics_draw_box(disp, vu_base_x-1, vu_base_y - vu_height - 1, vu_width+2, vu_height+2,
                              graphics_make_color(255,255,255,255));
            int h_l = (max_amp_l * vu_height) / 32768;
            if (h_l > vu_height) h_l = vu_height;
            graphics_draw_box(disp, vu_base_x, vu_base_y - h_l, vu_width, h_l,
                              graphics_make_color(0, 255, 0, 255));
            graphics_draw_text(disp, vu_base_x + (vu_width/2) - 4, vu_base_y + 4, "L");

            // Prawy
            graphics_draw_box(disp, vu_base_x + vu_width + 10 - 1, vu_base_y - vu_height - 1, vu_width+2, vu_height+2,
                              graphics_make_color(255,255,255,255));
            int h_r = (max_amp_r * vu_height) / 32768;
            if (h_r > vu_height) h_r = vu_height;
            graphics_draw_box(disp, vu_base_x + vu_width + 10, vu_base_y - h_r, vu_width, h_r,
                              graphics_make_color(0, 255, 0, 255));
            graphics_draw_text(disp, vu_base_x + vu_width + 10 + (vu_width/2) - 4, vu_base_y + 4, "R");
        } else {
            // Mono
            graphics_draw_box(disp, vu_base_x + 8 - 1, vu_base_y - vu_height - 1, vu_width+2, vu_height+2,
                              graphics_make_color(255,255,255,255));
            int h_m = (max_amp * vu_height) / 32768;
            if (h_m > vu_height) h_m = vu_height;
            graphics_draw_box(disp, vu_base_x + 8, vu_base_y - h_m, vu_width, h_m,
                              graphics_make_color(0, 255, 0, 255));
            graphics_draw_text(disp, vu_base_x + 8 + (vu_width/2) - 12, vu_base_y + 4, "Mono");
        }

        /* [20] Wyświetla hex nagłówka w 2 liniach */
        char hex_line1[40] = {0};
        char hex_line2[40] = {0};
        int hex_len = custom_strlen(header_hex_string);
        int split_pos = hex_len / 2;
        for (int i = 0; i < split_pos; i++)
            hex_line1[i] = header_hex_string[i];
        hex_line1[split_pos] = '\0';
        int idx = 0;
        for (int i = split_pos; i < hex_len; i++)
            hex_line2[idx++] = header_hex_string[i];
        hex_line2[idx] = '\0';
        graphics_draw_text(disp, 10, 202, hex_line1);
        graphics_draw_text(disp, 10, 212, hex_line2);

        /* [21] Wyświetla dane debug DFS */
        char buf[80];
        snprintf(buf, sizeof(buf), "DFS FD: %d, Read: %d", dfs_fd, dfs_bytes_read);
        graphics_draw_text(disp, 10, 230, buf);

        /* [22] Wyświetla ekran */
        display_show(disp);
        wait_ms(100);
    }

    /* [23] Zamyka zasoby audio */
    wav64_close(&sound);
    audio_close();
    return 0;
}
