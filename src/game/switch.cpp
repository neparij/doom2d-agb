#include "glob.h"
#include <string.h>
#include "files.h"
#include "view.h"
#include "bmap.h"
#include "switch.h"

#include "bn_log.h"
#include "player.h"
#include "misc.h"
#include "map.h"

#ifndef BN_CODE_IWRAM
#define BN_CODE_IWRAM __attribute__((section(".iwram")))
#endif

#define MAXSW 100
#define SW_REGION_SIZE 10
#define SW_REGIONS_X (FLDW/SW_REGION_SIZE)
#define SW_REGIONS_Y (FLDH/SW_REGION_SIZE)
#define SW_REGIONS (SW_REGIONS_X*SW_REGIONS_Y)

extern map_block_t blk;

typedef struct{
  unsigned char x,y;
  unsigned char t,tm;
  unsigned char a,b,c,d;
  unsigned char f;
}sw_t;

BN_DATA_EWRAM static sw_t sw[MAXSW];

static void SW_build_region_index(void);

/* Пространственный индекс в EWRAM (не IWRAM). region_start[r+1]-region_start[r] = кол-во переключателей в регионе r */
BN_DATA_EWRAM static int sw_region_start[SW_REGIONS+1];
BN_DATA_EWRAM static int sw_region_list[MAXSW];

static pcm_ref_t sndswn,sndswx,sndnoway,sndbdo,sndbdc,sndnotele;
static int swsnd;

int sw_secrets;

// void SW_savegame(int h) {
//   int n;
//
//   for(n=MAXSW;--n;) if(sw[n].t) break;
//   ++n;write(h,&n,4);write(h,sw,n*sizeof(sw[0]));
//   write(h,&sw_secrets,4);
// }
//
// void SW_loadgame(int h) {
//   int n;
//
//   read(h,&n,4);read(h,sw,n*sizeof(sw[0]));
//   read(h,&sw_secrets,4);
// }

int SW_load(const unsigned char* p, unsigned size) {
  int i;
  const unsigned char* cur = p;
  unsigned left = size;

  switch (blk.t) {
    case MB_SWITCH2:
      BN_LOG("SW_load: MB_SWITCH2");
  	  sw_secrets=0;
      for (i = 0; i < MAXSW && left >= sizeof(sw_t); ++i) {
      	memcpy(&sw[i], cur, sizeof(sw_t));
      	cur += sizeof(sw_t);
      	left -= sizeof(sw_t);
      	sw[i].tm=0;sw[i].d=0;
      	sw[i].f|=0x80;
      	if(sw[i].t==SW_SECRET) ++sw_secrets;
      }
      SW_build_region_index();
      return 1;
  }
  return 0;
}

void SW_alloc(void) {
//  logo("  switches");
  sndswn=Z_getsnd("SWTCHN");
  sndswx=Z_getsnd("SWTCHX");
  sndnoway=Z_getsnd("NOWAY");
  sndbdo=Z_getsnd("BDOPN");
  sndbdc=Z_getsnd("BDCLS");
  sndnotele=Z_getsnd("NOTELE");
}

static void SW_build_region_index(void) {
  int i,r,region_count[SW_REGIONS];

  for(r=0;r<SW_REGIONS;++r) region_count[r]=0;
  for(i=0;i<MAXSW;++i) if(sw[i].t) {
    r=(sw[i].y/SW_REGION_SIZE)*SW_REGIONS_X+(sw[i].x/SW_REGION_SIZE);
    if(r>=0 && r<SW_REGIONS) ++region_count[r];
  }
  sw_region_start[0]=0;
  for(r=0;r<SW_REGIONS;++r) sw_region_start[r+1]=sw_region_start[r]+region_count[r];
  for(r=0;r<SW_REGIONS;++r) region_count[r]=0;
  for(i=0;i<MAXSW;++i) if(sw[i].t) {
    r=(sw[i].y/SW_REGION_SIZE)*SW_REGIONS_X+(sw[i].x/SW_REGION_SIZE);
    if(r>=0 && r<SW_REGIONS)
      sw_region_list[sw_region_start[r]+region_count[r]++]=i;
  }
}

void SW_init(void) {
  int i;

  for(i=0;i<MAXSW;++i) sw[i].t=0;
  swsnd=0;
  sw_region_start[0]=0;
  for(i=0;i<SW_REGIONS;++i) sw_region_start[i+1]=0;
}

static unsigned char cht,chto,chf,f_ch;
static bool need_sky_mask_rebuild;

static void door(unsigned char x,unsigned char y) {
  unsigned char ex;

  if(x>=FLDW || y>=FLDH) return;
  if(fld[y][x]!=cht) return;
  ex=x+1;
  for(;x && fld[y][x-1]==cht;--x);
  for(;ex<FLDW && fld[y][ex]==cht;++ex);
  memset(fld[y]+x,chto,ex-x);
  if(f_ch) { memset(fldf[y]+x,chf,ex-x); need_sky_mask_rebuild = true; }
  for(;x<ex;++x) {
	door(x,y-1);
	door(x,y+1);
  }
}

