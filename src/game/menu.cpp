#include "glob.h"
// #include <stdio.h>
#include <cstring>
// #include <stdlib.h>
#include "files.h"
#include "memory.h"
// #include "vga.h"
// #include "error.h"
// #include "keyb.h"
#include "sound.h"
#include "view.h"
#include "player.h"
#include "switch.h"
#include "menu.h"
#include "renderer.h"
#include "save.h"

#include "bn_sprite_items_m_skull.h"
#include "bn_sprite_items_m_volbar.h"
#include "bn_sprite_items_m_volknob.h"
#include "bn_sprite_items_m_lsbar.h"

#include <tonc_memdef.h>

#include "bn_core.h"
#include "bn_core_update_callback_type.h"
#include "bn_keypad.h"
#include "bn_log.h"
#include "misc.h"
#include "../../../butano/butano/src/bn_sprites_manager_hot.h"
#include "../../../butano/butano/src/bn_sprite_tiles_manager.h"

void Z_clrst(void);

void F_nextmus(char*);

#define QSND_NUM 14

enum{HIT100,ARMOR,JUMP,WPNS,IMMORTAL,SPEED,OPEN,EXIT};

extern int PL_JUMP,PL_RUN;
extern unsigned char _warp,cheat,p_fly;

extern unsigned char g_music[8];

// extern unsigned char savname[7][24],savok[7];
// void F_getsavnames(void);
// void F_savegame(int,char*);
// void load_game(int);
void PL_reset(void);

static unsigned char ibuf[24],input=0;
static int icur;

enum{MENU,MSG};
enum{CANCEL,NEWGAME,LOADGAME,SAVEGAME,OPTIONS,QUITGAME,QUIT,ENDGAME,ENDGM,
  PLR1,PLR2,COOP,DM,BM,VOLUME,GAMMA,LOAD,SAVE,SAVEYES,SAVENO,PLCOLOR,PLCEND,MUSIC,INTERP,
  SVOLM,SVOLP,MVOLM,MVOLP,GAMMAM,GAMMAP,PL1CM,PL1CP,PL2CM,PL2CP};

// #ifndef DEMO
// static int qsnd[QSND_NUM];
// #endif

static char *main_txt[]={
  // "НОВАЯ ИГРА","СТАРАЯ ИГРА","СОХРАНИТЬ ИГРУ","РАЗНОЕ"//,"ВЫХОД"
  "НОоВАЯ ИГРА","СТАРАЯ ИГРА","РАЗНОоЕ"//,"ВЫХОД"
},*opt_txt[]={
  "НАЧАТЬ ЗАНОоВОо","ГРОоМКОоСТЬ","ЯРКОоСТЬ"//,"МУЗЫКА","ИНТЕРПОЛЯЦИЯ:"
},*ngplr_txt[]={
  ""//"ОДИН ИГРОК","ДВА ИГРОКА"
},*ngdm_txt[]={
  ""//"COOPERATIVE","DEATHMATCH","BOTMATCH"
},*vol_txt[]={
  "ЗВУК","МУЗЫКА"
},*plcolor_txt[]={
  ""//"ПЕРВЫЙ","ВТОРОЙ"
},*gamma_txt[]={
  ""
};

static unsigned char main_typ[]={
  NEWGAME,LOADGAME,OPTIONS//,QUITGAME
},ngplr_typ[]={
  0//PLR1,PLR2
},ngdm_typ[]={
  0//COOP,DM,BM
},opt_typ[]={
  ENDGAME,VOLUME,GAMMA//,MUSIC,INTERP
},quit_typ[]={
  QUIT,CANCEL
},endgm_typ[]={
  ENDGM,CANCEL
},saveask_typ[]={
  SAVEYES,SAVENO
},vol_typ[]={
  SVOLM,MVOLM
},plcolor_typ[]={
  0//PL1CM,PL2CM
},gamma_typ[]={
  GAMMAM
},load_typ[]={
  LOAD,LOAD,LOAD,LOAD,LOAD,LOAD,LOAD
},save_typ[]={
  SAVE,SAVE,SAVE,SAVE,SAVE,SAVE,SAVE
};

