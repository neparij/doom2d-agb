#include "glob.h"
// #include <stdio.h>
// #include <stdarg.h>
// #include <string.h>
#include <cstring>
// #include <stdlib.h>
#include "bn_memory.h"
#include "files.h"
#include "memory.h"
// #include "error.h"
// #include "vga.h"
#include "sound.h"
#include "view.h"
#include "bmap.h"
#include "dots.h"
#include "monster.h"
#include "misc.h"

#include "bn_log.h"
#include "bn_math.h"
#include "bn_sound_items.h"
#include "renderer.h"

static unsigned s_rnd_state = 1;

unsigned rnd_max(unsigned n) {
  if (n == 0) return 0;
  s_rnd_state = s_rnd_state * 1103515245u + 12345u;
  return s_rnd_state % n;
}

#define MAX_YV 30

#define MAXAIR 1091

extern unsigned int walf[256];

unsigned char z_dot=0;

// void V_manspr(int,int,vgaimg *,char);
// void V_manspr2(int,int,vgaimg *,char);

extern void *walp[256];

// static void *sth[22],*bfh[160-'!'],*sfh[160-'!'],*bulsnd[2],*stone[2],*keys[3];
// static int prx=0,pry=0;
BN_DATA_EWRAM static pcm_ref_t bulsnd[2];

int Z_sign(int a) {
  if(a>0) return 1;
  if(a<0) return -1;
  return 0;
}

BN_CODE_IWRAM int Z_dec(int a,int b) {
  if(bn::abs(a)<=b) return 0;
  if(a>0) return a-b;
  if(a<0) return a+b;
  return 0;
}

void *Z_getspr(char n[4],int s,int d,char *dir) {
  int h;

  h=F_getsprid(n,s,d);
  if(dir) *dir=(h&0x8000)?1:0;
  return M_lock(h);
}

void *Z_getspr_ewram(char n[4],int s,int d,char *dir, void* ewram_ptr) {
	int h;

	h=F_getsprid(n,s,d);
	if(dir) *dir=(h&0x8000)?1:0;
	return M_lock_ewram(h, ewram_ptr);
}

pcm_ref_t Z_getsnd(char n[6]) {
  char s[9];
  s[0]='D';
  s[1]='S';
  strncpy(s+2,n,6);
  s[8]='\0';
  return get_pcm_ref(s);
}

int Z_sound(pcm_ref_t& s,int v) {
   if(snd_type==-1) return 0;
   if(s.length == 0) return 0;

   S_play(*s.item, -1, 1024, v);
   return s.length;
}

#define GAS_START (MN__LAST-MN_DEMON+5)
#define GAS_TOTAL (MN__LAST-MN_DEMON+16+10)

