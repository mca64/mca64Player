#ifndef MENU_H
#define MENU_H

#include <libdragon.h>
#include <stdbool.h>
#include "resolutions.h"

/* ============================================================
 * (1) KONFIGURACJA STAŁA - WYMIARY, PADDING, CZASY POWTARZANIA
 * ============================================================ */
#define MENU_BOX_W_DEFAULT   460
#define MENU_BOX_H_DEFAULT   200
#define MENU_PAD_X           8
#define MENU_PAD_Y           8
#define MENU_TITLE_H         14
#define MENU_HINT_H          14
#define MENU_LINE_H          12

#define KEYREP_INITIAL_FRAMES 10
#define KEYREP_REPEAT_FRAMES  4

/* ============================================================
 * (2) STRUKTURY I ENUMY
 * ============================================================ */
typedef struct {
    const char *name;               // nazwa opcji w menu
    const resolution_t *res;        // wskaźnik na strukturę z resolutions.h
} resolution_entry_t;

typedef enum {
    MENU_STATUS_OPEN = 0,       // menu aktywne
    MENU_STATUS_SELECTED = 1,   // wybrano opcję
    MENU_STATUS_CANCEL = 2      // anulowano
} menu_status_t;

/* ============================================================
 * (3) API MENU
 * ============================================================ */
void menu_open(void);
void menu_close(void);
bool menu_is_open(void);
void menu_set_initial_resolution(const resolution_t *r);

menu_status_t menu_update(surface_t *disp, joypad_buttons_t pressed,
                          joypad_buttons_t held, const resolution_t **out_selected);

#endif /* MENU_H */
