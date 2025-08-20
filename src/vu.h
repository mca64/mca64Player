#pragma once
#include <libdragon.h>

/* Aktualizuje wewnętrzne, wygładzone wartości miernika.
   frame_interval_ms: przebieg czasu od ostatniej klatki
   max_amp_l / max_amp_r: maksymalne wartości szczytowe odczytane w tej klatce (short->abs) */
void vu_update(float frame_interval_ms, int max_amp_l, int max_amp_r);

/* Pobiera już wygładzone wartości jako int (do rysowania) */
int vu_get_left(void);
int vu_get_right(void);

/* Rysowanie pojedynczego miernika (możesz wywołać z main, interfejs identyczny z oryginałem) */
void draw_vu_meter(display_context_t disp, int base_x, int base_y, int width, int height,
                   int value, int max_val, uint32_t box_color, uint32_t fill_color, const char *label);

/* (opcjonalnie) konfiguracja parametrow wygładzania: half-life w ms i minimalna delta */
void vu_config(float half_life_ms, float min_delta);