void Z_initst(void) {
//   int i;
//   char s[10];
//   static char nm[22][8]={
// 	"STTNUM0","STTNUM1","STTNUM2","STTNUM3","STTNUM4",
// 	"STTNUM5","STTNUM6","STTNUM7","STTNUM8","STTNUM9",
// 	"STTMINUS","STTPRCNT",
// 	"FISTA0","CSAWA0","PISTA0","SHOTA0","SGN2A0","MGUNA0","LAUNA0",
// 	"PLASA0","BFUGA0","GUN2A0"
//   };
//
// //  logo("  status");
//   stone[0]=M_lock(F_getresid("STONE"));
//   stone[1]=M_lock(F_getresid("STONE2"));
//   keys[0]=M_lock(F_getresid("KEYRA0"));
//   keys[1]=M_lock(F_getresid("KEYGA0"));
//   keys[2]=M_lock(F_getresid("KEYBA0"));
//   for(i=0;i<22;++i)
//     sth[i]=M_lock(F_getresid(nm[i]));
//   strcpy(s,"STBF_*");
//   for(i='!';i<160;++i) {
// 	s[5]=i;
// 	bfh[i-'!']=M_lock(F_findres(s));
// 	if(!(i&15)) logo_gas(GAS_START+((i-'!')>>4),GAS_TOTAL);
//   }
//   for(i='!';i<160;++i) {
// 	sprintf(s,"STCFN%03d",i);
// 	sfh[i-'!']=M_lock(F_findres(s));
// 	if(!(i&15)) logo_gas(GAS_START+8+((i-'!')>>4),GAS_TOTAL);
//   }
//   strcpy(s,"WINUM*");
//   for(i='0';i<='9';++i) {
// 	s[5]=i;
// 	bfh[i-'!']=M_lock(F_getresid(s));
//   }
//   bfh[':'-'!']=M_lock(F_getresid("WICOLON"));
   bulsnd[0]=Z_getsnd("BUL1");
   bulsnd[1]=Z_getsnd("BUL2");
}
//
// void Z_putbfch(int c) {
//   vgaimg *p;
//
//   if(c>32 && c<160) p=bfh[c-'!']; else p=NULL;
//   if(p) {
//     V_spr(prx,pry,p);
//     prx+=p->w-1;
//   }else prx+=12;
// }
//
// void Z_putsfch(int c) {
//   vgaimg *p;
//
//   if(c>32 && c<160) p=sfh[c-'!']; else p=NULL;
//   if(p) {
//     V_spr(prx,pry,p);
//     prx+=p->w-1;
//   }else prx+=7;
// }
//
// void Z_gotoxy(int x,int y) {prx=x;pry=y;}
//
// void Z_printbf(char *s,...) {
//   int i;
//   va_list ap;
//   char buf[80];
//
//   va_start(ap,s);
//   vsprintf(buf,s,ap);
//   va_end(ap);
//   for(i=0;buf[i];++i) switch(buf[i]) {
// 	case '\n':
// 	  pry+=13;
// 	case '\r':
// 	  prx=0;break;
// 	default:
// 	  Z_putbfch((unsigned char)buf[i]);
//   }
// }
//
// void Z_printsf(char *s,...) {
//   int i;
//   va_list ap;
//   char buf[80];
//
//   va_start(ap,s);
//   vsprintf(buf,s,ap);
//   va_end(ap);
//   for(i=0;buf[i];++i) switch(buf[i]) {
// 	case '\n':
// 	  pry+=8;
// 	case '\r':
// 	  prx=0;break;
// 	default:
// 	  Z_putsfch((unsigned char)buf[i]);
//   }
// }
//
void Z_drawspr(int x,int y,void *p,char d) {
	if (d) V_spr2(x-w_x_draw+WD/2,y-w_y+HT/2+1+w_o,p);
	else V_spr(x-w_x_draw+WD/2,y-w_y+HT/2+1+w_o,p);
//   if(d) V_spr2(x-w_x+100,y-w_y+HT/2+1+w_o,p);
//     else V_spr(x-w_x+100,y-w_y+HT/2+1+w_o,p);
}
//
// void Z_clrst(void) {
//   V_pic(200,w_o,stone[0]);
//   if(HT>100) V_pic(200,w_o+100,stone[1]);
// }
//
// void Z_drawstlives(char n) {
// //  V_setrect(280,30,w_o,40);Z_clrst();
// //  V_spr(285,w_o+17,sth[n]);
// }
//
// void Z_drawstkeys(unsigned char k) {
//   int x,n;
//
//   V_setrect(200,70,w_o+77,23);Z_clrst();
//   for(k>>=4,n=0,x=245;n<3;++n,k>>=1,x+=9)
//     if(k&1) V_spr(x,w_o+91,keys[n]);
// }
//
// void Z_drawstair(int a) {
//   V_setrect(200,120,w_o+49,2);Z_clrst();
//   if(a<=0) return;
//   if(a>MAXAIR) a=MAXAIR;
//   a=a*100/MAXAIR;
//   if(!a) return;
//   V_clr(210,a,w_o+49,2,0xC8);
// }
//
// void Z_drawstprcnt(int y,int n) {
//   char s[20];
//   int l,i,x,c;
//
//   V_setrect(200,70,y*19+7+w_o,19);Z_clrst();
//   sprintf(s,"%3d%%",n);
//   l=strlen(s);x=210;
//   for(i=0;i<l;++i,x+=14) {
//     if(s[i]>='0' && s[i]<='9') c=s[i]-'0';
//     else if(s[i]=='-') c=10;
//     else if(s[i]=='%') c=11;
//     else c=-1;
//     if(c>=0)
//       V_spr(x,y*19+7+w_o,sth[c]);
//   }
// }
//
// void Z_drawstnum(int n) {
//   char s[20];
//   int l,i,x,c;
//
//   V_setrect(270,50,w_o+77,23);Z_clrst();
//   if(!g_dm) return;
//   sprintf(s,"%d",n);
//   l=strlen(s);x=(115-l*14)+200;
//   for(i=0;i<l;++i,x+=14) {
//     if(s[i]>='0' && s[i]<='9') c=s[i]-'0';
//     else if(s[i]=='-') c=10;
//     else if(s[i]=='%') c=11;
//     else c=-1;
//     if(c>=0)
//       V_spr(x,w_o+77+5,sth[c]);
//   }
// }
//
// void Z_drawstwpn(int n,int a) {
//   char s[20];
//   int l,i,x,c;
//
//   i=n;
//   V_setrect(200,120,w_o+58,23);Z_clrst();
//   if(i>=0) V_spr(232,w_o+58+19,sth[i+12]);
//   if(n>=2) {
// 	sprintf(s,"%d",a);
// 	l=strlen(s);x=310-l*14;
//     for(i=0;i<l;++i,x+=14) {
//       if(s[i]>='0' && s[i]<='9') c=s[i]-'0';
//       else if(s[i]=='-') c=10;
//       else if(s[i]=='%') c=11;
//       else c=-1;
//       if(c>=0)
// 	V_spr(x,w_o+58+2,sth[c]);
//     }
//   }
// }
//
void Z_drawmanspr(int x,int y,void *p,char d,unsigned char color) {
	// R_sprite(x-w_x+100,y-w_y+HT/2+1+w_o,p); // V_manspr(x-w_x+100,y-w_y+HT/2+1+w_o,p,color);
	// R_sprite(x-w_x+WD/2,y-w_y+HT/2+1+w_o,p,d); // V_manspr(x-w_x+100,y-w_y+HT/2+1+w_o,p,color);

	// TODO: uncomment
	// if (d) V_manspr2(x - w_x + 100, y - w_y + HT / 2 + 1 + w_o, p, color);
	// else V_manspr(x - w_x + 100, y - w_y + HT / 2 + 1 + w_o, p, color);

	// TODO: color variations.
	if (d) V_manspr2(x-w_x_draw+WD/2,y-w_y+HT/2+1+w_o,p);
	else V_manspr(x-w_x_draw+WD/2,y-w_y+HT/2+1+w_o,p);
}