static menu_t main_mnu={
  MENU,3,0,80,"МЕНЮю",main_txt,main_typ
},opt_mnu={
  MENU,3,0,75,"РАЗНОоЕ",opt_txt,opt_typ
},ngplr_mnu={
  MENU,2,0,90,"НОоВАЯ ИГРА",ngplr_txt,ngplr_typ
},ngdm_mnu={
  MENU,2,0,90,"ТИП ИГРЫы",ngdm_txt,ngdm_typ
},vol_mnu={
  MENU,2,0,40,"ГРОоМКОоСТЬ",vol_txt,vol_typ
},plcolor_mnu={
  MENU,2,0,90,"ЦВЕТ",plcolor_txt,plcolor_typ
},gamma_mnu={
  MENU,1,0,40,"ЯРКОоСТЬ",gamma_txt,gamma_typ
},load_mnu={
  MENU,7,0,85,"ЗАГРУЗИТЬ ИГРУ",NULL,load_typ
},save_mnu={
  MENU,7,0,85,"СОоХРАНИТЬ ИГРУ",NULL,save_typ
},quit1_msg={
  MSG,0,0,0,"ВЫ ДУМАЕТЕ, ОТСЮДА ТАК ПРОСТО ВЫЙТИ?",NULL,quit_typ
},quit2_msg={
  MSG,0,0,0,"ХОРОШЕНЬКО ПОДУМАЙТЕ ПЕРЕД ТЕМ КАК ВЫЙТИ",NULL,quit_typ
},quit3_msg={
  MSG,0,0,0,"У ВАС ЧТО, КОНЧИЛИСЬ ПАТРОНЫ?",NULL,quit_typ
},endgm_msg={
  MSG,0,0,0,"НАЧАТЬ ЭТОТ УРОВЕНЬ ЗАНОВО?",NULL,endgm_typ
},saveask_msg={
  MSG,0,0,0,"СОХРАНИТЬ ИГРУ?",NULL,saveask_typ
};

static menu_t *qmsg[3]={&quit1_msg,&quit2_msg,&quit3_msg};

menu_t *mnu=NULL;
int menu_input_block_frames = 0;
static unsigned char inter_save_flow = 0;

static unsigned char gm_redraw=0;
static int gm_tm=0;
short lastkey=0;
static pcm_ref_t csnd1,csnd2,msnd1,msnd2,msnd3,msnd4,msnd5,msnd6;
static int movsndt=0;
//static vgaimg *msklh[2],*mbarl,*mbarm,*mbarr,*mbaro,*mslotl,*mslotm,*mslotr;
unsigned short keybuf[32];

// static snd_t *voc=NULL;
// static int voc_ch=0;

void GMV_stop(void) {
  // if(voc) {
  //   if(voc_ch) {S_stop(voc_ch);voc_ch=0;}
  //   free(voc);voc=NULL;
  // }
}

void GMV_say(char *nm) {
  // int r,len;
  // snd_t *p;
  // unsigned char *d;
  //
  // if((r=F_findres(nm))==-1) return;
  // if(!(p=malloc((len=F_getreslen(r))+16))) return;
  // p->len=len;p->rate=11000;
  // p->lstart=p->llen=0;
  // GMV_stop();
  // F_loadres(r,p+1,0,len);
  // for(d=(unsigned char*)(p+1);len;--len,++d) *d^=128;
  // voc=p;
  // voc_ch=S_play(voc,-1,1024,255);
}