/*
static void door(unsigned char x,unsigned char y) {
  if(x>=FLDW || y>=FLDH) return;
  if(fld[y][x]!=cht) return;
  fld[y][x]=chto;fldf[y][x]=chf;
  door(x-1,y);
  door(x+1,y);
  door(x,y-1);
  door(x,y+1);
}
*/

void Z_water_trap(obj_t *o) {
  int i,j,sx,sy,x,y;

  if((y=o->y)>=FLDH*CELH+o->h) return;
  if((x=o->x)<0 || o->x>FLDW*CELW) return;
  sx=(x-o->r)>>3;
  sy=(y-o->h+1)>>3;
  x=(x+o->r)>>3;
  y=(y-o->h/2)>>3;
  for(i=sx;i<=x;++i)
	for(j=sy;j<=y;++j)
	  if(fld[j][i]==5) {
		cht=5;chto=255;f_ch=0;
		door(i,j);
	  }
  if (need_sky_mask_rebuild) {
    build_sky_mask();
    need_sky_mask_rebuild = false;
  }
}

void Z_untrap(unsigned char t) {
  unsigned char *p;
  unsigned short n;

  for(p=(unsigned char*)fld,n=FLDW*FLDH;n;--n,++p)
	if(*p==255) *p=t;
}

static void opendoor(int i) {
  int j;

  swsnd=Z_sound(sndbdo,128);
  j=fldf[sw[i].b][sw[i].a];
  cht=2;chto=3;chf=0;f_ch=1;
  door(sw[i].a,sw[i].b);
  fldf[sw[i].b][sw[i].a]=j;
  need_sky_mask_rebuild = true;
  fld_need_remap=1;
}

static int shutdoor(int i) {
  int j;

  cht=3;chto=255;chf=fldf[sw[i].b][sw[i].a];f_ch=1;
  door(sw[i].a,sw[i].b);
  cht=255;
  if(Z_chktrap(0,0,-3,HIT_SOME)) {
	j=fldf[sw[i].b][sw[i].a];
	chto=3;chf=0;f_ch=1;
	door(sw[i].a,sw[i].b);
	fldf[sw[i].b][sw[i].a]=j;
	return 0;
  }
  chto=2;
  door(sw[i].a,sw[i].b);
  fld_need_remap=1;
  swsnd=Z_sound(sndbdc,128);
  return 1;
}

void SW_act(void) {
  int i;

  if(swsnd) --swsnd;
  for(i=0;i<MAXSW;++i) if(sw[i].t) {
    if(sw[i].tm) --sw[i].tm;
    switch(sw[i].t) {
      case SW_DOOR5: case SW_DOOR: case SW_SHUTDOOR:
        if(!sw[i].d) break;
        if(fld[sw[i].b][sw[i].a]!=3) {sw[i].d=0;break;}
        if(--sw[i].d==0) if(!shutdoor(i)) sw[i].d=9;
        break;
      case SW_TRAP:
        if(!sw[i].d) break;
        if(fld[sw[i].b][sw[i].a]!=2) {sw[i].d=0;break;}
        if(--sw[i].d==0) {opendoor(i);sw[i].tm=18;}
        break;
    }
  }
}

static int doortime(int t) {
  switch(t) {
	case SW_DOOR5:		return 90;
  }
  return 0;
}

void SW_cheat_open(void) {
  int i;

  for(i=0;i<MAXSW;++i) if(sw[i].t && !sw[i].tm) switch(sw[i].t) {
	case SW_DOOR: case SW_DOOR5:
	case SW_OPENDOOR:
	  if(fld[sw[i].b][sw[i].a]!=2) break;
	  SW_press(sw[i].x*CELW+4,sw[i].y*CELH+4,1,1,0xFF,-3);
	  break;
  }
}

