#ifndef MENU_H
#define MENU_H

#include <libdragon.h>
#include <stdbool.h>
#include "resolutions.h"

/* [1] MENU CONFIGURATION - Dimensions, padding, repeat timings */
#define MENU_BOX_W_DEFAULT   460
#define MENU_BOX_H_DEFAULT   200
#define MENU_PAD_X           8
#define MENU_PAD_Y           8
#define MENU_TITLE_H         14
#define MENU_HINT_H          14
#define MENU_LINE_H          12
#define KEYREP_INITIAL_FRAMES 10
#define KEYREP_REPEAT_FRAMES  4

/* [2] STRUCTURES AND ENUMS */
typedef struct {
    const char *name;               /* Option name in the menu */
    const resolution_t *res;        /* Pointer to resolution struct from resolutions.h */
} resolution_entry_t;

typedef enum {
    MENU_STATUS_OPEN = 0,       /* Menu is active */
    MENU_STATUS_SELECTED = 1,   /* Option selected */
    MENU_STATUS_CANCEL = 2      /* Menu cancelled */
} menu_status_t;

/* [3] MENU API */
void menu_open(void);
void menu_close(void);
bool menu_is_open(void);
void menu_set_initial_resolution(const resolution_t *r);
menu_status_t menu_update(surface_t *disp, joypad_buttons_t pressed,
                          joypad_buttons_t held, const resolution_t **out_selected);

#endif /* MENU_H */