// TODO: Cheat Codes
void G_code(void) {
  pcm_ref_t* s;

  s=&csnd2;

  if (keybuf[21] == KEY_UP &&
      keybuf[22] == KEY_UP &&
      keybuf[23] == KEY_DOWN &&
      keybuf[24] == KEY_DOWN &&
      keybuf[25] == KEY_LEFT &&
      keybuf[26] == KEY_RIGHT &&
      keybuf[27] == KEY_LEFT &&
      keybuf[28] == KEY_RIGHT &&
      keybuf[29] == KEY_B &&
      keybuf[30] == KEY_A &&
      keybuf[31] == KEY_SELECT
  ) {
    // IDDQD (KONAMI CODE)
    PL_hit(&pl1,400,0,HIT_SOME);
    if(_2pl) PL_hit(&pl2,400,0,HIT_SOME);
    s=&csnd1;
  } else if (keybuf[25] == KEY_SELECT &&
             keybuf[26] == KEY_SELECT &&
             keybuf[27] == KEY_A &&
             keybuf[28] == KEY_L &&
             keybuf[29] == KEY_L &&
             keybuf[30] == KEY_A &&
             keybuf[31] == KEY_RIGHT) {
    // RAMBO (SELECT,SELECT,A,A,L,L,RIGHT)
    pl1.ammo=pl1.shel=pl1.rock=pl1.cell=pl1.fuel=30000;
    pl1.wpns=0x7FF;pl1.drawst|=PL_DRAWWPN|PL_DRAWKEYS;
    pl1.keys=0x70;
  } else if (keybuf[25] == KEY_SELECT &&
             keybuf[26] == KEY_SELECT &&
             keybuf[27] == KEY_A &&
             keybuf[28] == KEY_L &&
             keybuf[29] == KEY_L &&
             keybuf[30] == KEY_A &&
             keybuf[31] == KEY_LEFT) {
    // TANK (SELECT,SELECT,A,A,L,L,LEFT)
    pl1.life=pl1.armor = 200;
    pl1.drawst |= PL_DRAWARMOR | PL_DRAWLIFE;
  } else if (keybuf[25] == KEY_SELECT &&
             keybuf[26] == KEY_SELECT &&
             keybuf[27] == KEY_A &&
             keybuf[28] == KEY_L &&
             keybuf[29] == KEY_L &&
             keybuf[30] == KEY_A &&
             keybuf[31] == KEY_R) {
    // GOODBYE (SELECT,SELECT,A,A,L,L,A,R)
    g_exit=1;
  } else return;

  memset(keybuf,0,sizeof(keybuf));
  Z_sound(*s,128);
//   void *s;
//
//   s=csnd2;
//   if(memcmp(cbuf+32-5,"\x17\x20\x20\x10\x20",5)==0) {
//     PL_hit(&pl1,400,0,HIT_SOME);
//     if(_2pl) PL_hit(&pl2,400,0,HIT_SOME);
//     s=csnd1;
//   }else if(memcmp(cbuf+32-4,"\x14\x1E\x31\x25",4)==0) {
//     pl1.life=pl1.armor=200;pl1.drawst|=PL_DRAWARMOR|PL_DRAWLIFE;
//     if(_2pl) {pl2.life=pl2.armor=200;pl2.drawst|=PL_DRAWARMOR|PL_DRAWLIFE;}
//   }else if(memcmp(cbuf+32-8,"\x30\x16\x26\x26\x21\x13\x18\x22",8)==0) {
//     PL_JUMP=(PL_JUMP==10)?20:10;
//   }else if(memcmp(cbuf+32-8,"\x21\x18\x13\x32\x16\x26\x1E\x02",8)==0) {
//     PL_RUN=(PL_RUN==8)?24:8;
//   }else if(memcmp(cbuf+32-5,"\x13\x1E\x32\x30\x18",5)==0) {
//     pl1.ammo=pl1.shel=pl1.rock=pl1.cell=pl1.fuel=30000;
//     pl1.wpns=0x7FF;pl1.drawst|=PL_DRAWWPN|PL_DRAWKEYS;
//     pl1.keys=0x70;
//     if(_2pl) {
//       pl2.ammo=pl2.shel=pl2.rock=pl2.cell=pl1.fuel=30000;
//       pl2.wpns=0x7FF;pl2.drawst|=PL_DRAWWPN|PL_DRAWKEYS;
//       pl2.keys=0x70;
//     }
//   }else if(memcmp(cbuf+32-5,"\x16\x24\x23\x14\x11",5)==0) {
//     p_immortal=!p_immortal;
//   }else if(memcmp(cbuf+32-9,"\x33\x14\x25\x1F\x10\x24\x23\x14\x25",9)==0) {
//     p_fly=!p_fly;
//   }else if(memcmp(cbuf+32-6,"\x2E\x30\x2F\x2E\x30\x2F",6)==0) {
//     SW_cheat_open();
//   }else if(memcmp(cbuf+32-7,"\x22\x18\x18\x20\x30\x15\x12",7)==0) {
//     g_exit=1;
//   }else if(memcmp(cbuf+32-9,"\x22\x24\x17\x14\x25\x15\x21",7)==0) {
//     if(cbuf[30]>=2 && cbuf[30]<=11 && cbuf[31]>=2 && cbuf[31]<=11) {
//       g_map=(cbuf[30]==11)?0:(cbuf[30]-1)*10;
//       g_map+=(cbuf[31]==11)?0:(cbuf[31]-1);
//       G_start();
//     }
//   }else return;
//   memset(cbuf,0,32);
//   Z_sound(s,128);
}

