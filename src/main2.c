#include <libdragon.h>

int main(void) {
    display_init(RESOLUTION_320x240, DEPTH_16_BPP, 2, GAMMA_NONE, FILTER_POINT);
    rdpq_init();
    joypad_init();
    debug_init(DEBUG_FEATURE_LOG_ISVIEWER);

    const int size = 16;
    const int x1 = 0,    y1 = 0;
    const int x2 = 120,  y2 = 100;

    while(1) {
        surface_t *disp;
        while (!(disp = display_try_get()));

        graphics_fill_screen(disp, 0xFFFFFF00); // żółte tło

        rdpq_attach(disp, NULL);

        /* --- Używamy RGBA16 (0xF800 = pełna czerwień) --- */
        rdpq_set_env_color(0xF800); 
        rdpq_fill_rectangle(x1, y1, x1 + size, y1 + size);
        rdpq_fill_rectangle(x2, y2, x2 + size, y2 + size);

        rdpq_detach_show();
    }

    return 0;
}
