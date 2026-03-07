#include "glob.h"
// //#include <stdio.h>
// #include <stdlib.h>
#include "files.h"
#include "memory.h"
// #include "vga.h"
// #include "error.h"
// #include "keyb.h"
#include "sound.h"
#include "view.h"
#include "dots.h"
#include "misc.h"

#include "renderer.h"
#include "bn_memory.h"

#define MAXINI 50
#define MAXSR 20

#define BL_XV 4
#define BL_YV 4
#define BL_MINT 10
#define BL_MAXT 14

#define SP_V 2
#define SP_MINT 5
#define SP_MAXT 7

extern unsigned char z_dot;

typedef struct{
  obj_t o;
  unsigned char c,t;
}dot_t;

typedef struct{
  int xv,yv;
  unsigned char c,t;
}init_t;

BN_DATA_EWRAM static dot_t dot[MAXDOT];
BN_DATA_EWRAM static init_t bl_ini[MAXINI],sp_ini[MAXINI];
static int bl_r,sp_r,sr_r;
BN_DATA_EWRAM static int sxr[MAXSR],syr[MAXSR];
static int ldot;

// void DOT_savegame(int h) {
//   int i,n;
//
//   for(i=n=0;i<MAXDOT;++i) if(dot[i].t) ++n;
//   write(h,&n,4);
// //  printf("dots: %d\n",n);
//   for(i=0;i<MAXDOT;++i) if(dot[i].t) write(h,&dot[i],sizeof(dot_t));
// }

// void DOT_loadgame(int h) {
//   int n;
//
//   read(h,&n,4);
// //  printf("dots: %d\n",n);
//   read(h,dot,n*sizeof(dot_t));
// //  for(i=0;i<n;++i) printf("dot#%d %u\n",i,(unsigned)dot[i].t);
// }

void DOT_init(void) {
  int i;

  for(i=0;i<MAXDOT;++i) {dot[i].t=0;dot[i].o.r=0;dot[i].o.h=1;}
  ldot=0;
}

static void incldot(void) {
  if(++ldot>=MAXDOT) ldot=0;
}

void DOT_alloc(void) {
  int i;

//  logo("  dots");
  for(i=0;i<MAXINI;++i) {
	bl_ini[i].xv=random(BL_XV*2+1)-BL_XV;
	bl_ini[i].yv=-random(BL_YV);
	bl_ini[i].c=0xB0+random(16);
	bl_ini[i].t=random(BL_MAXT-BL_MINT+1)+BL_MINT;
	sp_ini[i].xv=random(SP_V*2+1)-SP_V;
	sp_ini[i].yv=random(SP_V*2+1)-SP_V;
	sp_ini[i].c=0xA0+random(6);
	sp_ini[i].t=random(SP_MAXT-SP_MINT+1)+SP_MINT;
  }
  for(i=0;i<MAXSR;++i) {
	sxr[i]=random(2*2+1)-2;
	syr[i]=random(2*2+1)-2;
  }
  bl_r=sp_r=sr_r=0;
}

BN_CODE_IWRAM void DOT_act(void) {
  int i,s,xv,yv;

  z_dot=1;
  for(i=0;i<MAXDOT;++i) if(dot[i].t) {
    obj_t *const dobj = &dot[i].o;
    xv=dobj->xv+dobj->vx;
    yv=dobj->yv+dobj->vy;
    s=Z_moveobj(&dot[i].o);
    if(dot[i].t<254) --dot[i].t;
    if(s&(Z_HITWATER|Z_FALLOUT)) {dot[i].t=0;continue;}
    if(s&Z_HITLAND) {
      if(!dobj->xv) {
        if(yv>2) {
          if(!xv) dobj->vx=(rand()&1)?-1:1;
          else dobj->vx=Z_sign(dobj->vx);
          if(rand()%yv==0) dobj->vx*=2;
          dobj->yv=yv-2;
        }
      }
      dobj->xv=0;
      if(dot[i].t>4 && dot[i].t!=255) dot[i].t=4;
    }
    if(s&Z_HITWALL) {
      dobj->vx=Z_sign(xv)*2;
      dobj->yv=Z_sign(dobj->yv);
      if(dobj->yv>=0) if(rand()&3) --dobj->yv;
      if(dobj->yv>=0) if(rand()&1) --dobj->yv;
    }
    if(s&Z_HITCEIL) {dobj->xv=0;dobj->yv=(random(100))?-2:0;}
  }
  z_dot=0;
}

