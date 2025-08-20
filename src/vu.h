#pragma once
#include <libdragon.h>

/* [1] Update internal, smoothed VU meter values.
   frame_interval_ms: time since last frame in ms
   max_amp_l / max_amp_r: peak values for this frame (short->abs) */
void vu_update(float frame_interval_ms, int max_amp_l, int max_amp_r);

/* [2] Get the current smoothed values as int (for drawing) */
int vu_get_left(void);
int vu_get_right(void);

/* [3] Draw a single VU meter (call from main, interface identical to original) */
void draw_vu_meter(display_context_t disp, int base_x, int base_y, int width, int height,
                   int value, int max_val, uint32_t box_color, uint32_t fill_color, const char *label);

/* [4] (Optional) Configure smoothing parameters: half-life in ms and minimum delta */
void vu_config(float half_life_ms, float min_delta);
