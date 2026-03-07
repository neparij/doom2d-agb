//
// Created by Николай Лаптев on 05.03.26.
//

#ifndef DOOM2D_DEBUG_H
#define DOOM2D_DEBUG_H

#ifdef D2D_DEBUG_ENABLE

void DBG_init(void);
void DBG_draw(void);
void DBG_act(void);

extern bool dbg_draw_fldb;
extern bool dbg_draw_fldf;
extern bool dbg_draw_sky;
extern bool dbg_draw_mn;
extern bool dbg_draw_it;
extern bool dbg_draw_dot;
extern bool dbg_draw_water;

#endif

#endif //DOOM2D_DEBUG_H