BN_CODE_IWRAM int Z_canstand(int x,int y,int r) {
  int i;

//  if(y>=FLDH*CELH-1) return 0;
//  if(x<0 || x>FLDW*CELW) return 0;
  i=(x-r)>>3;
  x=(x+r)>>3;
  y=(y+1)>>3;
  if(y>=FLDH || y<0) return 0;
  if(i<0) i=0;
  if(x>=FLDW) x=FLDW-1;
  for(;i<=x;++i)
    if(fld[y][i]==1 || fld[y][i]==2 || fld[y][i]==4)
      if(!z_dot) return 1;
      else if(!((walf[fldf[y][i]]|walf[fldb[y][i]])&2)) return 1;
  return 0;
}

BN_CODE_IWRAM int Z_hitceil(int x,int y,int r,int h) {
  int i;

//  if(y>=FLDH*CELH-1) return 0;
//  if(x<0 || x>FLDW*CELW) return 0;
  i=(x-r)>>3;
  x=(x+r)>>3;
  y=(y-h+1)>>3;
  if(y>=FLDH || y<0) return 0;
  if(i<0) i=0;
  if(x>=FLDW) x=FLDW-1;
  for(;i<=x;++i)
    if(fld[y][i]==1 || fld[y][i]==2)
      if(!z_dot) return 1;
      else if(!((walf[fldf[y][i]]|walf[fldb[y][i]])&2)) return 1;
  return 0;
}

