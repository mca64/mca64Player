/* [1] debug.c - Diagnostics and debug info rendering for N64 display. */
#include "debug.h"     /* [2] Own header for debug_info */
#include "utils.h"     /* [3] Helper functions: formatting, safe string copy */

/* [4] Draw diagnostic information (audio, performance, memory, uptime) on screen. */
void debug_info(surface_t *disp, int sample_rate, float frame_ms, float cpu_percent,
                float fps, int free_ram, int start_x, int start_y, unsigned int uptime_sec) {
    int can_write = audio_can_write();
    int buf_len = audio_get_buffer_length();
    float buf_ms = (float)buf_len / (float)sample_rate * 1000.0f;
    graphics_set_color(graphics_make_color(255, 255, 0, 255), 0);
    char tmp[128];
    int line_height = 15;
    int y = start_y;
    int pos = 0;

    /* [5] Audio buffer status */
    strcpy_s(tmp, sizeof(tmp), "Audio can write: ");
    safe_append_str(tmp, sizeof(tmp), -1, can_write ? "YES" : "NO");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [6] Audio buffer length in samples and ms */
    pos = 0;
    pos += int_to_dec(&tmp[pos], buf_len);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " samples (");
    pos += tiny_strlen(&tmp[pos]);
    pos += format_float_two_decimals(&tmp[pos], buf_ms);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " ms)");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [7] Audio sample rate */
    pos = 0;
    pos += int_to_dec(&tmp[pos], sample_rate);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " Hz");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [8] Frame render time (ms) */
    pos = 0;
    pos += format_float_two_decimals(&tmp[pos], frame_ms);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " ms");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [9] CPU usage (%) */
    pos = 0;
    pos += format_float_two_decimals(&tmp[pos], cpu_percent);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " %");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [10] Frames per second (FPS) */
    pos = 0;
    pos += format_float_two_decimals(&tmp[pos], fps);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " FPS");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [11] Free RAM (bytes) */
    pos = 0;
    pos += int_to_dec(&tmp[pos], free_ram);
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " bytes free RAM");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    /* [12] Application uptime (hh:mm:ss) */
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
}