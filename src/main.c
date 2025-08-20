/* [1] main.c - Main entry point for the mca64Player application. Detailed, step-by-step comments for C beginners. */
#include <stdio.h>      /* [2] Standard C I/O functions (for file operations) */
#include <stdint.h>     /* [3] Standard integer types (uint32_t, etc.) */
#include <stdbool.h>    /* [4] Boolean type support */
#include <stdlib.h>     /* [5] abs, malloc/free if needed */
#include <math.h>       /* [6] powf, fabsf for math operations */
#include <malloc.h>     /* [7] mallinfo() for memory usage */
#include <system.h>     /* [8] System-specific functions (libdragon) */
#include <libdragon.h>  /* [9] N64 SDK library */
#include "menu.h"      /* [10] Menu system header */
#include "utils.h"     /* [11] Utility functions header */
#include "debug.h"     /* [12] Debug info rendering header */
#include "arena.h"     /* [13] Simple memory arena header */
#include "cpu_usage.h" /* [14] CPU usage averaging header */
#include "vu.h"        /* [15] VU meter logic header */
#include "hud.h"       /* [16] HUD/message display header */

/* [17] Application-wide constants */
#define SCREEN_W 640     /* Default screen width */
#define SCREEN_H 288     /* Default screen height */
#define ANALOG_DEADZONE 8 /* Deadzone for analog stick */
#define FRAME_MS_ALPHA 0.02f /* Smoothing factor for frame time */
#define VU_HALF_LIFE_MS 800.0f /* VU meter smoothing half-life */
#define FRAME_MS_DISPLAY_THRESHOLD 0.15f /* Frame time smoothing threshold */
#define VU_MIN_DELTA 2.0f /* Minimum VU meter update delta */

/* [18] Global state variables */
static uint32_t last_frame_ticks = 0;   /* Last frame tick count */
static uint32_t fps = 0;                /* Raw FPS value */
static float volume = 1.0f;             /* Current audio volume (0.0 - 1.0) */
static float last_frame_start_ms = 0.0f;/* Last frame start time (ms) */
static float last_cpu_ms_display = 0.0f;/* Smoothed frame time (ms) */
static float smoothed_frame_ms = 0.0f;  /* Exponential moving average of frame time */
static float smoothed_fps = 0.0f;       /* Exponential moving average of FPS */
static const resolution_t *current_resolution = NULL; /* Current selected resolution */

