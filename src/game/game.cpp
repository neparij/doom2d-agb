#include "bn_log.h"
#include "glob.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
#include "files.h"
#include "memory.h"
// #include "vga.h"
// #include "error.h"
// #include "keyb.h"
// #include "sound.h"
#include "view.h"
#include "bmap.h"
#include "fx.h"
#include "switch.h"
#include "weapons.h"
#include "items.h"
#include "dots.h"
// #include "smoke.h"
#include "player.h"
#include "monster.h"
#include "menu.h"
#include <cstring>
#include <tonc_memdef.h>

#include "bn_core.h"
#include "bn_format.h"
#include "bn_string.h"
#include "enc.h"
#include "misc.h"
#include "map.h"
#include "renderer.h"
#include "sound.h"
#include "debug.h"

#define LT_DELAY 8
#define LT_HITTIME 6

#define GETIME 1092

// int A8_start(char*);
// int A8_nextframe(void);
// void A8_close(void);
//
// void FX_trans1(int t);
// extern unsigned char fx_scr1[64000],fx_scr2[64000];

extern short lastkey;

// //extern unsigned short chk;
//
extern int hit_xv,hit_yv;

// extern vgapal std_pal;
void setgamma(int);

extern int sw_secrets;

#define PL_FLASH 90

void Z_clrst(void);

extern int PL_JUMP;

extern map_block_t blk;
//
// extern unsigned char clrmap[256*12];
//
// void V_maptoscr(int,int,int,int,void *);
//
// void F_loadgame(int);
//
extern unsigned char cheat;

unsigned char g_bot=0,_2pl=0,g_dm=0,g_st=GS_TITLE,g_exit=0,g_map=1,_warp=0,g_music[8]="MENU";
int g_pending_newgame=0;
unsigned char _net=0;
int g_sttm=1092;
unsigned int g_time;
int dm_pnum,dm_pl1p,dm_pl2p;
void* sky_ptr = nullptr;
BN_DATA_EWRAM pos_t dm_pos[100];

static pcm_ref_t telepsnd;
static void *scrnh[3];
void *cd_scr;

extern int sky_type;
void *ltn[2][2];
int lt_time,lt_type,lt_side,lt_ypos,lt_force;
pcm_ref_t ltnsnd[2];

int g_trans=0,g_transt;
int last_bg_st = -1;
bool inter_stats_drawn = false;

static void set_trans(int st) {
  switch(g_st) {
    case GS_ENDANIM: case GS_END2ANIM: case GS_DARKEN:
    case GS_BVIDEO: case GS_EVIDEO: case GS_END3ANIM:
      g_st=st;return;
  }
  switch(g_st=st) {
    case GS_ENDANIM: case GS_END2ANIM: case GS_DARKEN:
    case GS_BVIDEO: case GS_EVIDEO: case GS_END3ANIM:
      return;
  }
  g_trans=1;g_transt=0;
}

// void G_savegame(int h) {
//   write(h,&_2pl,1);write(h,&g_dm,1);write(h,&g_exit,1);write(h,&g_map,1);
//   write(h,&g_time,4);write(h,&dm_pl1p,4);write(h,&dm_pl2p,4);
//   write(h,&dm_pnum,4);write(h,dm_pos,dm_pnum*sizeof(pos_t));
//   write(h,&cheat,1);
//   write(h,g_music,8);
// }
//
// void G_loadgame(int h) {
//   read(h,&_2pl,1);read(h,&g_dm,1);read(h,&g_exit,1);read(h,&g_map,1);
//   read(h,&g_time,4);read(h,&dm_pl1p,4);read(h,&dm_pl2p,4);
//   read(h,&dm_pnum,4);read(h,dm_pos,dm_pnum*sizeof(pos_t));
//   read(h,&cheat,1);
//   read(h,g_music,8);F_loadmus(g_music);
//   w_ht=_2pl?98:198;
// }
//
int G_load(const unsigned char* p, unsigned size) {
  switch (blk.t) {
    case MB_MUSIC:
      BN_LOG("G_load: MB_MUSIC");
      if (size < 8) {
        BN_LOG("G_load MB_MUSIC: block too short");
        return 0;
      }
      memcpy(g_music, p, 8);
      F_loadmus(reinterpret_cast<char *>(g_music));
      return 1;
    default:
      return 0;
  }
}