void DOT_draw(void) {
  int i;

  for(i=0;i<MAXDOT;++i)
    if(dot[i].t && R_obj_in_view(dot[i].o.x,dot[i].o.y,0,0))
      V_dot(dot[i].o.x-w_x_draw+WD/2,dot[i].o.y-w_y+HT/2+1+w_o,dot[i].c);
}

void DOT_add(int x,int y,char xv,char yv,unsigned char c,unsigned char t) {
  int i;

  if(!Z_canfit(x,y,0,1)) return;
  i=ldot;
  dot[i].o.x=x;dot[i].o.y=y;
  dot[i].o.xv=xv;dot[i].o.yv=yv;
  dot[i].c=c;dot[i].t=t;
  dot[i].o.vx=dot[i].o.vy=0;
  incldot();
}

void DOT_blood(int x,int y,int xv,int yv,int n) {
  int i,k,dx,dy;

  for(k=n;k;--k) {
    dx=x+sxr[sr_r];dy=y+syr[sr_r];
    if(!Z_canfit(x,y,0,1)) continue;
    i=ldot;
	dot[i].o.x=dx;dot[i].o.y=dy;
	dot[i].o.xv=bl_ini[bl_r].xv+Z_dec(xv,3);
	dot[i].o.yv=bl_ini[bl_r].yv+Z_dec(yv,3)-3;
	dot[i].c=bl_ini[bl_r].c;
	dot[i].t=255;
	dot[i].o.vx=dot[i].o.vy=0;
	if(++bl_r>=MAXINI) bl_r=0;
	if(++sr_r>=MAXSR) sr_r=0;
    incldot();
  }
}

void DOT_spark(int x,int y,int xv,int yv,int n) {
  int i,k,dx,dy;

  for(k=n;k;--k) {
    dx=x+sxr[sr_r];dy=y+syr[sr_r];
    if(!Z_canfit(x,y,0,1)) continue;
    i=ldot;
	dot[i].o.x=dx;dot[i].o.y=dy;
	dot[i].o.xv=sp_ini[sp_r].xv-xv/4;
	dot[i].o.yv=sp_ini[sp_r].yv-yv/4;
	dot[i].c=sp_ini[sp_r].c;
	dot[i].t=sp_ini[sp_r].t;
	dot[i].o.vx=dot[i].o.vy=0;
	if(++sp_r>=MAXINI) sp_r=0;
	if(++sr_r>=MAXSR) sr_r=0;
    incldot();
  }
}

void DOT_water(int x,int y,int xv,int yv,int n,int c) {
  int i,k,dx,dy;
  static unsigned char ct[3]={0xC0,0x70,0xB0};

  if(c<0 || c>=3) return;
  c=ct[c];
  for(k=n;k;--k) {
    dx=x+sxr[sr_r];dy=y+syr[sr_r];
    if(!Z_canfit(x,y,0,1)) continue;
    i=ldot;
	dot[i].o.x=dx;dot[i].o.y=dy;
	dot[i].o.xv=bl_ini[bl_r].xv-Z_dec(xv,3);
	dot[i].o.yv=bl_ini[bl_r].yv-abs(yv);
	dot[i].c=bl_ini[bl_r].c-0xB0+c;
	dot[i].t=254;
	dot[i].o.vx=dot[i].o.vy=0;
	if(++bl_r>=MAXINI) bl_r=0;
	if(++sr_r>=MAXSR) sr_r=0;
    incldot();
  }
}