/* [19] Main program entry point */
int main(void) {
    /* [20] --- Initialization section --- */
    const int SOUND_CH = 0; /* Audio channel index */
    static const resolution_t PAL = {SCREEN_W, SCREEN_H, false}; /* Default PAL resolution */
    display_init(PAL, DEPTH_32_BPP, 2, GAMMA_NONE, ANTIALIAS_OFF); /* Initialize display */
    joypad_init(); /* Initialize joypad input */
    current_resolution = &PAL; /* Set current resolution pointer */

    /* [21] Initialize file system (DFS) and handle error if it fails */
    if (dfs_init(DFS_DEFAULT_LOCATION) != DFS_ESUCCESS) {
        surface_t *disp = display_get();
        uint32_t black = graphics_make_color(0, 0, 0, 255);
        uint32_t red = graphics_make_color(255, 0, 0, 255);
        graphics_fill_screen(disp, black);
        graphics_set_color(red, 0);
        graphics_draw_text(disp, 10, 10, "Error: cannot initialize DFS");
        display_show(disp);
        return 1; /* Exit with error */
    }

    /* [22] Read WAV64 file header and extract compression info */
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
        strcpy_s(header_hex_string, (int)sizeof(header_hex_string), "Read error");
    }

    /* [23] Initialize timer and frame tick counter */
    timer_init();
    last_frame_ticks = timer_ticks();

    /* [24] Set up audio compression type */
    int compression_level = (int)manual_compression_level;
    if (compression_level != 0 && compression_level != 1 && compression_level != 3) compression_level = 0;
    wav64_init_compression(compression_level);

    /* [25] Open WAV64 file and initialize audio/mixer */
    const char *filename = "rom:/sound.wav64";
    wav64_t sound;
    fast_memset(&sound, 0, sizeof(sound));
    wav64_open(&sound, filename);
    audio_init((int)sound.wave.frequency, 4);
    mixer_init(32);
    wav64_set_loop(&sound, true);

    /* [26] Playback state variables */
    uint32_t current_sample_pos = 0;
    bool is_playing = false;
    int sound_channel = SOUND_CH;
    current_sample_pos = 0;
    wav64_play(&sound, sound_channel);
    mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
    is_playing = true;
    mixer_ch_set_vol(sound_channel, volume, volume);

    /* [27] Audio file parameters (cache for display) */
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

    /* [28] Allocate temporary buffers from arena for UI and state */
    char *last_button_pressed = (char *)arena_alloc(32);
    if (last_button_pressed) strcpy_s(last_button_pressed, 32, "None");
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
    /* [28.1] Load logo sprite (must be converted to .sprite and placed in romfs/logo.sprite) */
    sprite_t* logo = sprite_load("rom:/logo.sprite");
    if (logo) {
        menu_set_logo_sprite(logo);
    }
    /* [29] Variables for VU meter (audio peak levels) */
    int max_amp = 0, max_amp_l = 0, max_amp_r = 0;

    /* [30] --- Main application loop --- */
    while (1) {
        double ram_total = get_memory_size() / (1024.0 * 1024.0);
        struct mallinfo mi = mallinfo();
        double used_malloc = (double)mi.uordblks;
        double used_arena = (double)arena_get_used();
        double used_total_mb = (used_malloc + used_arena) / (1024.0 * 1024.0);
        double ram_free = ram_total - used_total_mb;
        if (ram_free < 0.0) ram_free = 0.0;

        /* [31] Start of frame: measure time */
        float frame_start_ms = get_ticks_ms();
        float frame_interval_ms = (last_frame_start_ms > 0.0f) ? (frame_start_ms - last_frame_start_ms) : (1000.0f / 60.0f);
        max_amp = max_amp_l = max_amp_r = 0;

        /* [32] Poll joypad and update last button/analog state */
        joypad_poll();
        joypad_inputs_t inputs = joypad_get_inputs(JOYPAD_PORT_1);
        update_last_button_pressed(last_button_pressed, 32, inputs);
        int ax = inputs.stick_x;
        int ay = inputs.stick_y;
        if (abs(ax) <= ANALOG_DEADZONE) ax = 0;
        if (abs(ay) <= ANALOG_DEADZONE) ay = 0;
        format_analog(analog_pos, ax, ay);

        /* [33] Get pressed and held buttons */
        joypad_buttons_t pressed = joypad_get_buttons_pressed(JOYPAD_PORT_1);
        joypad_buttons_t held = joypad_get_buttons_held(JOYPAD_PORT_1);

        /* [34] Audio mixing and peak analysis */
        while (audio_can_write()) {
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

        /* [35] Auto-restart or end of playback */
        if (!mixer_ch_playing(sound_channel)) {
            if (loop_enabled) {
                wav64_play(&sound, sound_channel);
                mixer_ch_set_pos(sound_channel, 0.0f);
                mixer_ch_set_vol(sound_channel, volume, volume);
                is_playing = true;
                current_sample_pos = 0;
                show_message("Restart (loop)");
            } else {
                if (is_playing) {
                    is_playing = false;
                    current_sample_pos = total_samples;
                    show_message("End");
                }
            }
        }

        /* [36] Calculate current playback position for display */
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

        /* [37] --- UI Drawing section --- */
        surface_t *disp = display_get();
        graphics_fill_screen(disp, bg_color);
        graphics_set_color(white, 0);
        const char *title = "mca64Player";
        int title_x = ((int)display_get_width() - tiny_strlen(title) * 8) / 2;
        graphics_draw_sprite(disp, 0, 0, logo);
        graphics_draw_text(disp, title_x, 4, title);
        graphics_draw_text(disp, 10, 4, last_button_pressed);
        graphics_draw_text(disp, 10, 16, analog_pos);

        /* [38] Draw file name */
        strcpy_s(linebuf, 128, "File: ");
        int off = tiny_strlen(linebuf);
        int remaining = (int)128 - off;
        strcpy_s(&linebuf[off], remaining, filename);
        graphics_draw_text(disp, 10, 28, linebuf);

        /* [39] Draw playback time (current/total) */
        format_time_line(linebuf, (unsigned)play_min, (unsigned)play_sec,
                         (unsigned)total_minutes, (unsigned)total_secs_rem);
        graphics_draw_text(disp, 10, 46, linebuf);

        /* [40] Draw progress bar */
        int bar_x = 10, bar_y = 58, bar_w = 300, bar_h = 8;
        graphics_draw_box(disp, bar_x, bar_y, bar_w, bar_h, white);
        if (total_seconds > 0) {
            int pw = (int)(((uint64_t)elapsed_sec * (uint64_t)bar_w) / (uint64_t)total_seconds);
            if (pw > bar_w) pw = bar_w;
            graphics_draw_box(disp, bar_x, bar_y, pw, bar_h, green);
        }

        /* [41] Draw audio parameters (sample rate, bitrate, channels, bits) */
        strcpy_s(linebuf, 128, "Sample rate: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)sample_rate);
        strcpy_s(&linebuf[off], 128 - off, " Hz");
        graphics_draw_text(disp, 10, 76, linebuf);
        strcpy_s(linebuf, 128, "Bitrate: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)bitrate_kbps);
        strcpy_s(&linebuf[off], 128 - off, " kbps");
        graphics_draw_text(disp, 10, 94, linebuf);
        if (channels == 1) strcpy_s(linebuf, 128, "Channels: Mono (1)");
        else strcpy_s(linebuf, 128, "Channels: Stereo (2)");
        graphics_draw_text(disp, 10, 112, linebuf);
        strcpy_s(linebuf, 128, "Bits: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)bits_per_sample);
        strcpy_s(&linebuf[off], 128 - off, " bit");
        graphics_draw_text(disp, 10, 130, linebuf);

        /* [42] Draw resolution info */
        if (current_resolution) {
            strcpy_s(linebuf, 128, "Resolution: ");
            off = tiny_strlen(linebuf);
            off += int_to_dec(&linebuf[off], (int)current_resolution->width);
            linebuf[off++] = 'x';
            off += int_to_dec(&linebuf[off], (int)current_resolution->height);
            linebuf[off++] = current_resolution->interlaced ? 'i' : 'p';
            linebuf[off] = '\0';
        } else {
            strcpy_s(linebuf, 128, "Resolution: ");
            off = tiny_strlen(linebuf);
            off += int_to_dec(&linebuf[off], display_get_width());
            linebuf[off++] = 'x';
            off += int_to_dec(&linebuf[off], display_get_height());
            linebuf[off] = '\0';
        }
        graphics_draw_text(disp, 10, 148, linebuf);

        /* [43] Draw compression info */
        strcpy_s(linebuf, 128, "Compression: ");
        off = tiny_strlen(linebuf);
        if (manual_compression_level == 0) strcpy_s(&linebuf[off], 128 - off, "PCM");
        else if (manual_compression_level == 1) strcpy_s(&linebuf[off], 128 - off, "VADPCM");
        else if (manual_compression_level == 3) strcpy_s(&linebuf[off], 128 - off, "Opus");
        else { off += int_to_dec(&linebuf[off], (int)manual_compression_level); }
        graphics_draw_text(disp, 10, 166, linebuf);

        /* [44] Draw total sample count */
        strcpy_s(linebuf, 128, "Samples: ");
        off = tiny_strlen(linebuf);
        off += int_to_dec(&linebuf[off], (int)total_samples);
        graphics_draw_text(disp, 10, 184, linebuf);

        /* [45] Draw VU meters (audio levels) */
        vu_update(frame_interval_ms, max_amp_l, max_amp_r);
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

        /* [46] Draw WAV64 header hex dump (split into two lines) */
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

        /* [47] Update FPS and CPU usage meters */
        uint32_t now_ticks = timer_ticks();
        uint32_t diff_ticks = now_ticks - last_frame_ticks;
        if (last_frame_ticks != 0 && diff_ticks > 0) {
            fps = TICKS_PER_SECOND / diff_ticks;
        }
        last_frame_ticks = now_ticks;

        if (smoothed_fps <= 0.0f) smoothed_fps = (float)fps;
        else smoothed_fps = (1.0f - FRAME_MS_ALPHA) * smoothed_fps + FRAME_MS_ALPHA * (float)fps;
        strcpy_s(perfbuf, 128, "FPS: ");
        off = tiny_strlen(perfbuf);
        off += int_to_dec(&perfbuf[off], (int)(smoothed_fps + 0.5f));
        graphics_draw_text(disp, 10, 230, perfbuf);

        /* [48] Draw RAM usage and free memory */
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

        /* [49] Handle menu opening (START button) */
        if (pressed.start && !menu_is_open()) {
            menu_set_initial_resolution(current_resolution);
            menu_open();
            pressed.start = 0;
        }
        /* [50] Handle menu navigation and selection */
        if (menu_is_open()) {
            const resolution_t *sel = NULL;
            menu_status_t st = menu_update(disp, pressed, held, &sel);
            display_show(disp);
            if (st == MENU_STATUS_SELECTED && sel) {
                current_resolution = sel;
                display_close();
                display_init(*sel, DEPTH_32_BPP, 2, GAMMA_NONE, ANTIALIAS_OFF);
                menu_close();
                char buf[64];
                strcpy_s(buf, 64, "Selected: ");
                int pos = tiny_strlen(buf);
                pos += int_to_dec(&buf[pos], (int)sel->width);
                pos = safe_append_str(buf, 64, pos, " x ");
                pos += int_to_dec(&buf[pos], (int)sel->height);
                pos = safe_append_str(buf, 64, pos, " ");
                pos = safe_append_str(buf, 64, pos, sel->interlaced ? "i" : "p");
                show_message(buf);
            } else if (st == MENU_STATUS_CANCEL) {
                show_message("Cancelled");
                menu_close();
            }
            continue;
        }

        /* [51] Handle playback controls (A, B, L, R, C, D, Z buttons) */
        if (pressed.a) {
            if (is_playing && mixer_ch_playing(sound_channel)) {
                float pos_f = mixer_ch_get_pos(sound_channel);
                if (pos_f < 0.0f) pos_f = 0.0f;
                uint64_t pos_u = (uint64_t)(pos_f + 0.5f);
                if (pos_u > (uint64_t)total_samples) pos_u = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)pos_u;
                mixer_ch_stop(sound_channel);
                is_playing = false;
                show_message("Pause");
            } else {
                if (compression_level != 0 && current_sample_pos != 0) {
                    current_sample_pos = 0;
                    show_message("Unavailable - start from beginning");
                }
                wav64_play(&sound, sound_channel);
                mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                mixer_ch_set_vol(sound_channel, volume, volume);
                is_playing = true;
                show_message("Resumed");
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
                show_message("Rewind -5s");
            } else show_message("Unavailable for this format");
        }
        if (pressed.r || pressed.d_right) {
            if (compression_level == 0) {
                uint64_t delta_samples = (uint64_t)5 * (uint64_t)sample_rate;
                uint64_t newpos = (uint64_t)current_sample_pos_display + delta_samples;
                if (newpos > (uint64_t)total_samples) newpos = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Forward +5s");
            } else show_message("Unavailable for this format");
        }
        if (pressed.c_left) {
            if (compression_level == 0) {
                int64_t delta_samples = (int64_t)30 * (int64_t)sample_rate;
                int64_t newpos = (int64_t)current_sample_pos_display - delta_samples;
                if (newpos < 0) newpos = 0;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Rewind -30s");
            } else show_message("Unavailable for this format");
        }
        if (pressed.c_right) {
            if (compression_level == 0) {
                uint64_t delta_samples = (uint64_t)30 * (uint64_t)sample_rate;
                uint64_t newpos = (uint64_t)current_sample_pos_display + delta_samples;
                if (newpos > (uint64_t)total_samples) newpos = (uint64_t)total_samples;
                current_sample_pos = (uint32_t)newpos;
                if (is_playing && mixer_ch_playing(sound_channel)) mixer_ch_set_pos(sound_channel, (float)current_sample_pos);
                show_message("Forward +30s");
            } else show_message("Unavailable for this format");
        }
        if (pressed.d_up || pressed.c_up) {
            volume += 0.1f;
            if (volume > 1.0f) volume = 1.0f;
            mixer_ch_set_vol(sound_channel, volume, volume);
            strcpy_s(tmpbuf, 64, "Volume: ");
            int len = tiny_strlen(tmpbuf);
            format_float_one_decimal(&tmpbuf[len], volume);
            show_message(tmpbuf);
        }
        if (pressed.d_down || pressed.c_down) {
            volume -= 0.1f;
            if (volume < 0.0f) volume = 0.0f;
            mixer_ch_set_vol(sound_channel, volume, volume);
            strcpy_s(tmpbuf, 64, "Volume: ");
            int len = tiny_strlen(tmpbuf);
            format_float_one_decimal(&tmpbuf[len], volume);
            show_message(tmpbuf);
        }
        if (pressed.z) {
            loop_enabled = !loop_enabled;
            wav64_set_loop(&sound, loop_enabled);
            if (loop_enabled) show_message("Loop: ON"); else show_message("Loop: OFF");
        }

        /* [52] Draw HUD message and debug info */
        hud_draw_message(disp, box_frame_color, box_bg_color);
        unsigned int uptime_sec = (unsigned int)(last_frame_start_ms / 1000.0f);
        debug_info(disp,
           (int)sample_rate,
           last_cpu_ms_display,
           cpu_usage_get_avg(),
           smoothed_fps,
           (int)(ram_free * 1024.0 * 1024.0),
           320, 12,
           uptime_sec,
           ram_total,
           &sound,
           header_hex_string);

        display_show(disp);

        /* [53] End of frame: measure and update CPU/frame stats */
        float frame_end_ms = get_ticks_ms();
        float measured_ms = frame_end_ms - frame_start_ms;
        if (last_frame_start_ms > 0.0f) frame_interval_ms = frame_start_ms - last_frame_start_ms;
        if (frame_interval_ms <= 0.0f) frame_interval_ms = (measured_ms > 0.0f) ? measured_ms : (1000.0f / 60.0f);
        float cpu_percent = (frame_interval_ms > 0.0f) ? ((measured_ms / frame_interval_ms) * 100.0f) : 0.0f;
        if (cpu_percent > 100.0f) cpu_percent = 100.0f;
        if (cpu_percent < 0.0f) cpu_percent = 0.0f;
        cpu_usage_add_sample(cpu_percent);
        if (smoothed_frame_ms <= 0.0f) smoothed_frame_ms = measured_ms;
        else {
            float new_smoothed = (1.0f - FRAME_MS_ALPHA) * smoothed_frame_ms + FRAME_MS_ALPHA * measured_ms;
            if (fabsf(new_smoothed - smoothed_frame_ms) > FRAME_MS_DISPLAY_THRESHOLD) smoothed_frame_ms = new_smoothed;
        }
        last_cpu_ms_display = smoothed_frame_ms;
        last_frame_start_ms = frame_start_ms;
        wait_ms(1); /* Short yield to avoid busy loop */
    }

    /* [54] Cleanup (not normally reached) */
    if (mixer_ch_playing(sound_channel)) mixer_ch_stop(sound_channel);
    wav64_close(&sound);
    audio_close();
    return 0;
}