void GM_set(menu_t *m) {
  if (m == NULL && mnu != NULL && g_st == GS_GAME)
    menu_input_block_frames = 3;
  mnu=m;
  gm_redraw=1;
  if(g_st==GS_GAME) {
    // TODO: GS_GAME
	  // V_setrect(0,320,0,200);V_clr(0,320,0,200,0);
	  // if(_2pl) {V_setrect(200,120,0,200);w_o=0;Z_clrst();w_o=100;Z_clrst();}
	  // else {V_setrect(200,120,0,200);w_o=0;Z_clrst();}
	  // pl1.drawst=pl2.drawst=0xFF;V_setrect(0,320,0,200);

    w_o=0;
    pl1.drawst=0xFF;
    BN_LOG("GM_set: g_st==GS_GAME, pl1.drawst set to 0xFF");
  }
}

void GM_open_save_prompt(void) {
  inter_save_flow = 1;
  GM_set(&saveask_msg);
}

void GM_command(int c) {
  switch(c) {
    case CANCEL:
      GM_set(NULL);break;
    // case INTERP:
    //   s_interp=!s_interp;
    //   GM_set(mnu);
    //   break;
    // case MUSIC:
    //   F_freemus();
    //   F_nextmus(g_music);
    //   F_loadmus(g_music);
    //   S_startmusic();
    //   GM_set(mnu);
    //   break;
    case NEWGAME:
      GMV_say("_NEWGAME");
      Z_sound(msnd2, 128);
      g_pending_newgame = 1;
      GM_set(NULL);
      break;
    // case PLR2:
    //   GMV_say("_2PLAYER");
    //   GM_set(&ngdm_mnu);break;
    // case PLR1:
    //   GMV_say("_1PLAYER");
    //   ngdm_mnu.cur=0;
    // case COOP: case DM: case BM:
    //   if(c==COOP) GMV_say("_COOP");
    //   else if(c==DM) GMV_say("_DM");
    //   if(c!=PLR1) {GM_set(&plcolor_mnu);break;}
    // case PLCEND:
    //   _2pl=ngplr_mnu.cur;
    //   w_ht=_2pl?98:198;
    //   g_dm=ngdm_mnu.cur!=0;
    //   g_bot=ngdm_mnu.cur==2;
    //   g_map=(_warp)?_warp:1;
    //   PL_reset();
    //   if(_2pl) {
    //     pl1.color=pcolortab[p1color];
    //     pl2.color=pcolortab[p2color];
    //   }else pl1.color=0x70;
    //   G_start();
    //   GM_set(NULL);break;
    case OPTIONS:
      GMV_say("_RAZNOE");
      GM_set(&opt_mnu);break;
    case LOADGAME:
      GMV_say("_OLDGAME");
      GM_set(&load_mnu);break;
    case SAVE:
      SV_save_slot(save_mnu.cur);
      Z_sound(msnd2, 128);
      GM_set(NULL);
      if (inter_save_flow) {
        inter_save_flow = 0;
        G_start();
      }
      break;
    case LOAD:
      if (!SV_slot_used(load_mnu.cur)) {
        Z_sound(msnd4, 128);
        break;
      }
      g_pending_load = load_mnu.cur + 1;
      GM_set(NULL);
      break;
    case SAVEYES:
      GM_set(&save_mnu);break;
    case SAVENO:
      inter_save_flow = 0;
      GM_set(NULL);
      G_start();
      break;
	case VOLUME:
	  GMV_say("_VOLUME");
	  GM_set(&vol_mnu);break;
	case GAMMA:
	  GMV_say("_GAMMA");
	  GM_set(&gamma_mnu);break;
	// case QUITGAME:
	//   GMV_say((rand()&1)?"_EXIT1":"_EXIT2");
	//   GM_set(qmsg[random(3)]);break;
	case ENDGAME:
	  if(g_st!=GS_GAME) break;
	  GMV_say("_RESTART");
	  GM_set(&endgm_msg);break;
// 	case QUIT:
// 	  F_freemus();
// 	  GMV_stop();
// #ifndef DEMO
// 	  for(c=(Z_sound(M_lock(qsnd[random(QSND_NUM)]),256)+9)<<16,timer=0;timer<c;);
// #endif
// 	  ERR_quit();break;
    case ENDGM:
      bn::core::update();
	    PL_reset();G_start();
	    GM_set(NULL);break;
	// case PL1CM:
	//   if(--p1color<0) p1color=PCOLORN-1; break;
	// case PL1CP:
	//   if(++p1color>=PCOLORN) p1color=0; break;
	// case PL2CM:
	//   if(--p2color<0) p2color=PCOLORN-1; break;
	// case PL2CP:
	//   if(++p2color>=PCOLORN) p2color=0; break;
	case SVOLM:
	  if ((snd_vol -= 8) < 0) snd_vol = 0;
	  S_apply_volume();
	  SV_commit();
	  GM_set(mnu);
	  break;
	case SVOLP:
	  if ((snd_vol += 8) > 128) snd_vol = 128;
	  S_apply_volume();
	  SV_commit();
	  GM_set(mnu);
	  break;
	case MVOLM:
	  if ((mus_vol -= 8) < 0) mus_vol = 0;
	  S_apply_volume();
	  SV_commit();
	  GM_set(mnu);
	  break;
	case MVOLP:
	  if ((mus_vol += 8) > 128) mus_vol = 128;
	  S_apply_volume();
	  SV_commit();
	  GM_set(mnu);
	  break;
	case GAMMAM:
	  if ((pal_level -= 8) < 0) pal_level = 0;
	  V_apply_display();
	  SV_commit();
	  GM_set(mnu);
	  break;
	case GAMMAP:
	  if ((pal_level += 8) > 128) pal_level = 128;
	  V_apply_display();
	  SV_commit();
	  GM_set(mnu);
	  break;
  }
}

