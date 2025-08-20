/* [1] debug.c - Diagnostics and debug info rendering for N64 display. */
#include "debug.h"     /* [2] Own header for debug_info */
#include "utils.h"     /* [3] Helper functions: formatting, safe string copy */

/* [4] Draw diagnostic information (audio, performance, memory, uptime) on screen. */
void debug_info(surface_t *disp, int sample_rate, float frame_ms, float cpu_percent,
                float fps, int free_ram, int start_x, int start_y, unsigned int uptime_sec, double ram_total_mb, const wav64_t* wav,
                const char* wav64_header_hex) {
    int can_write = audio_can_write();
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

    /* [3] Audio can write */
    strcpy_s(tmp, sizeof(tmp), "Audio can write: ");
    safe_append_str(tmp, sizeof(tmp), -1, can_write ? "YES" : "NO");
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

    /* [5] Sample rate */
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "Sample rate: ");
    pos = tiny_strlen(tmp);
    pos += int_to_dec(&tmp[pos], sample_rate);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " Hz");
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
        strcpy_s(tmp, sizeof(tmp), "WAV64: ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], (int)wav->wave.frequency);
        strcpy_s(&tmp[pos], sizeof(tmp) - pos, " Hz, ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], (int)wav->wave.len);
        strcpy_s(&tmp[pos], sizeof(tmp) - pos, " samples, ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], (int)wav->wave.channels);
        strcpy_s(&tmp[pos], sizeof(tmp) - pos, " ch, ");
        pos = tiny_strlen(tmp);
        pos += int_to_dec(&tmp[pos], (int)wav->wave.bits);
        strcpy_s(&tmp[pos], sizeof(tmp) - pos, " bit");
        graphics_draw_text(disp, start_x, y, tmp);
    }
    /* [11] WAV64 header hex */
    if (wav64_header_hex && wav64_header_hex[0]) {
        y += line_height;
        strcpy_s(tmp, sizeof(tmp), "WAV64 header: ");
        safe_append_str(tmp, sizeof(tmp), -1, wav64_header_hex);
        graphics_draw_text(disp, start_x, y, tmp);
    }
}