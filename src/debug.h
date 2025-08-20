#ifndef DEBUG_H
#define DEBUG_H
#include <libdragon.h>

/* [1] Draw diagnostic information (audio, performance, memory, uptime) on screen. */
void debug_info(surface_t *disp, int sample_rate, float frame_ms, float cpu_percent,
                float fps, int free_ram, int start_x, int start_y, unsigned int uptime_sec, double ram_total_mb, const wav64_t* wav,
                const char* wav64_header_hex) ;

#endif /* DEBUG_H */