// unsigned char keychar[2][128]={{
//   0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
//   'Q','W','E','R','T','Y','U','I','O','P','[',']','\r',0,'A','S',
//   'D','F','G','H','J','K','L',';','\'',0,0,'\\','Z','X','C','V',
//   'B','N','M',',','.','/',0,'*',0,' ',0,0,0,0,0,0,
//   0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,
//   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
//   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
//   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
// },{
//   0,0,'!','\"','#','$','%',':','&','*','(',')','_','+',0,0,
//   'Й','Ц','У','К','Е','Н','Г','Ш','Щ','З','Х','Ъ','\r',0,'Ф','Ы',
//   'В','А','П','Р','О','Л','Д','Ж','Э',0,0,0,'Я','Ч','С','М',
//   'И','Т','Ь','Б','Ю','?',0,'*',0,' ',0,0,0,0,0,0,
//   0,0,0,0,0,0,0,0,0,0,'-',0,0,0,'+',0,
//   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
//   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
//   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
// }};
//
// extern vgapal main_pal,std_pal;
// extern unsigned char shot_vga;
//
// static void shot(void) {
//   FILE *h;
//   static int num=1;
//   char fn[13];
//
//   sprintf(fn,"shot%04d.vga",num);
//   h=fopen(fn,"wb");
//   fwrite("VGAED2",7,1,h);
//   fwrite(std_pal,768,1,h);
//   fwrite("scr\0\x40\1\xC8\0\0\0\0\0",12,1,h);
//   fwrite((void*)0xA0000,64000,1,h);
//   fwrite("",1,1,h);
//   fclose(h);
//   ++num;
// }
//
int GM_act(void) {
  unsigned char c;

  if (movsndt > 0) --movsndt;
  else movsndt = 0;

  if (g_st == GS_TITLE && !mnu && lastkey) {
    Z_sound(msnd3, 128);
    GM_set(&main_mnu);
    lastkey = 0;
    return 1;
  }

  if (input) {
    if (lastkey & (KEY_START | KEY_A)) {
      input = 0;
      GM_set(NULL);
    }
    lastkey = 0;
    return (mnu ? 1 : 0);
  }

  /* Обрабатываем только ОДНУ клавишу за кадр (приоритет), т.к. lastkey — битмаска */
  if (lastkey & KEY_START) {
    if (!mnu) {
      Z_sound(msnd3, 128);
      GM_set(&main_mnu);
    } else if (!inter_save_flow) {
      Z_sound(msnd4, 128);
      GM_set(NULL);
    }
  } else if (lastkey & KEY_B) {
    menu_text_sprites.clear();
    menu_cursor_sprites.clear();
    menu_vol_sprites.clear();

    bn::core::update();

    if (!mnu) {
      /* на титуле B — открыть меню; в игре только START открывает меню */
      if (g_st == GS_TITLE) {
        Z_sound(msnd3, 128);
        GM_set(&main_mnu);
      }
    } else if (mnu->type == MSG) {
      Z_sound(msnd4, 128);
      GM_command(mnu->t[1]);
    } else if (inter_save_flow && mnu == &save_mnu) {
      Z_sound(msnd4, 128);
      GM_set(&saveask_msg);
    } else {
      Z_sound(msnd4, 128);
      GM_set(NULL);
    }
  } else if (lastkey & KEY_UP) {
    if (mnu && mnu->type == MENU) {
      if (--mnu->cur < 0) mnu->cur = mnu->n - 1;
      Z_sound(msnd1, 128);
      GM_set(mnu);
    }
  } else if (lastkey & KEY_DOWN) {
    if (mnu && mnu->type == MENU) {
      if (++mnu->cur >= mnu->n) mnu->cur = 0;
      Z_sound(msnd1, 128);
      GM_set(mnu);
    }
  } else if (lastkey & (KEY_LEFT | KEY_RIGHT)) {
    if (mnu && mnu->type == MENU && mnu->t[mnu->cur] >= SVOLM) {
      GM_command(mnu->t[mnu->cur] + ((lastkey & KEY_LEFT) ? 0 : 1));
      if (!movsndt) movsndt = Z_sound((lastkey & KEY_LEFT) ? msnd5 : msnd6, 255);
      GM_set(mnu);
    }
  } else if (lastkey & KEY_A) {
    if (mnu && mnu->type == MSG) {
      Z_sound(msnd3, 128);
      GM_command(mnu->t[0]);
    } else if (mnu && mnu->type == MENU) {
      if (mnu->t[mnu->cur] >= PL1CM) {
        Z_sound(msnd2, 128);
        GM_command(PLCEND);
      } else if (mnu->t[mnu->cur] < SVOLM) {
        Z_sound(msnd2, 128);
        GM_command(mnu->t[mnu->cur]);
      }
    }
  }
  lastkey = 0;
  return (mnu ? 1 : 0);
}

