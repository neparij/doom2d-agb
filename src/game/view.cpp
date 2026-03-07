#include "glob.h"
#include <cstdint>
#include <cstring>
// #include <malloc.h>
// #include "vga.h"
#include "memory.h"
#include "files.h"
// #include "error.h"
#include "view.h"
#include "dots.h"
// #include "smoke.h"
#include "weapons.h"
#include "items.h"
#include "switch.h"
#include "fx.h"
#include "player.h"
#include "monster.h"
#include "misc.h"
#include "map.h"
#include "renderer.h"

#include "bn_assert.h"
#include "bn_log.h"
#include "bn_memory.h"
#include "debug.h"

#define ANIT 5

#define MAXX (FLDW*CELW-WD/2)
#define MAXY (FLDH*CELH-HT/2+1)  /* +1: камера при нижней границе была на пиксель выше — даём опуститься на 1 пкс */
// Параллакс неба: как в main_v6 — размер текстуры RSKY 256, диапазон сдвига (SKY_SIZE - viewport).
#define SKY_SIZE 256
#define CAM_RANGE_W (FLDW*CELW - WD)
#define CAM_RANGE_H (FLDH*CELH - HT)

extern map_block_t blk;

// extern unsigned char clrmap[256*12];
// void V_remap_rect(int,int,int,int,unsigned char *);

// int w_ht=198; // Original viewport height
int w_ht=120;
int w_ht_half=w_ht/2;
unsigned char w_horiz=ON;
static void *horiz=NULL;
static void *horiz_u=NULL; // Shifted by 1 pixel left, for parallax (as we use 16/32 bit DMA)
int w_o,w_x,w_y,w_x_draw,sky_type=1;
BN_DATA_EWRAM void *walp[256];
BN_DATA_EWRAM unsigned int walf[256];
BN_DATA_EWRAM int walh[256];
BN_DATA_EWRAM unsigned char walswp[256];
BN_DATA_EWRAM unsigned char walani[256];
BN_DATA_EWRAM int anih[ANIT][5];
BN_DATA_EWRAM unsigned char anic[ANIT];
BN_DATA_EWRAM unsigned char fldb[FLDH][FLDW];
BN_DATA_EWRAM unsigned char fldf[FLDH][FLDW];
BN_DATA_EWRAM unsigned char fld[FLDH][FLDW];

/** Маска неба: 1 = в ячейке видно небо (нет непрозрачного тайла), 0 = не рисовать небо. */
BN_DATA_EWRAM unsigned char sky_at_map[FLDH][FLDW];

extern int lt_time,lt_type,lt_side,lt_ypos;
extern void *ltn[2][2];

static void getname(int n,char *s) {
  if(walh[n]==-1) {memset(s,0,8);return;}
  if(walh[n]==-2) {
    memcpy(s,"_WATER_",8);                              // Was previoiusly:
    s[7] = (unsigned char)(uintptr_t)walp[n] - 1 + '0'; // memcpy(s,"_WATER_",8);s[7]=(unsigned char)walp[n]-1+'0';
    return;
  }
  F_getresname(s,walh[n]&0x7FFF);
}

static short getani(char *n) {
  if(strnicmp(n,"WALL22_1",8)==0) return 1;
  if(strnicmp(n,"WALL58_1",8)==0) return 2;
  if(strnicmp(n,"W73A_1",8)==0) return 3;
  if(strnicmp(n,"RP2_1",8)==0) return 4;
  return 0;
}

