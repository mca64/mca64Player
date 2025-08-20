#ifndef DEBUG_H
#define DEBUG_H

#include <libdragon.h> 

void debug_info(surface_t *disp, int sample_rate, float frame_ms, float cpu_percent,
                float fps, int free_ram, int start_x, int start_y);

#endif // DEBUG_H