void G_keyf(unsigned k) {
  int i;

  lastkey=k;
  // if(!_2pl || cheat) {
    for(i=0;i<31;++i) keybuf[i]=keybuf[i+1];
    keybuf[31]=k;
  // }
}

void GM_init(void) {
// #ifndef DEMO
//   int i;
//   static char nm[QSND_NUM][7]={
// 	"CYBSIT","KNTDTH","MNPAIN","PEPAIN","SLOP","MANSIT","BOSPN","VILACT",
// 	"PLFALL","BGACT","BGDTH2","POPAIN","SGTATK","VILDTH"
//   };
//   char s[8];
//
//   s[0]='D';s[1]='S';
//   for(i=0;i<QSND_NUM;++i) {
//     memcpy(s+2,nm[i],6);
//     qsnd[i]=F_getresid(s);
//   }
// #endif
  csnd1=Z_getsnd("HAHA1");
  csnd2=Z_getsnd("RADIO");
  msnd1=Z_getsnd("PSTOP");
  msnd2=Z_getsnd("PISTOL");
  msnd3=Z_getsnd("SWTCHN");
  msnd4=Z_getsnd("SWTCHX");
  msnd5=Z_getsnd("SUDI");
  msnd6=Z_getsnd("TUDI");
  S_apply_volume();
  // TODO: vga images.
//   msklh[0]=M_lock(F_getresid("M_SKULL1"));
// //  msklh[0]=load_vga("vga\\spr.vga","M_SKULL1");
//   msklh[1]=M_lock(F_getresid("M_SKULL2"));
//   mbarl=M_lock(F_getresid("M_THERML"));
//   mbarm=M_lock(F_getresid("M_THERMM"));
//   mbarr=M_lock(F_getresid("M_THERMR"));
//   mbaro=M_lock(F_getresid("M_THERMO"));
//   mslotl=M_lock(F_getresid("M_LSLEFT"));
//   mslotm=M_lock(F_getresid("M_LSCNTR"));
//   mslotr=M_lock(F_getresid("M_LSRGHT"));
  // K_setkeyproc(G_keyf);
}

