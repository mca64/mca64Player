// [1] Nagłówki wymagane przez funkcję debug_info
#include "debug.h"           // [1.1] Własny interfejs funkcji debug_info
#include "utils.h"           // [1.2] Pomocnicze funkcje: formatowanie liczb, bezpieczne kopiowanie
// [2] Główna funkcja wyświetlająca dane diagnostyczne na ekranie
void debug_info(surface_t *disp, int sample_rate, float frame_ms, float cpu_percent,
                float fps, int free_ram, int start_x, int start_y)
{
    // [2.1] Pobranie stanu bufora audio
    int can_write = audio_can_write();             // [2.1.1] Czy można pisać do bufora audio (0 = pełny, !=0 = wolny)
    int buf_len = audio_get_buffer_length();       // [2.1.2] Liczba próbek w buforze
    float buf_ms = (float)buf_len / (float)sample_rate * 1000.0f; // [2.1.3] Czas trwania bufora w milisekundach

    // [2.2] Ustawienie koloru tekstu debugowego (żółty, pełna przezroczystość)
    graphics_set_color(graphics_make_color(255, 255, 0, 255), 0);

    // [2.3] Inicjalizacja zmiennych pomocniczych
    char tmp[128];                // [2.3.1] Bufor tekstowy do formatowania danych
    int line_height = 15;         // [2.3.2] Odległość między liniami tekstu
    int y = start_y;              // [2.3.3] Aktualna pozycja Y tekstu
    int pos = 0;                  // [2.3.4] Pozycja w buforze tmp

    // [3] Tymczasowe pole (można rozszerzyć np. o menu_selected)
    strcpy_s(tmp, sizeof(tmp), "");               // [3.1] Pusta linia lub miejsce na przyszłe dane
    graphics_draw_text(disp, start_x, y, tmp);    // [3.2] Rysowanie pustej linii
    y += line_height;

    // [4] Informacja o możliwości zapisu do bufora audio
    strcpy_s(tmp, sizeof(tmp), "Audio can write: ");                      // [4.1] Nagłówek
    safe_append_str(tmp, sizeof(tmp), -1, can_write ? "YES" : "NO");     // [4.2] Dodanie wartości logicznej
    graphics_draw_text(disp, start_x, y, tmp);                            // [4.3] Rysowanie tekstu
    y += line_height;

    // [5] Długość bufora audio w próbkach i milisekundach
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "");                                       // [5.1] Reset bufora
    pos += int_to_dec(&tmp[pos], buf_len);                                // [5.2] Liczba próbek
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " samples (");                 // [5.3] Tekst pomocniczy
    pos += tiny_strlen(&tmp[pos]);                                        // [5.4] Aktualizacja pozycji
    pos += format_float_two_decimals(&tmp[pos], buf_ms);                 // [5.5] Bufor w ms
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " ms)");                       // [5.6] Zamknięcie nawiasu
    graphics_draw_text(disp, start_x, y, tmp);                            // [5.7] Rysowanie tekstu
    y += line_height;

    // [6] Częstotliwość próbkowania audio
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "");
    pos += int_to_dec(&tmp[pos], sample_rate);                            // [6.1] Wartość Hz
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " Hz");                        // [6.2] Jednostka
    graphics_draw_text(disp, start_x, y, tmp);                            // [6.3] Rysowanie tekstu
    y += line_height;

    // [7] Czas renderowania jednej klatki
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "");
    pos += format_float_two_decimals(&tmp[pos], frame_ms);               // [7.1] Wartość w ms
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " ms");                        // [7.2] Jednostka
    graphics_draw_text(disp, start_x, y, tmp);                            // [7.3] Rysowanie tekstu
    y += line_height;

    // [8] Użycie CPU w procentach
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "");
    pos += format_float_two_decimals(&tmp[pos], cpu_percent);            // [8.1] Wartość %
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " %");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    // [9] Liczba klatek na sekundę (FPS)
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "");
    pos += format_float_two_decimals(&tmp[pos], fps);                    // [9.1] Wartość FPS
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " FPS");
    graphics_draw_text(disp, start_x, y, tmp);
    y += line_height;

    // [10] Ilość wolnej pamięci RAM
    pos = 0;
    strcpy_s(tmp, sizeof(tmp), "");
    pos += int_to_dec(&tmp[pos], free_ram);                              // [10.1] Wartość w bajtach
    strcpy_s(&tmp[pos], sizeof(tmp) - pos, " bytes free RAM");
    graphics_draw_text(disp, start_x, y, tmp);
}