BN_CODE_IWRAM int Z_canfit(int x,int y,int r,int h) {
  int i,j,sx,sy;

//  if(y>=FLDH*CELH-1) return 1;
//  if(x<0 || x>FLDW*CELW) return 1;
  sx=(x-r)>>3;
  sy=(y-h+1)>>3;
  if(sx<0) sx=0;
  if(sy<0) sy=0;
  x=(x+r)>>3;
  y=y>>3;
  if(x>=FLDW) x=FLDW-1;
  if(y>=FLDH) y=FLDH-1;
  for(i=sx;i<=x;++i)
    for(j=sy;j<=y;++j)
      if(fld[j][i]==1 || fld[j][i]==2)
        if(!z_dot) return 0;
        else if(!((walf[fldf[j][i]]|walf[fldb[j][i]])&2)) return 0;
  return 1;
}

BN_CODE_IWRAM int Z_inlift(int x,int y,int r,int h) {
  int i,j,sx,sy;

//  if(y>=FLDH*CELH-1) return 0;
//  if(x<0 || x>FLDW*CELW) return 0;
  sx=(x-r)>>3;
  sy=(y-h+1)>>3;
  if(sx<0) sx=0;
  if(sy<0) sy=0;
  x=(x+r)>>3;
  y=(y-1)>>3;
  if(x>=FLDW) x=FLDW-1;
  if(y>=FLDH) y=FLDH-1;
  for(i=sx;i<=x;++i)
	for(j=sy;j<=y;++j)
	  if(fld[j][i]==9 || fld[j][i]==10) return fld[j][i]-8;
  return 0;
}

BN_CODE_IWRAM int Z_isblocked(int x,int y,int r,int h,int xv) {
  int i,j,sx,sy;

//  if(y>=FLDH*CELH-1) return 0;
//  if(x<0 || x>FLDW*CELW) return 0;
  sx=(x-r)>>3;
  sy=(y-h+1)>>3;
  if(sx<0) sx=0;
  if(sy<0) sy=0;
  x=(x+r)>>3;
  y=(y-1)>>3;
  if(xv<0) x=sx;
  else if(xv>0) sx=x;
  if(x>=FLDW) x=FLDW-1;
  if(y>=FLDH) y=FLDH-1;
  for(i=sx;i<=x;++i)
	for(j=sy;j<=y;++j)
	  if(fld[j][i]==8) return 1;
  return 0;
}

BN_CODE_IWRAM int Z_istrapped(int x,int y,int r,int h) {
  int i,j,sx,sy;

//  if(y>=FLDH*CELH+h) return 0;
//  if(x<0 || x>FLDW*CELW) return 0;
  sx=(x-r)>>3;
  sy=(y-h+1)>>3;
  if(sx<0) sx=0;
  if(sy<0) sy=0;
  x=(x+r)>>3;
  y=(y-1)>>3;
  if(x>=FLDW) x=FLDW-1;
  if(y>=FLDH) y=FLDH-1;
  for(i=sx;i<=x;++i)
    for(j=sy;j<=y;++j)
	  if(fld[j][i]==255) return 1;
  return 0;
}

void Z_set_speed(obj_t *o,int s) {
  int m;

  if(!(m=bn::max(bn::abs(o->xv),bn::abs(o->yv)))) m=1;
  o->xv=o->xv*s/m;o->yv=o->yv*s/m;
}

BN_DATA_EWRAM static unsigned char wfront;

BN_CODE_IWRAM int Z_inwater(int x,int y,int r,int h) {
  int i,j,sx,sy;

//  if(y>=FLDH*CELH+h) return 0;
//  if(x<0 || x>FLDW*CELW) return 0;
  sx=(x-r)>>3;
  sy=(y-h+1)>>3;
  if(sx<0) sx=0;
  if(sy<0) sy=0;
  x=(x+r)>>3;
  y=(y-h/2)>>3;
  if(x>=FLDW) x=FLDW-1;
  if(y>=FLDH) y=FLDH-1;
  for(i=sx;i<=x;++i)
	for(j=sy;j<=y;++j)
	  if(fld[j][i]>=5 && fld[j][i]<=7) {wfront=fldf[j][i];return 1;}
  return 0;
}