// void W_savegame(int h) {
//   char s[8];
//   int i;
//
//   write(h,&sky_type,4);
//   for(i=1;i<256;++i) {
//     getname(i,s);write(h,s,8);
//   }
//   write(h,walf,sizeof(walf));
//   write(h,walswp,sizeof(walswp));
//   write(h,fldb,FLDW*FLDH);
//   write(h,fld,FLDW*FLDH);
//   write(h,fldf,FLDW*FLDH);
// }
//
// void W_loadgame(int h) {
//   char s[8];
//   int i;
//
//   read(h,&sky_type,4);
//   for(i=1;i<256;++i) {
//     walani[i]=0;
//     read(h,s,8);if(!s[0]) {walh[i]=-1;walp[i]=NULL;continue;}
//     walani[i]=getani(s);
//     if(memicmp(s,"_WATER_",7)==0) {
//       walh[i]=-2;walp[i]=(void*)(s[7]-'0'+1);
//     }else walp[i]=M_lock(walh[i]=F_getresid(s));
//   }
//   read(h,walf,sizeof(walf));
//   for(i=1;i<256;++i) if(walf[i]&1) walh[i]|=0x8000;
//   read(h,walswp,sizeof(walswp));
//   read(h,fldb,FLDW*FLDH);
//   read(h,fld,FLDW*FLDH);
//   read(h,fldf,FLDW*FLDH);
//   strcpy(s,"RSKY1");s[4]=sky_type+'0';
//   M_unlock(horiz);
//   horiz=M_lock(F_getresid(s));
// }
//
void W_adjust(void) {
  if(w_x<WD/2) w_x=WD/2;
  if(w_y<HT/2) w_y=HT/2;
  if(w_x>MAXX) w_x=MAXX;
  if(w_y>MAXY) w_y=MAXY;
}

