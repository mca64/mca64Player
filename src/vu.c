#include "vu.h"
#include "utils.h"
#include <math.h>
#include <stdint.h>

/* Default smoothing half-life in ms */
static float VU_HALF_LIFE_MS = 800.0f;
/* Minimum change to update value */
static float VU_MIN_DELTA = 2.0f;

/* Smoothed value for left channel */
static float smoothed_vu_l = 0.0f;
/* Smoothed value for right channel */
static float smoothed_vu_r = 0.0f;

/* Configure smoothing parameters for the VU meter. */
void vu_config(float half_life_ms, float min_delta) {
    if (half_life_ms > 0.0f) VU_HALF_LIFE_MS = half_life_ms;
    VU_MIN_DELTA = min_delta;
}

/* Update smoothed VU values based on new peaks. */
void vu_update(float frame_interval_ms, int max_amp_l, int max_amp_r) {
    if (frame_interval_ms <= 0.0f) frame_interval_ms = 16.0f; /* fallback for invalid interval */

    float decay_factor = powf(0.5f, frame_interval_ms / VU_HALF_LIFE_MS);

    /* left channel */
    float peak_l = (float)max_amp_l;
    float cur_l = smoothed_vu_l * decay_factor;
    if (peak_l > cur_l) cur_l = peak_l;
    if (fabsf(cur_l - smoothed_vu_l) > VU_MIN_DELTA) smoothed_vu_l = cur_l;

    /* right channel */
    float peak_r = (float)max_amp_r;
    float cur_r = smoothed_vu_r * decay_factor;
    if (peak_r > cur_r) cur_r = peak_r;
    if (fabsf(cur_r - smoothed_vu_r) > VU_MIN_DELTA) smoothed_vu_r = cur_r;
}

/* Get the current smoothed left VU value. */
int vu_get_left(void) { return (int)smoothed_vu_l; }
/* Get the current smoothed right VU value. */
int vu_get_right(void) { return (int)smoothed_vu_r; }

/* Draw a single VU meter on the screen. */
void draw_vu_meter(display_context_t disp, int base_x, int base_y, int width, int height,
                   int value, int max_val, uint32_t box_color, uint32_t fill_color, const char *label) {
    if (!disp) return;
    if (max_val <= 0) max_val = 1;
    graphics_draw_box(disp, base_x - 1, base_y - height - 1, width + 2, height + 2, box_color);
    int h = (value * height) / max_val;
    if (h > height) h = height;
    graphics_draw_box(disp, base_x, base_y - h, width, h, fill_color);
    int tx = base_x + (width / 2) - ((int)tiny_strlen(label) * 4);
    graphics_draw_text(disp, tx, base_y + 4, label);
}