BN_CODE_IWRAM int Z_getacid(int x,int y,int r,int h) {
  int i,j,sx,sy,a;
  static unsigned char tab[4]={0,5,10,20};

//  if(y>=FLDH*CELH+h) return 0;
//  if(x<0 || x>FLDW*CELW) return 0;
  a=0;
  sx=(x-r)>>3;
  sy=(y-h+1)>>3;
  if(sx<0) sx=0;
  if(sy<0) sy=0;
  x=(x+r)>>3;
  y=y>>3;
  if(x>=FLDW) x=FLDW-1;
  if(y>=FLDH) y=FLDH-1;
  for(i=sx;i<=x;++i)
	for(j=sy;j<=y;++j)
	  if(fld[j][i]==6) a|=1;
	  else if(fld[j][i]==7) a|=2;
  return tab[a];
}

BN_CODE_IWRAM int Z_canbreathe(int x,int y,int r,int h) {
  int i,j,sx,sy;

//  if(y>=FLDH*CELH+h) return 1;
//  if(x<0 || x>FLDW*CELW) return 1;
  sx=(x-r)>>3;
  sy=(y-h+1)>>3;
  if(sx<0) sx=0;
  if(sy<0) sy=0;
  x=(x+r)>>3;
  y=(y-h/2)>>3;
  if(x>=FLDW) x=FLDW-1;
  if(y>=FLDH) y=FLDH-1;
  if(sx>x || sy>y) return 1;
  for(i=sx;i<=x;++i)
    for(j=sy;j<=y;++j)
      if(fld[j][i]==0 || fld[j][i]==3 || fld[j][i]==9 || fld[j][i]==10) return 1;
  return 0;
}

BN_CODE_IWRAM int Z_overlap(obj_t *a,obj_t *b) {
  if(a->x - a->r > b->x + b->r) return 0;
  if(a->x + a->r < b->x - b->r) return 0;
  if(a->y <= b->y - b->h) return 0;
  if(a->y - a->h >= b->y) return 0;
  return 1;
}

void Z_kickobj(obj_t *o,int x,int y,int pwr) {
  int dx,dy,m;

  dx=o->x-x;dy=o->y-o->h/2-y;
  if(!(m=bn::max(bn::abs(dx),bn::abs(dy)))) m=1;
  o->vx+=(long)dx*pwr/m;
  o->vy+=(long)dy*pwr/m;
}

BN_CODE_IWRAM int Z_cansee(int x,int y,int xd,int yd) {
  // register unsigned int d,m; - Clangd: ISO C++17 does not allow 'register' storage class specifier
  unsigned int d,m;
  int sx,sy;
  unsigned int xe,ye,s,i;

  if((xd-=x)>0) sx=1;
  else if(xd<0) sx=-1;
  else sx=0;
  if((yd-=y)>0) sy=1;
  else if(yd<0) sy=-1;
  else sy=0;
  if(!xd && !yd) return 1;
  if((xd=bn::abs(xd)) > (yd=bn::abs(yd))) d=xd; else d=yd;
  xe=ye=0;
  for(i=0;i<=d;) {
	if(x<0 || x>=FLDW*8 || y<0 || y>=FLDH*8) return 0;
	if((bmap[y>>5][x>>5]&BM_WALL)) {
	  if(fld[y>>3][x>>3]==1 || fld[y>>3][x>>3]==2) return 0;
	  if((xe+=(xd<<3))>=d) {
		x+=xe/d*sx;xe=xe%d;
	  }
	  if((ye+=(yd<<3))>=d) {
		y+=ye/d*sy;ye=ye%d;
	  }
	  i+=8;
	}else{
	  if(sx==0) m=0;
	  else{m=x&31;if(sx>0) m^=31; ++m;}
	  if(sy==0) s=0;
	  else{s=y&31;if(sy>0) s^=31; ++s;}
	  if((s<m && s!=0) || m==0) m=s;
	  i+=m;
		x+=(xd*m+xe)/d*sx;xe=(xd*m+xe)%d;
		y+=(yd*m+ye)/d*sy;ye=(yd*m+ye)%d;
	}
  }
  return 1;
}