static inline uint16_t read_le16(const uint8_t* p) {
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

/** Строит sky_at_map по fldb/fldf с учётом размеров текстур (как в main_v6).
 *  Сначала всё = небо видно; для каждой непрозрачной стены затираем все ячейки, которые она перекрывает (по w,h из заголовка текстуры). */
BN_CODE_IWRAM void build_sky_mask(void) {
  BN_LOG("build_sky_mask requested");
  for (int my = 0; my < FLDH; ++my)
    for (int mx = 0; mx < FLDW; ++mx)
      sky_at_map[my][mx] = 1;

  for (int my = 0; my < FLDH; ++my) {
    for (int mx = 0; mx < FLDW; ++mx) {
      unsigned char b = fldb[my][mx];
      if (b == 0 || (walf[b] & 1)) continue;
      const uint8_t* tex = (const uint8_t*)walp[b];
      int w = (int)read_le16(tex + 0), h = (int)read_le16(tex + 2);
      int cx_end = mx + (w - 1) / CELW;
      int cy_end = my + (h - 1) / CELH;
      if (cx_end >= FLDW) cx_end = FLDW - 1;
      if (cy_end >= FLDH) cy_end = FLDH - 1;
      for (int cy = my; cy <= cy_end; ++cy)
        for (int cx = mx; cx <= cx_end; ++cx)
          sky_at_map[cy][cx] = 0;
    }
  }
  for (int my = 0; my < FLDH; ++my) {
    for (int mx = 0; mx < FLDW; ++mx) {
      unsigned char f = fldf[my][mx];
      if (f == 0 || (walf[f] & 1)) continue;
      const uint8_t* tex = (const uint8_t*)walp[f];
      int w = (int)read_le16(tex + 0), h = (int)read_le16(tex + 2);
      int cx_end = mx + (w - 1) / CELW;
      int cy_end = my + (h - 1) / CELH;
      if (cx_end >= FLDW) cx_end = FLDW - 1;
      if (cy_end >= FLDH) cy_end = FLDH - 1;
      for (int cy = my; cy <= cy_end; ++cy)
        for (int cx = mx; cx <= cx_end; ++cx)
          sky_at_map[cy][cx] = 0;
    }
  }
}

static void Z_drawsky(void) {
  if (w_horiz) {
    const int canvas_off = w_x - w_x_draw;
    const int sky_px = (CAM_RANGE_W > 0 ? -(w_x - WD/2) * (SKY_SIZE - WD) / CAM_RANGE_W : 0) + canvas_off;
    const int sky_py = w_o - (_2pl ? 64 : 0) - (CAM_RANGE_H > 0 ? (w_y - w_ht/2) * (SKY_SIZE - HT) / CAM_RANGE_H : 0);
  	// even/odd check
  	if (sky_px & 1) {
	  V_pic_tiled_masked(horiz_u, sky_px+1, sky_py, (const unsigned char*)sky_at_map);
	} else {
	  V_pic_tiled_masked(horiz, sky_px, sky_py, (const unsigned char*)sky_at_map);
	}
    if (sky_type == 2 && lt_time < 0) {
      if (!lt_side)
        V_spr(0, w_o + lt_ypos, ltn[lt_type][(lt_time < -5) ? 0 : 1]);
      else
        V_spr2(WD - 1, w_o + lt_ypos, ltn[lt_type][(lt_time < -5) ? 0 : 1]);
    }
  } else {
    V_clr(0, WD, w_o + 1, HT, 0x97);
  }
}

void W_draw(void) {
  W_adjust();
  w_x_draw = w_x & ~3;
  R_set_canvas_offset_x(w_x - w_x_draw);
#ifdef D2D_DEBUG_ENABLE
	V_setrect(0, WD, w_o + 1, HT);
	if (dbg_draw_sky) Z_drawsky();
	if (dbg_draw_fldb) Z_drawfld((unsigned char *)fldb);
	if (dbg_draw_dot) DOT_draw();
	if (dbg_draw_it) IT_draw();
	PL_draw(&pl1);
	if (dbg_draw_mn) MN_draw();
	WP_draw();
	FX_draw();
	if (dbg_draw_fldf) Z_drawfld((unsigned char *)fldf);
#else
  V_setrect(0, WD, w_o + 1, HT);
  Z_drawsky();
   Z_drawfld((unsigned char *)fldb);
   DOT_draw();
   IT_draw();
   PL_draw(&pl1);
//   if(_2pl) PL_draw(&pl2);
   MN_draw();
   WP_draw();
//   SMK_draw();
   FX_draw();
   Z_drawfld((unsigned char *)fldf);
#endif

}

void W_init(void) {
  int i,j;
  static char *anm[ANIT-1][5]={
    {"WALL22_1","WALL23_1","WALL23_2",NULL,NULL},
    {"WALL58_1","WALL58_2","WALL58_3",NULL,NULL},
    {"W73A_1","W73A_2",NULL,NULL,NULL},
    {"RP2_1","RP2_2","RP2_3","RP2_4",NULL}
  };

  for(i=1;i<ANIT;++i) {
    for(j=0;anm[i-1][j];++j)
      anih[i][j]=F_getresid(anm[i-1][j]);
    for(;j<5;++j) anih[i][j]=-1;
  }
  memset(anic,0,sizeof(anic));
  DOT_init();
  // SMK_init(); // TODO: uncomment
  FX_init();
  WP_init();
  IT_init();
  SW_init();
  PL_init();
  MN_init();
  // M_unlock_ewram(horiz, sky_ptr);
  // horiz=M_lock_ewram(F_getresid("RSKY1"), sky_ptr); // TODO: Fast sky
  M_unlock(horiz);
  M_unlock(horiz_u);
  horiz=M_lock(F_getresid("RSKY1"));
  horiz_u=M_lock(F_getresid("RSKYU1"));
}

void W_act(void) {
  int i,a;

  if(g_time%3!=0) return;
  for(i=1;i<256;++i) if((a=walani[i])!=0) {
    if(anih[a][++anic[a]]==-1) anic[a]=0;
    walp[i]=M_lock(anih[a][anic[a]]); // DO not copy to EWRAM!!!
  }
}

static void unpack(const unsigned char* buf, int len, void* obuf) {
  const unsigned char* p = buf;
  unsigned char* o = (unsigned char*)obuf;
  int l = len;

  while (l > 0) {
    if (*p == 255) {
      if (l < 4) break;
      int n = (int)p[1] | ((int)p[2] << 8);
      unsigned char v = p[3];
      p += 4;
      l -= 4;
      while (n--) *o++ = v;
    } else {
      *o++ = *p++;
      --l;
    }
  }
}

int W_load(const unsigned char* p, unsigned size) {
  int i,j,k,g;
  static wall_t w;
  void *dst;
  const unsigned char* cur = p;
  unsigned left = size;

  switch(blk.t) {
	case MB_WALLNAMES:
  	  BN_LOG("W_load: WALLNAMES");
  		// TODO: Check EWRAM consistency!
  	  // for (i=0;i<256;++i) {
  	  	// Unlock EWRAM: p must be the data pointer (walp[i]), not &walh[i]
  	  	// if (walh[i] >= 0) {
  	  	// 	M_unlock_ewram(walp[i], nullptr);
  	  	// }
  	  // }
	  for(i=0;i<256;++i) {walh[i]=-1;walswp[i]=i;walani[i]=0;walf[i]=0;}
	  for(i=1;i<256 && blk.sz>0;++i,blk.sz-=(int)sizeof(w)) {
		if (left < sizeof(w)) BN_ERROR("W_load: WALLNAMES overflow");
		memcpy(&w, cur, sizeof(w)); cur += sizeof(w); left -= sizeof(w);
		if(memicmp(w.n,"_WATER_",7)==0){walp[i]=(void*)(w.n[7]-'0'+1);walh[i]=-2;walf[i]=1;continue;}
		walp[i]=M_lock_ewram(walh[i]=F_getresid(w.n), walp[i]);
		if(w.n[0]=='S' && w.n[1]=='W' && w.n[4]=='_') walswp[i]=0;
		w.t=(char)is_wallname_transparent(w.n);
		walf[i]=(w.t)?1:0; if(w.t) walh[i]|=0x8000;
		if(memicmp(w.n,"VTRAP01",8)==0) walf[i]|=2;
		walani[i]=getani(w.n);
	  }
	  for(j=i,i=1;i<256;++i) if(walswp[i]==0) {
		if(j>=256) break;
		F_getresname(w.n,walh[i]&0x7FFF);
		w.n[5]^=1;
		g=F_getresid(w.n)|(walh[i]&0x8000);
		for(k=1;k<256;++k) if(walh[k]==g) break;
		if(k>=256) {
		  walh[k=j++]=g;walp[k]=M_lock_ewram(g, walp[k]);
		  walf[k]=(g&0x8000)?1:0;
		}
		walswp[i]=k;walswp[k]=i;
	  }
	  return 1;
	case MB_BACK:  BN_LOG("W_load: BACK");dst=fldb;goto unp;
	case MB_WTYPE: BN_LOG("W_load: WTYPE");dst=fld;goto unp;
	case MB_FRONT: BN_LOG("W_load: FRONT");dst=fldf;
	unp: switch(blk.st) {
		BN_LOG("Unpack routine...");
	    case 0:
	      if (left < (unsigned)(FLDW*FLDH)) BN_ERROR("W_load: BACK/WTYPE/FRONT overflow");
	      memcpy(dst, cur, FLDW*FLDH); cur += FLDW*FLDH; left -= FLDW*FLDH;
	      build_sky_mask();
	      break;
	    case 1:
	      if (left < (unsigned)blk.sz) BN_ERROR("W_load: packed block overflow");
	      unpack(cur, blk.sz, dst);
	      cur += blk.sz; left -= blk.sz;
	      build_sky_mask();
	      break;
	    default: return 0;
	  }return 1;
	case MB_SKY:
  	  BN_LOG("W_load: SKY");
	  if (left < 2u) BN_ERROR("W_load: MB_SKY overflow");
	  sky_type=0; memcpy(&sky_type, cur, 2); cur += 2; left -= 2;
	  strcpy(w.n,"RSKY1");w.n[4]=sky_type+'0';
	  // M_unlock_ewram(horiz, sky_ptr);
	  // horiz=M_lock_ewram(F_getresid(w.n), sky_ptr); // TODO: Fast sky
  	  M_unlock(horiz);
  	  M_unlock(horiz_u);
  	  horiz=M_lock(F_getresid(w.n));
  	  // Add sky in format "RSKYU1":
  	  strcpy(w.n,"RSKYU1");w.n[5]=sky_type+'0';
  	  horiz_u=M_lock(F_getresid(w.n));
	  return 1;
  }return 0;
}