// void load_game(int n) {
//   F_freemus();
//   W_init();
//   F_loadgame(n);
//   set_trans(GS_GAME);
//   V_setscr((g_trans)?fx_scr2:scrbuf);V_setrect(0,320,0,200);
//   V_clr(0,320,0,200,0);
//   if(_2pl) {w_o=0;Z_clrst();w_o=100;Z_clrst();}
//   else {w_o=0;Z_clrst();}
// //  V_copytoscr(0,320,0,200);
//   V_setscr(scrbuf);
//   pl1.drawst=0xFF;
//   if(_2pl) pl2.drawst=0xFF;
//   BM_remapfld();
//   BM_clear(BM_PLR1|BM_PLR2|BM_MONSTER);
//   BM_mark(&pl1.o,BM_PLR1);
//   if(_2pl) BM_mark(&pl2.o,BM_PLR2);
//   MN_mark();
//   S_startmusic();
// }
//
// void BOT_init(void);
//
void G_start(void) {
  // TODO: uncomment all
  char s[8];

  F_freemus();
  s[0] = 'M';
  s[1] = 'A';
  s[2] = 'P';
  s[3] = '0' + (g_map / 10);
  s[4] = '0' + (g_map % 10);
  s[5] = '\0';
  F_loadmap(s);
  // F_loadmap("MAP06");

  set_trans(GS_GAME);
  // V_setscr((g_trans)?fx_scr2:scrbuf);V_setrect(0,320,0,200);
  // V_clr(0,320,0,200,0);
  // if(_2pl) {w_o=0;Z_clrst();w_o=100;Z_clrst();}
  // else {w_o=0;Z_clrst();}
  // V_setscr(scrbuf);
  pl1.drawst=0xFF;
  if(_2pl) pl2.drawst=0xFF;
  g_exit=0;
  itm_rtime=(g_dm)?1092:0;
  p_immortal=0;PL_JUMP=10;
  g_time=0;
  lt_time=1000;
  lt_force=1;
  if(!_2pl) pl1.lives=1;
  BM_remapfld();
  BM_clear(BM_PLR1|BM_PLR2|BM_MONSTER);
  BM_mark(&pl1.o,BM_PLR1);
  if(_2pl) BM_mark(&pl2.o,BM_PLR2);
  MN_mark();
  S_startmusic();
  // BOT_init();
}

/** TEST ONLY: use from main to start level with guaranteed normal state (player alive, with weapon).
 *  Remove or replace with menu flow when menu exists. */
void PL_reset(void);
void G_start_test_level(void) {
  PL_reset();
  G_start();
}

// #define GGAS_TOTAL (MN__LAST-MN_DEMON+16+10)
//
void G_init(void) {
   int i,j;
   char s[9];

   // logo("G_init: настройка ресурсов игры ");
   BN_LOG("G_init: setting up game resources");
//   logo_gas(5,GGAS_TOTAL);
   telepsnd=Z_getsnd("TELEPT");
   scrnh[0]=M_lock(F_getresid("TITLEPIC"));
   scrnh[1]=M_lock(F_getresid("INTERPIC"));
   scrnh[2]=M_lock(F_getresid("ENDPIC"));
   // cd_scr=M_lock(F_getresid("CD1PIC"));
   for(i=0;i<2;++i) {
      ltn[i][0] = Z_getspr("LTN1", i, 0, NULL);
      ltn[i][1] = Z_getspr("LTN2", i, 0, NULL);
   }
   ltnsnd[0]=Z_getsnd("THUND1");
   ltnsnd[1]=Z_getsnd("THUND2");
   DOT_alloc();
//   SMK_alloc();
   FX_alloc();
   WP_alloc();
   IT_alloc();
   SW_alloc();
   PL_alloc();
   MN_alloc();
   Z_initst();
//   logo_gas(GGAS_TOTAL,GGAS_TOTAL);
//   logo("\n");
// //  GM_init();
   pl1.color=0x70;
   pl2.color=0x60;
   g_trans=0;
}
//
// int G_beg_video(void) {
//   switch(g_map) {
//     case 3: return A8_start("FALL");
//     case 4: return A8_start("KORIDOR");
//     case 5: return A8_start("SKULL");
//     case 6: return A8_start("TORCHES");
//     case 7: return A8_start("CACO");
//     case 8: return A8_start("DARTS");
//     case 9: return A8_start("FISH");
//     case 10: return A8_start("TRAP");
//     case 11: return A8_start("JAIL");
//     case 12: return A8_start("MMON1");
//     case 13: return A8_start("TOWER");
//     case 14: return A8_start("SAPOG");
//     case 15: return A8_start("SWITCH");
//     case 16: return A8_start("ACCEL");
//     case 17: return A8_start("MEAT");
//     case 18: return A8_start("LEGION");
//     case 19: return A8_start("CLOUDS");
// //    case : return A8_start("");
//   }
//   return 0;
// }
//
// int G_end_video(void) {
//   switch(g_map) {
//     case 1: return A8_start("TRUBA");
//     case 10: return A8_start("GOTCHA");
// //    case : return A8_start("");
//   }
//   return 0;
// }

