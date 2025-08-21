/* [1] debug.c - Diagnostics and debug info rendering for N64 display. */
#include "debug.h"     /* [2] Own header for debug_info */
#include "utils.h"     /* [3] Helper functions: formatting, safe string copy */

/* [4] Draw diagnostic information (audio, performance, memory, uptime) on screen. */
void debug_info(surface_t *disp, int sample_rate, float frame_ms, float cpu_percent,
                float fps, int free_ram, int start_x, int start_y, unsigned int uptime_sec, double ram_total_mb, const wav64_t* wav,
                const char* wav64_header_hex, uint8_t compression_level) {
    int buf_len = audio_get_buffer_length();
    float buf_ms = (float)buf_len / (float)sample_rate * 1000.0f;
    graphics_set_color(graphics_make_color(255, 255, 0, 255), 0);
    char tmp[128];
    int line_height = 15;
    int y = start_y;
    int pos = 0;

    /* [1] Total RAM (MB) */
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "Total RAM: ");
    pos = tiny_strlen(tmp);
    pos += format_float_two_decimals(&tmp[pos], ram_total_mb);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " MB");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [2] Free RAM (bytes) */
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "Free RAM: ");
    pos = tiny_strlen(tmp);
    pos += int_to_dec(&tmp[pos], free_ram);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " bytes");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [3] Resolution info (moved from main.c) */
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "Resolution: ");
    pos = tiny_strlen(tmp);
    // Zak³adamy, ¿e szerokoœæ i wysokoœæ ekranu mo¿na pobraæ przez display_get_width/height
    pos += int_to_dec(&tmp[pos], display_get_width());
    tmp[pos++] = 'x';
    pos += int_to_dec(&tmp[pos], display_get_height());
    tmp[pos++] = 'p'; // lub 'i' jeœli interlaced, tu uproszczenie
    tmp[pos] = '\0';
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [4] Audio buffer: samples (ms) */
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "Audio buffer: ");
    pos = tiny_strlen(tmp);
    pos += int_to_dec(&tmp[pos], buf_len);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " samples (");
    pos = tiny_strlen(tmp);
    pos += format_float_two_decimals(&tmp[pos], buf_ms);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " ms)");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [6] Frame time */
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "Frame time: ");
    pos = tiny_strlen(tmp);
    pos += format_float_two_decimals(&tmp[pos], frame_ms);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " ms");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [7] CPU usage */
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "CPU usage: ");
    pos = tiny_strlen(tmp);
    pos += format_float_two_decimals(&tmp[pos], cpu_percent);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " %");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [8] FPS */
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "FPS: ");
    pos = tiny_strlen(tmp);
    pos += format_float_two_decimals(&tmp[pos], fps);
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [9] Uptime */
    unsigned int hours = uptime_sec / 3600;
    unsigned int minutes = (uptime_sec % 3600) / 60;
    unsigned int seconds = uptime_sec % 60;
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "Uptime: ");
    pos = tiny_strlen(tmp);
    pos += append_uint_zero_pad(&tmp[pos], hours, 2);
    tmp[pos++] = ':';
    pos += append_uint_zero_pad(&tmp[pos], minutes, 2);
    tmp[pos++] = ':';
    pos += append_uint_zero_pad(&tmp[pos], seconds, 2);
    tmp[pos] = '\0';
    graphics_draw_text(disp, start_x, y, tmp);

    /* [10] WAV64 info */
    if (wav) {
        y += line_height;
        pos = 0;
        strcpy_s(tmp, sizeof(tmp), "WAV64 frequency: ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], (int)wav->wave.frequency);
        strcpy_s(&tmp[pos], sizeof(tmp) - pos, " Hz");
        graphics_draw_text(disp, start_x, y, tmp);
        y += line_height;
        pos = 0;
        strcpy_s(tmp, sizeof(tmp), "WAV64 samples: ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], (int)wav->wave.len);
        graphics_draw_text(disp, start_x, y, tmp);
        y += line_height;
        pos = 0;
        strcpy_s(tmp, sizeof(tmp), "WAV64 channels: ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], (int)wav->wave.channels);
        if (wav->wave.channels == 1)
            strcpy_s(&tmp[pos], sizeof(tmp) - pos, " (Mono)");
        else if (wav->wave.channels == 2)
            strcpy_s(&tmp[pos], sizeof(tmp) - pos, " (Stereo)");
        graphics_draw_text(disp, start_x, y, tmp);
        y += line_height;
        pos = 0;
        strcpy_s(tmp, sizeof(tmp), "WAV64 bits: ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], (int)wav->wave.bits);
        graphics_draw_text(disp, start_x, y, tmp);
        y += line_height;
        // Bitrate
        int bitrate_bps = wav64_get_bitrate((wav64_t*)wav);
        int bitrate_kbps = (bitrate_bps > 0) ? (bitrate_bps / 1000) : ((wav->wave.frequency * wav->wave.channels * wav->wave.bits) / 1000);
        pos = 0;
        strcpy_s(tmp, sizeof(tmp), "WAV64 bitrate: ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], bitrate_kbps);
        strcpy_s(&tmp[pos], sizeof(tmp) - pos, " kbps");
        graphics_draw_text(disp, start_x, y, tmp);
        y += line_height;
        // Compression (przeniesione z main.c, teraz przez argument compression_level)
        pos = 0;
        strcpy_s(tmp, sizeof(tmp), "WAV64 compression: ");
        pos = tiny_strlen(tmp);
        if (compression_level == 0)
            strcpy_s(&tmp[pos], sizeof(tmp) - pos, "PCM (0)");
        else if (compression_level == 1)
            strcpy_s(&tmp[pos], sizeof(tmp) - pos, "VADPCM (1)");
        else if (compression_level == 3)
            strcpy_s(&tmp[pos], sizeof(tmp) - pos, "Opus (3)");
        else
            int_to_dec(&tmp[pos], (int)compression_level);
        graphics_draw_text(disp, start_x, y, tmp);
    }
    /* [11] WAV64 header hex (split into two lines) */
    if (wav64_header_hex && wav64_header_hex[0]) {
        y += line_height;
        int hex_len = tiny_strlen(wav64_header_hex);
        int groups = hex_len / 3;
        int split_groups = groups / 2;
        int split_pos = split_groups * 3;
        int i = 0, idx = 0;
        char hex_line1[40], hex_line2[40];
        for (i = 0; i < split_pos && i < (int)sizeof(hex_line1) - 1; i++) hex_line1[i] = wav64_header_hex[i];
        hex_line1[i] = '\0';
        idx = 0;
        for (i = split_pos; i < hex_len && idx < (int)sizeof(hex_line2) - 1; i++) hex_line2[idx++] = wav64_header_hex[i];
        hex_line2[idx] = '\0';
        strcpy_s(tmp, sizeof(tmp), "WAV64 header: ");
        safe_append_str(tmp, sizeof(tmp), -1, hex_line1);
        graphics_draw_text(disp, start_x, y, tmp);
        y += line_height;
        // Wyznacz offset do danych (po "WAV64 header: ")
        int label_len = tiny_strlen("WAV64 header: ");
        int x_offset = start_x + label_len * 8; // 8 px na znak
        graphics_draw_text(disp, x_offset, y, hex_line2);
    }
}