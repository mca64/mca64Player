#include "menu.h"

/* ============================================================
 * (1) STAŁA LISTA ROZDZIELCZOŚCI
 * ============================================================ */
static const resolution_entry_t resolution_list[] = {
    { "PAL 640x288p",     &PAL_640x288p },
    { "NTSC 640x240p",    &NTSC_640x240p },
    { "PAL 320x288p",     &PAL_320x288p },
    { "NTSC 320x240p",    &NTSC_320x240p },
    { "PAL 640x576i",     &PAL_640x576i },
    { "NTSC 640x480i",    &NTSC_640x480i },
    { "PAL 320x576i",     &PAL_320x576i },
    { "NTSC 320x480i",    &NTSC_320x480i },
    { "All-Star Baseball 2001 512x240i",   &PAL_AllStarBaseball2001_512x240i },
    { "Armorines 480x360i (hi-res)",       &PAL_ArmorinesSWARM_480x360i },
    { "Armorines 480x232i (letterbox)",    &PAL_ArmorinesSWARM_480x232i },
    { "Army Men Sarge's Heroes 640x240i",  &PAL_ArmyMenSargesHeroes_640x240i },
    { "Banjo-Kazooie 320x288p",            &PAL_BanjoKazooie_320x288p },
    { "Banjo-Tooie 320x288p",              &PAL_BanjoTooie_320x288p },
    { "Castlevania LoD 490x355i",          &PAL_CastlevaniaLoD_490x355i },
    { "Donald Duck 480x360i",              &PAL_DonaldDuckGoinQuackers_480x360i },
    { "Excitebike64 640x480i",             &PAL_Excitebike64_640x480i },
    { "F-Zero X 320x288p",                 &PAL_FZeroX_320x288p },
    { "FIFA 99 640x480i",                  &PAL_FIFA99_640x480i },
    { "GoldenEye 320x288p",                &PAL_GoldenEye_320x288p },
    { "Hybrid Heaven 320x288p",            &PAL_HybridHeaven_320x288p },
    { "Hybrid Heaven 640x474i",            &PAL_HybridHeaven_640x474i },
    { "Indiana Jones 400x440i",            &PAL_IndianaJones_400x440i },
    { "Jet Force Gemini 320x288p",         &PAL_JetForceGemini_320x288p },
    { "Mario Kart 64 320x288p",            &PAL_MarioKart64_320x288p },
    { "Perfect Dark 320x288p",             &PAL_PerfectDark_320x288p },
    { "Perfect Dark 448x268p (hi-res)",    &PAL_PerfectDark_448x268p },
    { "Quake II 320x240p",                 &PAL_QuakeII_Default320x240p },
    { "Rayman2 320x288p",                  &PAL_Rayman2_320x288p },
    { "Rayman2 480x360p (hi-res)",         &PAL_Rayman2_480x360p },
    { "Re-Volt 640x240p",                  &PAL_ReVolt_640x240p },
    { "RE2 320x288p",                      &PAL_RE2_320x288p },
    { "RE2 640x576i",                      &PAL_RE2_640x576i },
    { "Road Rash 64 640x240p",             &PAL_RoadRash64_640x240p },
    { "Rogue Squadron 400x440i",           &PAL_RogueSquadron_400x440i },
    { "SW Episode I Racer 640x480i",       &PAL_SWEpisodeIRacer_640x480i },
    { "Top Gear Overdrive 320x288p",       &PAL_TopGearOverdrive_320x288p },
    { "Top Gear Overdrive 640x240p",       &PAL_TopGearOverdrive_640x240p },
    { "Turok2 320x288p",                   &PAL_Turok2_320x288p },
    { "Turok2 480x360i",                   &PAL_Turok2_480x360i },
    { "Turok2 480x232i",                   &PAL_Turok2_480x232i },
    { "Turok: Rage Wars 320x288p",         &PAL_TurokRageWars_320x288p },
    { "Turok: Rage Wars 480x360i",         &PAL_TurokRageWars_480x360i },
    { "Vigilante 8 320x288p",              &PAL_Vigilante8_320x288p },
    { "Vigilante 8 480x360i",              &PAL_Vigilante8_480x360i },
    { "Vigilante 8 640x480i",              &PAL_Vigilante8_640x480i },
    { "World Driver Championship 640x480i",&PAL_WorldDriver_640x480i },
    { "Zelda: MM 320x288p",                &PAL_Zelda_MM_320x288p },
    { "Zelda: OoT 320x288p",               &PAL_Zelda_OoT_320x288p }
};

static const int resolution_count = (int)(sizeof(resolution_list) / sizeof(resolution_list[0]));

/* ============================================================
 * (2) ZMIENNE STANU MENU
 * ============================================================ */
static bool menu_running = false;
static int menu_selected = 0;
static int menu_scroll = 0;
static int repeat_up_counter = 0;
static int repeat_down_counter = 0;

/* ============================================================
 * (3) FUNKCJE OBSŁUGI MENU
 * ============================================================ */
void menu_open(void) {
    menu_running = true;
    menu_selected = 0;
    menu_scroll = 0;
    repeat_up_counter = 0;
    repeat_down_counter = 0;
}

void menu_close(void) {
    menu_running = false;
}

bool menu_is_open(void) {
    return menu_running;
}

void menu_set_initial_resolution(const resolution_t *r) {
    if (!r) return;
    for (int i = 0; i < resolution_count; ++i) {
        if (resolution_list[i].res == r) {
            menu_selected = i;
            return;
        }
    }
}