int Z_look(obj_t *a,obj_t *b,int d) {
  if(Z_sign(b->x-a->x)!=d*2-1) return 0;
  return Z_cansee(a->x,a->y-a->h/2,b->x,b->y-b->h/2);
}

#define wvel(v) if((xv=bn::abs(v)+1)>5) v=Z_dec(v,xv/2-2)

unsigned char z_mon=0;

BN_DATA_EWRAM static int _zmove_x, _zmove_y, _zmove_xv, _zmove_yv, _zmove_r, _zmove_h, _zmove_lx, _zmove_ly, _zmove_st;
BN_DATA_EWRAM static unsigned char _zmove_inw;

BN_CODE_IWRAM int Z_moveobj(obj_t *p) {
  int &x=_zmove_x, &y=_zmove_y, &xv=_zmove_xv, &yv=_zmove_yv, &r=_zmove_r, &h=_zmove_h, &lx=_zmove_lx, &ly=_zmove_ly, &st=_zmove_st;
  unsigned char &inw=_zmove_inw;

  st=0;
  switch(Z_inlift(x=p->x,y=p->y,r=p->r,h=p->h)) {
    case 0:
      if(++p->yv>MAX_YV) --p->yv;
      break;
    case 1:
      if(--p->yv < -5) ++p->yv;
      break;
    case 2:
      if(p->yv > 5) {--p->yv;break;}
      ++p->yv;break;
  }
  if((inw=Z_inwater(x,y,r,h))!=0) {
	st|=Z_INWATER;
	wvel(p->xv);
	wvel(p->yv);
	wvel(p->vx);
	wvel(p->vy);
  }
  p->vx=Z_dec(p->vx,1);
  p->vy=Z_dec(p->vy,1);
  xv=p->xv+p->vx;yv=p->yv+p->vy;
  while(xv || yv) {
	if(x<-100 || x>=FLDW*8+100 || y<-100 || y>=FLDH*8+100)
	  {st|=Z_FALLOUT;}

	lx=x;
	x+=(bn::abs(xv)<=7)?xv:((xv>0)?7:-7);
	if(z_mon) if(Z_isblocked(x,y,r,h,xv)) st|=Z_BLOCK;
	if(!Z_canfit(x,y,r,h)) {
	  if(xv==0) x=lx;
	  else if(xv<0) x=((lx-r)&0xFFF8)+r;
          else x=((lx+r)&0xFFF8)-r+7;
	  xv=p->xv=p->vx=0;st|=Z_HITWALL;
	}
	xv-=(bn::abs(xv)<=7)?xv:((xv>0)?7:-7);

	ly=y;
	y+=(bn::abs(yv)<=7)?yv:((yv>0)?7:-7);
	if(yv>=8) --y;
	if(yv<0 && Z_hitceil(x,y,r,h)) {
	  y=((ly-h+1)&0xFFF8)+h-1;
	  yv=p->vy=1;p->yv=0;st|=Z_HITCEIL;
	}
	if(yv>0 && Z_canstand(x,y,r)) {
	  y=((y+1)&0xFFF8)-1;
	  yv=p->yv=p->vy=0;st|=Z_HITLAND;
	}
	yv-=(bn::abs(yv)<=7)?yv:((yv>0)?7:-7);
  }
  p->x=x;p->y=y;
  if(Z_inwater(x,y,r,h)) {
	st|=Z_INWATER;
	if(!inw) st|=Z_HITWATER;
  }else if(inw) st|=Z_HITAIR;
  return st;
}

void Z_calc_time(unsigned int t, unsigned short *h, unsigned short *m, unsigned short *s) {
  /* 1092 ticks = 1 minute */
  unsigned int total_sec = t * 60 / 1092;
  *h = total_sec / 3600;
  *m = (total_sec % 3600) / 60;
  *s = total_sec % 60;
}

void Z_splash(obj_t *p, int n) {
	Z_sound(bulsnd[0], 128);
	DOT_water(p->x, p->y - p->h / 2, p->xv + p->vx, p->yv + p->vy, n,
	          (int) walp[wfront] - 1);
}