int SW_press(int x,int y,int r,int h,unsigned char t,int o) {
  int sx,sy,ex,ey,rx,ry,rx0,rx1,ry0,ry1,ri,i,p;

  sx=(x-r)>>3;sy=(y-h+1)>>3;
  ex=(x+r)>>3;ey=y/CELH;
  rx0=sx/SW_REGION_SIZE; if(rx0<0) rx0=0; if(rx0>=SW_REGIONS_X) rx0=SW_REGIONS_X-1;
  rx1=ex/SW_REGION_SIZE; if(rx1<0) rx1=0; if(rx1>=SW_REGIONS_X) rx1=SW_REGIONS_X-1;
  ry0=sy/SW_REGION_SIZE; if(ry0<0) ry0=0; if(ry0>=SW_REGIONS_Y) ry0=SW_REGIONS_Y-1;
  ry1=ey/SW_REGION_SIZE; if(ry1<0) ry1=0; if(ry1>=SW_REGIONS_Y) ry1=SW_REGIONS_Y-1;
  for(p=0,ry=ry0;ry<=ry1;++ry) for(rx=rx0;rx<=rx1;++rx) {
    ri=ry*SW_REGIONS_X+rx;
    for(i=sw_region_start[ri];i<sw_region_start[ri+1];++i) {
      int si=sw_region_list[i];
      if(!sw[si].tm && sw[si].x>=sx && sw[si].x<=ex && sw[si].y>=sy && sw[si].y<=ey && ((sw[si].f&0x8F)&t)) {
	if(sw[si].f&0x70) if((sw[si].f&(t&0x70))!=(sw[si].f&0x70)) continue;
      switch(sw[si].t) {
		case SW_EXIT:
		  g_exit=1;sw[si].tm=9;swsnd=Z_sound(sndswx,128);break;
		case SW_EXITS:
		  g_exit=2;sw[si].tm=9;swsnd=Z_sound(sndswx,128);break;
		case SW_DOOR: case SW_DOOR5:
		  switch(fld[sw[si].b][sw[si].a]) {
			case 2:
			  opendoor(si);sw[si].tm=9;sw[si].d=doortime(sw[si].t);break;
			case 3:
			  if(shutdoor(si)) {sw[si].tm=9;sw[si].d=0;}
			  else {
			    if(!swsnd) swsnd=Z_sound(sndnoway,128);
			    sw[si].d=2;
			  }break;
		  }break;
		case SW_PRESS:
		  sw[si].tm=9;
		  SW_press((unsigned int)sw[si].a*8+4,(unsigned int)sw[si].b*8+12,8,16,(t&0x70)|0x80,o);
		  break;
		case SW_TELE:
		  if(o < -2) break;
		  if(!Z_canfit((unsigned int)sw[si].a*8+4,(unsigned int)sw[si].b*8+7,r,h)) {
		    if(!swsnd) swsnd=Z_sound(sndnotele,128);
		    break;
		  }
      		Z_teleobj(o,(unsigned int)sw[si].a*8+4,(unsigned int)sw[si].b*8+7);
		  sw[si].tm=1;
		  break;
		case SW_OPENDOOR:
		  if(fld[sw[si].b][sw[si].a]!=2) break;
		  opendoor(si);
		  sw[si].tm=1;
		  break;
		case SW_SHUTDOOR:
		  if(fld[sw[si].b][sw[si].a]!=3) break;
		  if (shutdoor(si)) {
			  sw[si].tm = 1;
			  sw[si].d = 0;
		  } else {
			  if (!swsnd) swsnd = Z_sound(sndnoway, 128);
			  sw[si].d = 2;
		  }
		  break;
		case SW_SHUTTRAP: case SW_TRAP:
		  if(fld[sw[si].b][sw[si].a]!=3) break;
		  cht=3;chto=255;chf=fldf[sw[si].b][sw[si].a];f_ch=1;
		  door(sw[si].a,sw[si].b);
		  Z_chktrap(1,100,-3,HIT_TRAP);
		  cht=255;chto=2;
		  door(sw[si].a,sw[si].b);
		  fld_need_remap=1;
		  swsnd=Z_sound(sndswn,128);
		  sw[si].tm=1;sw[si].d=20;
		  break;
		case SW_LIFT:
		  if(fld[sw[si].b][sw[si].a]==10) {
		    cht=10;chto=9;f_ch=0;
		  }else if(fld[sw[si].b][sw[si].a]==9) {
		    cht=9;chto=10;f_ch=0;
		  }else break;
		  door(sw[si].a,sw[si].b);
		  fld_need_remap=1;
		  swsnd=Z_sound(sndswx,128);
		  sw[si].tm=9;
		  break;
		case SW_LIFTUP:
		  if(fld[sw[si].b][sw[si].a]!=10) break;
		  cht=10;chto=9;f_ch=0;
		  door(sw[si].a,sw[si].b);
		  fld_need_remap=1;
		  swsnd=Z_sound(sndswx,128);
		  sw[si].tm=1;
		  break;
		case SW_LIFTDOWN:
		  if(fld[sw[si].b][sw[si].a]!=9) break;
		  cht=9;chto=10;f_ch=0;
		  door(sw[si].a,sw[si].b);
		  fld_need_remap=1;
		  swsnd=Z_sound(sndswx,128);
		  sw[si].tm=1;
		  break;
		case SW_SECRET:
		  if(o!=-1 && o!=-2) break;
		  if(o==-1) ++pl1.secrets;
		  else ++pl2.secrets;
		  sw[si].tm=1;sw[si].t=0;break;
      }
      if(sw[si].tm) {
        fldb[sw[si].y][sw[si].x]=walswp[fldb[sw[si].y][sw[si].x]]; p=1;
        need_sky_mask_rebuild = true;
      }
      if(sw[si].tm==1) sw[si].tm=0;
      }
    }
  }
  if (need_sky_mask_rebuild) {
    build_sky_mask();
    need_sky_mask_rebuild = false;
  }
  return p;
}