/* ============================================================
 * (4) GŁÓWNA FUNKCJA OBSŁUGI MENU
 * ============================================================ */
menu_status_t menu_update(surface_t *disp, joypad_buttons_t pressed,
                          joypad_buttons_t held, const resolution_t **out_selected) {
    if (!menu_running) {
        if (out_selected) *out_selected = NULL;
        return MENU_STATUS_CANCEL;
    }
    if (!disp) {
        menu_running = false;
        if (out_selected) *out_selected = NULL;
        return MENU_STATUS_CANCEL;
    }

    /* (1) obliczenia pozycji i rozmiarów */
    int screen_w = disp->width;
    int screen_h = disp->height;
    int box_w = MENU_BOX_W_DEFAULT; if (box_w > screen_w - 20) box_w = screen_w - 20;
    int box_h = MENU_BOX_H_DEFAULT; if (box_h > screen_h - 20) box_h = screen_h - 20;
    int box_x = (screen_w - box_w) / 2;
    int box_y = (screen_h - box_h) / 2;
    int pad_x = MENU_PAD_X;
    int pad_y = MENU_PAD_Y;
    int title_h = MENU_TITLE_H;
    int hint_h = MENU_HINT_H;
    int line_h = MENU_LINE_H;
    int avail_h = box_h - pad_y*2 - title_h - hint_h;
    if (avail_h < line_h) avail_h = line_h;
    int visible_lines = avail_h / line_h;

    /* (2) obsługa nawigacji */
    if (pressed.d_up) {
        menu_selected--; repeat_up_counter = 0;
    } else if (pressed.d_down) {
        menu_selected++; repeat_down_counter = 0;
    } else {
        if (held.d_up) {
            repeat_up_counter++;
            if (repeat_up_counter >= KEYREP_INITIAL_FRAMES &&
                (repeat_up_counter - KEYREP_INITIAL_FRAMES) % KEYREP_REPEAT_FRAMES == 0) {
                menu_selected--;
            }
        } else repeat_up_counter = 0;
        if (held.d_down) {
            repeat_down_counter++;
            if (repeat_down_counter >= KEYREP_INITIAL_FRAMES &&
                (repeat_down_counter - KEYREP_INITIAL_FRAMES) % KEYREP_REPEAT_FRAMES == 0) {
                menu_selected++;
            }
        } else repeat_down_counter = 0;
    }

    /* (3) zawijanie indeksu */
    if (menu_selected < 0) menu_selected = resolution_count - 1;
    if (menu_selected >= resolution_count) menu_selected = 0;

    /* (4) przewijanie listy */
    if (menu_selected < menu_scroll) menu_scroll = menu_selected;
    if (menu_selected >= menu_scroll + visible_lines) menu_scroll = menu_selected - visible_lines + 1;

    /* (5) potwierdzenie */
    if (pressed.a) {
        menu_running = false;
        if (out_selected) *out_selected = resolution_list[menu_selected].res;
        return MENU_STATUS_SELECTED;
    }

    /* (6) anulowanie */
    if (pressed.b || pressed.start) {
        menu_running = false;
        if (out_selected) *out_selected = NULL;
        return MENU_STATUS_CANCEL;
    }

    /* (7) kolory i rysowanie */
    uint32_t col_frame = graphics_make_color(0,200,0,255);
    uint32_t col_box_bg = graphics_make_color(24,24,24,200);
    uint32_t col_text = graphics_make_color(255,255,255,255);
    uint32_t col_text_dim = graphics_make_color(180,180,180,255);
    uint32_t col_highlight = graphics_make_color(200,200,0,255);

    graphics_draw_box(disp, box_x, box_y, box_w, box_h, col_box_bg);
    graphics_draw_line(disp, box_x - 2, box_y - 2, box_x + box_w + 1, box_y - 2, col_frame);
    graphics_draw_line(disp, box_x - 2, box_y + box_h + 1, box_x + box_w + 1, box_y + box_h + 1, col_frame);
    graphics_draw_line(disp, box_x - 2, box_y - 2, box_x - 2, box_y + box_h + 1, col_frame);
    graphics_draw_line(disp, box_x + box_w + 1, box_y - 2, box_x + box_w + 1, box_y + box_h + 1, col_frame);

    graphics_set_color(col_text, 0);
    graphics_draw_text(disp, box_x + pad_x, box_y + pad_y, "Wybierz rozdzielczosc:");

    int y = box_y + pad_y + title_h;
    for (int i = 0; i < visible_lines; ++i) {
        int idx = menu_scroll + i;
        if (idx >= resolution_count) break;
        if (idx == menu_selected) {
            graphics_draw_box(disp, box_x + pad_x - 2, y - 2,
                              box_w - pad_x*2 + 4, line_h + 2, col_highlight);
            graphics_set_color(col_box_bg, 0);
        } else {
            graphics_set_color(col_text, 0);
        }
        char tmp[128];
        const char *name = resolution_list[idx].name;
        int max_chars = (box_w - pad_x * 2) / 8;
        int j;
        for (j = 0; j < max_chars - 1 && name[j]; ++j) tmp[j] = name[j];
        tmp[j] = '\0';
        graphics_draw_text(disp, box_x + pad_x, y, tmp);
        y += line_h;
    }

    graphics_set_color(col_text_dim, 0);
    graphics_draw_text(disp, box_x + pad_x, box_y + box_h - hint_h + 2,
                       "A = OK  ENTER/B = Anuluj  D-UP/DOWN");

    if (out_selected) *out_selected = NULL;
    return MENU_STATUS_OPEN;
}
