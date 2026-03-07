#include "debug.h"

#include <cstring>
#include <tonc_memdef.h>


#include "bn_keypad.h"
#include "bn_log.h"
#include "menu.h"
#include "sound.h"
#ifdef D2D_DEBUG_ENABLE

static pcm_ref_t dbg_onsnd, dbg_offsnd;

pcm_ref_t Z_getsnd(char [6]);

int Z_sound(pcm_ref_t &, int);

bool dbg_draw_fldb = true;
bool dbg_draw_fldf = true;
bool dbg_draw_sky = true;
bool dbg_draw_mn = true;
bool dbg_draw_it = true;
bool dbg_draw_dot = true;
bool dbg_draw_water = true;

void DBG_init() {
    BN_LOG("Debug mode enabled");
    dbg_onsnd = Z_getsnd("SWTCHN");
    dbg_offsnd = Z_getsnd("SWTCHX");
}

void DBG_draw() {
}

void DBG_act() {
    if (bn::keypad::select_held()) {
        if (keybuf[29] == KEY_R && keybuf[30] == KEY_R && keybuf[31] == KEY_UP) {
            dbg_draw_mn = !dbg_draw_mn;
            BN_LOG(" >>> dbg_draw_mn: ", dbg_draw_mn);
            Z_sound(dbg_draw_mn ? dbg_onsnd : dbg_offsnd, 128);
        } else if (keybuf[29] == KEY_R && keybuf[30] == KEY_R && keybuf[31] == KEY_DOWN) {
            dbg_draw_it = !dbg_draw_it;
            BN_LOG(" >>> dbg_draw_it: ", dbg_draw_it);
            Z_sound(dbg_draw_it ? dbg_onsnd : dbg_offsnd, 128);
        } else if (keybuf[29] == KEY_R && keybuf[30] == KEY_R && keybuf[31] == KEY_LEFT) {
            dbg_draw_dot = !dbg_draw_dot;
            BN_LOG(" >>> dbg_draw_dot: ", dbg_draw_dot);
            Z_sound(dbg_draw_dot ? dbg_onsnd : dbg_offsnd, 128);
        } else if (keybuf[30] == KEY_R && keybuf[31] == KEY_UP) {
            dbg_draw_sky = !dbg_draw_sky;
            BN_LOG(" >>> dbg_draw_sky: ", dbg_draw_sky);
            Z_sound(dbg_draw_sky ? dbg_onsnd : dbg_offsnd, 128);
        } else if (keybuf[30] == KEY_R && keybuf[31] == KEY_DOWN) {
            dbg_draw_water = !dbg_draw_water;
            BN_LOG(" >>> dbg_draw_water: ", dbg_draw_water);
            Z_sound(dbg_draw_water ? dbg_onsnd : dbg_offsnd, 128);
        } else if (keybuf[30] == KEY_R && keybuf[31] == KEY_LEFT) {
            dbg_draw_fldb = !dbg_draw_fldb;
            BN_LOG(" >>> dbg_draw_fldb: ", dbg_draw_fldb);
            Z_sound(dbg_draw_fldb ? dbg_onsnd : dbg_offsnd, 128);
        } else if (keybuf[30] == KEY_R && keybuf[31] == KEY_RIGHT) {
            dbg_draw_fldf = !dbg_draw_fldf;
            BN_LOG(" >>> dbg_draw_fldf: ", dbg_draw_fldf);
            Z_sound(dbg_draw_fldf ? dbg_onsnd : dbg_offsnd, 128);
        } else {
            return;
        }

        memset(keybuf, 0, sizeof(keybuf));
    }
}

#endif
