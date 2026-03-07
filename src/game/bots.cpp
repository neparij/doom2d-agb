#include "menu.h"

extern "C" {
#include "glob.h"
#include "view.h"
#include "player.h"
//#include "keyb.h"
    extern unsigned char g_bot;
}

#include "botplay.h"
#include "bn_keypad.h"

BotPlayer *bot;

extern "C" void BOT_init(void) {
    if(bot) {
        delete bot;
        bot=NULL;
    }
    bot=BOT_new();
}

extern "C" int BOT_getkeys(int id) {
    int k;
    player_t *p;

    if(id==0) p=&pl1;
    else if(id==1) p=&pl2;
    else return 0;

    if(g_bot && id==1) {
        if(!bot) return 0;
        return bot->getkeys(p);
    }

    // TODO: use keys array instead of direct access to keys[p->kl] etc.
    k=0;
    /*
    if(keys[p->kl]) k|=PLK_L;
    if(keys[p->kr]) k|=PLK_R;
    if(keys[p->ku]) k|=PLK_U;
    if(keys[p->kd]) k|=PLK_D;
    if(keys[p->kf]) k|=PLK_F;
    if(keys[p->kj]) k|=PLK_J;
    if(keys[p->kp]) k|=PLK_P;
    if(keys[p->kwl]) k|=PLK_WL;
    if(keys[p->kwr]) k|=PLK_WR;
    */

    // if (bn::keypad::left_held()) k |= PLK_L;
    // if (bn::keypad::right_held()) k |= PLK_R;
    // if (bn::keypad::up_held()) k |= PLK_U;
    // if (bn::keypad::down_held()) k |= PLK_D;
    // if (bn::keypad::b_held()) k |= PLK_F;
    // if (bn::keypad::a_held()) k |= PLK_J;
    // if (bn::keypad::select_held()) k |= PLK_P;
    // if (bn::keypad::l_held()) k |= PLK_WL;
    // if (bn::keypad::r_held()) k |= PLK_WR;


    // if (bn::keypad::left_held()) k |= PLK_L;
    // if (bn::keypad::right_held()) k |= PLK_R;
    // if (bn::keypad::up_held()) k |= PLK_U;
    // if (bn::keypad::down_held()) k |= PLK_D;
    // if (bn::keypad::r_held()) k |= PLK_F;
    // if (bn::keypad::b_held()) k |= PLK_J;
    // if (bn::keypad::a_held()) k |= PLK_P;
    // // if (bn::keypad::l_held()) k |= PLK_WL;
    // if (bn::keypad::l_held()) k |= PLK_WR;

    if (bn::keypad::left_held()) k |= PLK_L;
    if (bn::keypad::right_held()) k |= PLK_R;
    if (bn::keypad::up_held()) k |= PLK_U;
    if (bn::keypad::down_held()) k |= PLK_D;
    if (bn::keypad::b_held()) k |= PLK_F;
    if (bn::keypad::a_held()) k |= PLK_J;
    if (bn::keypad::r_held()) k |= PLK_P;
    if (bn::keypad::l_held()) k |= PLK_WR;
    // if (bn::keypad::r_held()) k |= PLK_WR;
    return k;
}