static unsigned char transdraw=0;

extern "C" int BOT_getkeys(int i);

void G_act(void) {
   static unsigned char pcnt=0;

   if(g_trans) {
     if(g_transt==0) {
       // V_setscr(NULL);
       // memcpy(fx_scr1,scra,64000);
       // V_setscr(fx_scr2);
       transdraw=1;
       G_draw();
       transdraw=0;
       // V_setscr(scrbuf);
     }
     // FX_trans1(g_transt*2);
     // V_copytoscr(0,320,0,200);
     if(++g_transt>32) {
       g_trans=0;
     }
     return;
   }
   if(g_st==GS_BVIDEO || g_st==GS_EVIDEO) {
// //    V_setscr(NULL);
//     if(!A8_nextframe() || lastkey==1) {
//       if(lastkey==1) lastkey=0;
//       A8_close();
// //      V_setscr(scrbuf);
//       if(g_st==GS_BVIDEO) G_start();
//       else goto inter;
//     }
//     V_copytoscr(0,320,0,200);
// //    V_setscr(scrbuf);
     return;
   }else if(g_st==GS_ENDANIM || g_st==GS_END2ANIM || g_st==GS_END3ANIM) {
     g_st=GS_ENDSCR;
     // TODO: video????

// //    V_setscr(NULL);
//     if(!A8_nextframe()) {
//       switch(g_st) {
//         case GS_ENDANIM: g_st=GS_DARKEN;break;
//         case GS_END2ANIM: g_st=GS_END3ANIM;A8_start("KONEC");break;
//         case GS_END3ANIM: g_st=GS_ENDSCR;lastkey=0;break;
//       }g_sttm=0;return;
//     }
//     V_copytoscr(0,320,0,200);
// //    V_setscr(scrbuf);
     return;
   }else if(g_st==GS_DARKEN) {
// //    if(++g_sttm>=105) {
// //      V_setscr(NULL);V_clr(0,320,0,200,0);V_setscr(scrbuf);
// //      setgamma(gamma);
//       g_st=GS_END2ANIM;A8_start("CREDITS");
// //    }
// //    if(g_sttm>64) return;
// //    VP_tocolor(std_pal,0,0,0,64,g_sttm);VP_setall(pal_tmp);
     return;
   }
  if(g_pending_newgame) {
    g_pending_newgame=0;
    _2pl=0;g_dm=0;g_bot=0;
    g_map=_warp?_warp:1;
    PL_reset();
    pl1.color=0x70;
    R_clear_sprites();
    R_clear();
    G_start();
    return;
  }
  /* GS_INTER: A/B — следующий уровень. Обрабатывать до GM_act(), т.к. GM_act обнуляет lastkey. */
  if (g_st == GS_INTER && !mnu && (lastkey & (KEY_A | KEY_B))) {
    lastkey = 0;
    // if (!G_beg_video()) G_start(); // TODO: video
    inter_text_sprites.clear();
    bn::core::update();
    G_start();
    return;
  }
  if(GM_act()) return;
  switch (g_st) {
    case GS_TITLE:
    case GS_ENDSCR:
      return;
    case GS_INTER:
      return;
   }
   if(sky_type==2) {
     if(lt_time>LT_DELAY || lt_force) {
       if(!(rand()&31) || lt_force) {
         lt_force=0;
         lt_time=-LT_HITTIME;
         lt_type=rand()%2;
         lt_side=rand()&1;
         lt_ypos=rand()&31;
         Z_sound(ltnsnd[rand()&1],128);
       }
     }else ++lt_time;
   }
   ++g_time;
   pl1.hit=0;pl1.hito=-3;
   if(_2pl) {pl2.hit=0;pl2.hito=-3;}
#ifdef D2D_DEBUG_ENABLE
   DBG_act();
#endif
   G_code();
   W_act();
   IT_act();
   SW_act();
  int keys_pl1 = 0, keys_pl2 = 0;
  if (menu_input_block_frames <= 0) {
    keys_pl1 = BOT_getkeys(0);
    keys_pl2 = BOT_getkeys(1);
  } else {
    menu_input_block_frames--;
  }
  if(_2pl) {
    if (pcnt) {
      PL_act(&pl1, keys_pl1);
      PL_act(&pl2, keys_pl2);
    } else {
      PL_act(&pl2, keys_pl2);
      PL_act(&pl1, keys_pl1);
    }
    pcnt ^= 1;
  } else PL_act(&pl1, keys_pl1);
   MN_act();
   if(fld_need_remap) BM_remapfld();
   BM_clear(BM_PLR1|BM_PLR2|BM_MONSTER);
   BM_mark(&pl1.o,BM_PLR1);
   if(_2pl) BM_mark(&pl2.o,BM_PLR2);
   MN_mark();
   WP_act();
   DOT_act();
//   SMK_act();
   FX_act();
  if(_2pl) {
    PL_damage(&pl1);
    PL_damage(&pl2);
    if (!(pl1.f & PLF_PNSND) && pl1.pain) PL_cry(&pl1);
    if (!(pl2.f & PLF_PNSND) && pl2.pain) PL_cry(&pl2);
    if ((pl1.pain -= 5) < 0) {
      pl1.pain = 0;
      pl1.f &= (0xFFFF - PLF_PNSND);
    }
    if ((pl2.pain -= 5) < 0) {
      pl2.pain = 0;
      pl2.f &= (0xFFFF - PLF_PNSND);
    }
  } else {
    PL_damage(&pl1);
    if (!(pl1.f & PLF_PNSND) && pl1.pain) PL_cry(&pl1);
    if ((pl1.pain -= 5) < 0) {
      pl1.pain = 0;
      pl1.f &= (0xFFFF - PLF_PNSND);
    }
  }
  if(g_exit == 1) {
    R_clear_sprites();
    lastkey=0; // Clear the keys, so GS_INTER would not skip.
    // TODO: video????
    // if (G_end_video()) {
    //   F_freemus();
    //   g_st = GS_EVIDEO;
    //   return;
    // }
  inter:
    switch (g_map) {
      case 19: g_st = GS_ENDANIM;
        // A8_start("FINAL"); // TODO: video????
        break;
      case 31:
      case 32: g_map = 16;
        set_trans(GS_INTER);
        break;
      default: ++g_map;
        set_trans(GS_INTER);
        break;
    }
    F_freemus();
    if (g_st == GS_INTER) {
      F_loadmus("INTERMUS");
    } else {
      unsigned char mus_name[8] = {0};
      utf_to_cp866("КОНЕЦ", mus_name, 8);
      F_loadmus(reinterpret_cast<char*>(mus_name));
      if (mus_vol > 0) mus_vol = 128;
    }
    S_startmusic();
  } else if (g_exit == 2) {
    R_clear_sprites();
    switch (g_map) {
      case 31: g_map = 32;
        set_trans(GS_INTER);
        break;
      case 32: g_map = 16;
        set_trans(GS_INTER);
        break;
      default: g_map = 31;
        set_trans(GS_INTER);
        break;
    }
    F_freemus();
    F_loadmus("INTERMUS");
    S_startmusic();
  }
}

