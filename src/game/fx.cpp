#include "glob.h"
// #include <io.h>
// //#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "vga.h"
// #include "error.h"
#include "view.h"
#include "fx.h"
#include "misc.h"
#include "renderer.h"

enum{NONE,TFOG,IFOG,BUBL};

typedef struct{
  int x,y,xv,yv;
  char t,s;
}fx_t;

BN_DATA_EWRAM static void *spr[15];
BN_DATA_EWRAM static pcm_ref_t bsnd[2];
BN_DATA_EWRAM static char sprd[15];
BN_DATA_EWRAM static fx_t fx[MAXFX];
static char bubsn;
static int last;

// unsigned char fx_scr1[64000],fx_scr2[64000];

#define SINP 256
#define SINPM (SINP-1)
#define W (SINP/100)

static int stdsin[SINP]={
  #include "fx1sin.dat"
};
static int sintab[SINP];

static unsigned char dmap[32*32];

static int isin(int a) {
  return sintab[a&SINPM];
}

static void setamp(int a) {
  int i;

  for(i=0;i<SINP;++i) sintab[i]=stdsin[i]*a;
}

// void FX_trans1(int t) {
//   int x,y,u,v;
//   static unsigned char k;
//   unsigned p;
//
//   setamp(t);
// //  k=(t>=32)?(t*2-64):0;
//   k=t;
//   t*=W;
//   for(y=0,p=0;y<200;++y)
//     for(x=0;x<320;++x,++p) {
//       if(dmap[(y&31)*32+(x&31)]>=k) {
//         u=x+((isin(y+t))>>16);
//         v=y+((isin(u+t))>>16);
//         if(u<0 || u>=320 || v<0 || v>=200) scra[p]=0;
//         else scra[p]=fx_scr1[v*320+u];
//       }else scra[p]=fx_scr2[p];
//     }
// }

// static void init_fx1sin(void) {
//   int j,r,l,rr;
//   unsigned i;
//
//   memset(dmap,0,32*32);
//   for(i=1,rr=32*32;i<64;++i) {
//     for(l=32*32/64;l;--l,--rr) {
//       r=rand()%rr;
//       for(j=0;r;--r,++j) {
//         for(;dmap[j];++j);
//       }
//       for(;dmap[j];++j);
//       dmap[j]=i;
//     }
//   }
// }

// void FX_savegame(int h) {
//   int i,n;
//
//   for(i=n=0;i<MAXFX;++i) if(fx[i].t) ++n;
//   write(h,&n,4);
// //  printf("FX: %d\n",n);
//   for(i=0;i<MAXFX;++i) if(fx[i].t) write(h,&fx[i],sizeof(fx_t));
// }

// void FX_loadgame(int h) {
//   int n;
//
//   read(h,&n,4);
// //  printf("FX: %d\n",n);
//   read(h,fx,n*sizeof(fx_t));
// }

void FX_alloc(void) {
  int i;

//  logo("  effects");
  for(i=0;i<10;++i) spr[i]=Z_getspr("TFOG",i,0,sprd+i);
  for(;i<15;++i) spr[i]=Z_getspr("IFOG",i-10,0,sprd+i);
  bsnd[0]=Z_getsnd("BUBL1");
  bsnd[1]=Z_getsnd("BUBL2");
  // init_fx1sin(); // TODO: or not todo
}

void FX_init(void) {
  int i;

  for(i=0;i<MAXFX;++i) fx[i].t=0;
  bubsn=0;
  last=0;
}

void FX_act(void) {
  int i;
  unsigned char b;

  bubsn=0;
  for(i=0;i<MAXFX;++i) switch(fx[i].t) {
    case TFOG:
      if(++fx[i].s>=20) fx[i].t=0;
      break;
    case IFOG:
      if(++fx[i].s>=10) fx[i].t=0;
      break;
    case BUBL:
      fx[i].yv-=5;
      fx[i].xv=Z_dec(fx[i].xv,20);
      fx[i].x+=fx[i].xv;
      fx[i].y+=fx[i].yv;
      if((b=fld[fx[i].y>>11][fx[i].x>>11]) < 5 || b>7) fx[i].t=0;
      break;
  }
}

void FX_draw(void) {
  int i,s;

  for(i=0;i<MAXFX;++i) {
    s=-1;
    switch(fx[i].t) {
      case TFOG:
        s=fx[i].s/2;
        break;
	  case IFOG:
        s=fx[i].s/2+10;
        break;
	  case BUBL:
        if(R_obj_in_view(fx[i].x>>8,fx[i].y>>8,0,0))
          V_dot((fx[i].x>>8)-w_x_draw+WD/2,(fx[i].y>>8)-w_y+HT/2+1+w_o,fx[i].s+0xC0);
		  continue;
    }
    if(s>=0 && R_obj_in_view(fx[i].x,fx[i].y,8,8))
      Z_drawspr(fx[i].x,fx[i].y,spr[s],sprd[s]);
  }
}

static int findfree(void) {
  int i;

  for(i=0;i<MAXFX;++i) if(!fx[i].t) return i;
  for(i=0;i<MAXFX;++i) if(fx[i].t==IFOG) return i;
  if(++last>=MAXFX) last=0;
  return last;
}

void FX_tfog(int x,int y) {
  int i;

  i=findfree();
	fx[i].t=TFOG;fx[i].s=0;
	fx[i].x=x;fx[i].y=y;
}

void FX_ifog(int x,int y) {
  int i;

  i=findfree();
    fx[i].t=IFOG;fx[i].s=0;
    fx[i].x=x;fx[i].y=y;
}

void FX_bubble(int x,int y,int xv,int yv,int n) {
  int i;

  if(!bubsn) {Z_sound(bsnd[rand()&1],128);bubsn=1;}
  for(;n>0;--n) {
	i=findfree();
	fx[i].t=BUBL;fx[i].s=rand()&3;
	fx[i].x=(x<<8)+random(513)-256;fx[i].y=(y<<8)+random(513)-256;
	fx[i].xv=xv;fx[i].yv=yv-random(256)-768;
  }
}