inline void set_cursor_frame() {
  if (!menu_cursor_sprites.empty()) {
    const int cursor_frame = (gm_tm / 6) & 1;
    for (auto& sprite : menu_cursor_sprites) {
      sprite.set_visible(false);
    }
    if (mnu->type == MENU) {
      menu_cursor_sprites.at(cursor_frame).set_visible(true);
    }
  }
}

int GM_draw(void) {
  int i, y, menu_x, base_y;

  ++gm_tm;
  if (!mnu && !gm_redraw) return 0;
  if (!mnu) {
    gm_redraw = 0;
    inter_stats_drawn = false;
    menu_text_sprites.clear();
    menu_cursor_sprites.clear();
    menu_vol_sprites.clear();
    return 1;
  }
  if (!gm_redraw) {
    set_cursor_frame();
    return 1;
  }
  gm_redraw = 0;
  inter_text_sprites.clear();
  menu_text_sprites.clear();
  menu_vol_sprites.clear();

  // bn::core::update();
  // if (!text_stbf) return 1;

  text_stbf->set_left_alignment();

  // Coordinate system: (0,0) at screen center. 240x160 → x: -120..119, y: -80..79
  menu_x = -120 + 32;  // 32 px from left edge
  base_y = -(12 + mnu->n * 8);  // vertically centered
  if (base_y < -72) base_y = -72;

  if (mnu->type == MENU) {
    // Title
    text_stbf->generate(menu_x, base_y, mnu->ttl, menu_text_sprites);

    // Menu items
    for (i = 0; i < mnu->n; ++i) {
      int item_y = base_y + 24 + i * 16;
      if (mnu->t[i] == LOAD || mnu->t[i] == SAVE) {
        constexpr int frame_w = 32;
        menu_vol_sprites.push_back(
            bn::sprite_items::m_lsbar.create_sprite(menu_x + 16, item_y, 0));
        for (int f = 1; f <= 4; ++f) {
          menu_vol_sprites.push_back(
              bn::sprite_items::m_lsbar.create_sprite(menu_x + 16 + f * frame_w, item_y, 1));
        }
        menu_vol_sprites.push_back(
            bn::sprite_items::m_lsbar.create_sprite(menu_x + 16 + 5 * frame_w, item_y, 2));
        if (SV_slot_used(i)) {
          text_stcfn->set_left_alignment();
          text_stcfn->generate(menu_x + 4, item_y + 1, SV_slot_name(i), menu_text_sprites);
        }
      } else {
        const bool slider_item = (mnu->t[i] == SVOLM || mnu->t[i] == MVOLM || mnu->t[i] == GAMMAM);
        int item_x = menu_x;
        if (slider_item) {
          constexpr int bar_w = 96;
          constexpr int bar_inner = 80;
          const int bar_left = menu_x;
          int vol = 0;
          switch (mnu->t[i]) {
            case SVOLM: vol = snd_vol; break;
            case MVOLM: vol = mus_vol; break;
            case GAMMAM: vol = pal_level; break;
          }
          if (vol < 0) vol = 0;
          if (vol > 128) vol = 128;
          const int knob_x = bar_left + 8 + vol * bar_inner / 128;
          menu_vol_sprites.push_back(
              bn::sprite_items::m_volbar.create_sprite(bar_left + 16, item_y, 0));
          menu_vol_sprites.push_back(
              bn::sprite_items::m_volbar.create_sprite(bar_left + 48, item_y, 1));
          menu_vol_sprites.push_back(
              bn::sprite_items::m_volbar.create_sprite(bar_left + 80, item_y, 2));
          menu_vol_sprites.push_back(
              bn::sprite_items::m_volknob.create_sprite(knob_x, item_y));
          item_x = bar_left + bar_w + 8;
        }
        if (mnu->m && mnu->m[i] && mnu->m[i][0]) {
          // TODO: player color (PL1CM+) — color sprite
          text_stbf->generate(item_x, item_y, mnu->m[i], menu_text_sprites);
        }
      }
    }
    // Cursor for selected item (temporary: keys sprite; original used skull msklh)
    const int cursor_x = menu_x - 16;
    const int cursor_y = base_y + 24 + mnu->cur * 16 - 4;
    // V_spr(mnu->x-25,y+mnu->cur*16+20-8,msklh[(gm_tm/6)&1]);

    if (menu_cursor_sprites.empty()) {
      menu_cursor_sprites.push_back(bn::sprite_items::m_skull.create_sprite(0, -128, 0));
      menu_cursor_sprites.push_back(bn::sprite_items::m_skull.create_sprite(0, -128, 1));
    }
    for (auto& sprite : menu_cursor_sprites) {
      sprite.set_position(cursor_x, cursor_y);
    }
    set_cursor_frame();
  } else {
    // MSG: Yes/No dialog — original centered STCFN title + (Y/N)
    text_stcfn->set_left_alignment();
    const int title_w = text_stcfn->width(mnu->ttl);
    text_stcfn->generate(-title_w / 2, -12, mnu->ttl, menu_text_sprites);
    const int yes_w = text_stcfn->width("A: ДА");
    text_stcfn->generate(-yes_w / 2, 8, "A: ДА", menu_text_sprites);
    const int no_w = text_stcfn->width("B: НЕТ");
    text_stcfn->generate(-no_w / 2, 20, "B: НЕТ", menu_text_sprites);
    set_cursor_frame();
  }
  return 1;
}