obj_t *MN_getobj(int i);

static void drawview(player_t *p) {
  // TODO: uncomment all
  obj_t *o;
  if(p->looky<-w_ht/2) p->looky=-w_ht/2;
  else if(p->looky>w_ht/2) p->looky=w_ht/2;
  o=MN_getobj(p->mon);
  if(o==NULL) {
    w_x=p->o.x;w_y=p->o.y-12+p->looky;
  } else {
    w_x=o->x;w_y=o->y-o->h/2+p->looky;
  }
  R_clear();
  W_draw();
  PL_drawst(p);
#ifdef D2D_DEBUG_ENABLE
  DBG_draw();
#endif
}

static int get_pu_st(int t) {
  if(t>=PL_FLASH) return 1;
  if((t/9)&1) return 0;
  return 1;
}

// static void pl_info(player_t *p,int y) {
//   unsigned int t;
//
//   t=p->kills*10920/g_time;
//   Z_gotoxy(25,y);Z_printbf("УБИЛ");
//   Z_gotoxy(25,y+15);Z_printbf("УБИЙСТВ В МИНУТУ");
//   Z_gotoxy(25,y+30);Z_printbf("НАШЕЛ СЕКРЕТОВ %u ИЗ %u",p->secrets,sw_secrets);
//   Z_gotoxy(255,y);Z_printbf("%u",p->kills);
//   Z_gotoxy(255,y+15);Z_printbf("%u.%u",t/10,t%10);
// }
//
void G_draw(void) {
   int h;
   unsigned short hr,mn,sc;

  // BN_LOG("gs_t: ", g_st, " g_trans: ", g_trans, " transdraw: ", transdraw);
   if(g_trans && !transdraw) return;
   switch(g_st) {
     case GS_ENDANIM: case GS_END2ANIM: case GS_DARKEN:
     case GS_BVIDEO: case GS_EVIDEO: case GS_END3ANIM:
       return;
     case GS_TITLE:
       if (last_bg_st != GS_TITLE) {
         last_bg_st = GS_TITLE;
         V_clr(0,0,240,160,0);
         V_pic_2buf(0,0,scrnh[0]);
       }
       break;
     case GS_ENDSCR:
       if (last_bg_st != GS_ENDSCR) {
         last_bg_st = GS_ENDSCR;
         V_clr(0,0,240,160,0);
         V_pic_2buf(0,0,scrnh[2]);
       }
       break;
     case GS_INTER: {
       if (last_bg_st != GS_INTER) {
         last_bg_st = GS_INTER;
         V_clr(0,0,240,160,0);
         V_pic_2buf(0,0,scrnh[1]);
       }
       if (!inter_stats_drawn && !mnu) {
         inter_stats_drawn = true;
         inter_text_sprites.clear();
	      if (text_stbf && text_wi) {
	        text_stbf->set_left_alignment();
	        text_wi->set_left_alignment();
	        const int level_completed_text_width = text_stbf->width("УРОВЕНЬ ПРОЙДЕН");
	        const int level_timer_text_p0_width = text_stbf->width("ЗА ");
	        const int level_timer_text_p1_width = text_wi->width("0:00:00");
	        const int level_timer_text_width = level_timer_text_p0_width + level_timer_text_p1_width;

	        text_stbf->generate(-level_completed_text_width / 2, -60, "УРОВЕНЬ ПРОЙДЕН", inter_text_sprites);
	        Z_calc_time(g_time, &hr, &mn, &sc);
	        text_stbf->generate(-level_timer_text_width/2, -35, "ЗА", inter_text_sprites);
	        text_wi->generate(-level_timer_text_width/2 + level_timer_text_p0_width, -35, bn::format<12>("{}:{}{}:{}{}", hr, mn/10, mn%10, sc/10, sc%10), inter_text_sprites);
	        // h = _2pl ? -5 : 0;
	        // if (_2pl) {
	        //   text_stbf->generate(-80, h - 15, "ПЕРВЫЙ ИГРОК", inter_text_sprites);
	        //   text_stbf->generate(-80, h + 45, "ВТОРОЙ ИГРОК", inter_text_sprites);
	        // }
	        h = 0;
	        {
	          player_t *p = &pl1;
	          const int level_secrets_N_offset = text_wi->width(bn::format<5>("{}", p->secrets));
	          const int level_secrets_N_of_offset = level_secrets_N_offset + text_stbf->width("ИЗ");

	          int y = h;
	          text_stbf->generate(-95, y, "УБИЛ", inter_text_sprites);
	          text_wi->generate(55, y, bn::format<8>("{}", p->kills), inter_text_sprites);
	          unsigned int kpm = g_time ? p->kills * 10920 / g_time : 0;
	          text_stbf->generate(-95, y + 18, "УБ. В МИН.", inter_text_sprites);
	          text_wi->generate(55, y + 18, bn::format<8>("{}.{}", kpm / 10, kpm % 10), inter_text_sprites);
	          text_stbf->generate(-95, y + 36, "СЕКРЕТОВ", inter_text_sprites);
	          text_wi->generate(55, y + 36, bn::format<4>("{}", p->secrets), inter_text_sprites);
	          text_wi->generate(55 + level_secrets_N_of_offset + 8, y + 36, bn::format<4>("{}", sw_secrets), inter_text_sprites);
	          text_stbf->generate(55 + level_secrets_N_offset + 4, y + 36, "ИЗ", inter_text_sprites);
	        }
	        // if (_2pl) {
	        //   player_t *p = &pl2;
	        //   int y = h + 60;
	        //   text_stbf->generate(-95, y, "УБИЛ", inter_text_sprites);
	        //   text_wi->generate(55, y, bn::format<8>("{}", p->kills), inter_text_sprites);
	        //   unsigned int kpm = g_time ? p->kills * 10920 / g_time : 0;
	        //   text_stbf->generate(-95, y + 18, "УБИЙСТВ В МИНУТУ", inter_text_sprites);
	        //   text_wi->generate(55, y + 18, bn::format<8>("{}.{}", kpm / 10, kpm % 10), inter_text_sprites);
	        //   text_stbf->generate(-95, y + 36, "НАШЕЛ СЕКРЕТОВ", inter_text_sprites);
	        //   text_wi->generate(55, y + 36, bn::format<12>("{} / {}", p->secrets, sw_secrets), inter_text_sprites);
	        // }
	      }
       }
	  break;
	  }
   }
   if (g_st != GS_INTER && inter_stats_drawn) {
     inter_stats_drawn = false;
     inter_text_sprites.clear();
   }
   if (g_st == GS_GAME && last_bg_st != -1) last_bg_st = -1;
   if(g_st!=GS_GAME) {
     if(g_trans) return;
     GM_draw();
//     V_copytoscr(0,320,0,200);
     return;
   }
   if(_2pl) {
// 	w_o=0;drawview(&pl1);
// 	w_o=100;drawview(&pl2);
   } else {;
     w_o = 0;
     drawview(&pl1);
   }
   if(g_trans) return;
  if(GM_draw()) {
    pl1.drawst = pl2.drawst = 0;
    return;
  }
   if(pl1.invl) h=get_pu_st(pl1.invl)*6;
   else if(pl1.pain<15) h=0;
   else if(pl1.pain<35) h=1;
   else if(pl1.pain<55) h=2;
   else if(pl1.pain<75) h=3;
   else if(pl1.pain<95) h=4;
   else h=5;
  V_apply_palette_effects(h);
//   if(h) V_maptoscr(0,200,(_2pl)?1:1,w_ht,clrmap+h*256);
//   else V_copytoscr(0,200,(_2pl)?1:1,w_ht);
//   if(pl1.drawst) V_copytoscr(200,120,(_2pl)?0:0,100);
//   pl1.drawst=0;
   if(_2pl) {
    // if(pl2.invl) h=get_pu_st(pl2.invl)*6;
    // else if (pl2.pain < 15) h = 0;
    // else if (pl2.pain < 35) h = 1;
    // else if (pl2.pain < 55) h = 2;
    // else if (pl2.pain < 75) h = 3;
    // else if (pl2.pain < 95) h = 4;
    // else h = 5;
    // if (h) V_maptoscr(0, 200, 101, w_ht, clrmap + h * 256);
    // else V_copytoscr(0, 200, 101, w_ht);
    // if (pl2.drawst) V_copytoscr(200, 120, 100, 100);
    // pl2.drawst = 0;
   }
}

void G_respawn_player(player_t *p) {
  int i;

  if(dm_pnum==2) {
    if(p==&pl1) i=dm_pl1p^=1;
    else i=dm_pl2p^=1;
    p->o.x=dm_pos[i].x;p->o.y=dm_pos[i].y;p->d=dm_pos[i].d;
    FX_tfog(dm_pos[i].x,dm_pos[i].y);Z_sound(telepsnd,128);
    return;
  }
  do{i=random(dm_pnum);}while(i==dm_pl1p || i==dm_pl2p);
  p->o.x=dm_pos[i].x;p->o.y=dm_pos[i].y;p->d=dm_pos[i].d;
  if(p==&pl1) dm_pl1p=i; else dm_pl2p=i;
  FX_tfog(dm_pos[i].x,dm_pos[i].y);Z_sound(telepsnd,128);
}
