#include "glob.h"
// //#include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include "vga.h"
#include "files.h"
// #include "error.h"
#include "view.h"
#include "renderer.h"
#include "bmap.h"
#include "dots.h"
#include "weapons.h"
#include "player.h"
#include "monster.h"

#include <cstring>

#include "bn_log.h"
#include "bn_math.h"
#include "bn_memory.h"
#include "items.h"
#include "switch.h"
#include "misc.h"
#include "fx.h"
// #include "smoke.h"
#ifndef BN_CODE_IWRAM
#define BN_CODE_IWRAM __attribute__((section(".iwram")))
#endif

#define MANCOLOR 0xD0

#define MAX_ATM 90

#define MN_TN (MN__LAST-MN_DEMON)

extern unsigned char z_mon;

enum{
  SLEEP,GO,RUN,CLIMB,DIE,DEAD,ATTACK,SHOOT,PAIN,WAIT,REVIVE,RUNOUT
};

typedef struct{
  obj_t o;
  unsigned char t,d,st,ftime;
  unsigned char initialized;  // 0=первый тик (полная ветка), 1=можно early-exit для SLEEP
  int fobj;
  int s;
  char *ap;
  int aim,life,pain,ac,tx,ty,ammo;
  short atm;
}mn_t;

typedef struct{
  int r,h,l,mp,rv,jv,sp,minp;
}mnsz_t;

unsigned char nomon=1;

static int MN_hit(int n, int d, int o, int t);

static char *sleepanim[MN_TN]={
  "AAABBB","AAABBB","AAABBB","AAABBB","AAABBB","AAABBB","AAABBB","AAABBB",
  "A","AAABBB","AAABBB","AAABBB","AAABBB","AAABBB","AAABBB","AAABBB","AAABBB",
  "A","A","AAABBB"
}, *goanim[MN_TN]={
  "AABBCCDD","AABBCCDD","AABBDDAACCDD","AABBDDAACCDD","AABBDDCCDDBB",
  "AABBDDAACCDD","AABBCCDD","AABBCCDD","A","AABB","AABBCCBB",
  "AABBCCDDEEFF","AABBCCDDEEFF","AABBCCDDEEFF","AABBCCDDEEFF","AABBCCDDEEFF",
  "AABB","A","DDEEFFGGHHIIJJKKLLAABBCC","ACDABD"
}, *painanim[MN_TN]={
  "H","H","G","G","G","G","H","H","F","E","G","I","I","J","L","Q","EECCDDCC",
  "A","D","G"
}, *waitanim[MN_TN]={
  "A","A","A","A","A","A","A","A","A","AABB","A","A","A","I","K","A","A",
  "A","D","E"
}, *attackanim[MN_TN]={
  "EEFFGG","EEFFGG","EEEEEF","EEEEEF","EEEEEF","EF","EEFFGG","EEFFGG",
  "BBCCDD","CCDD","DDEEFF","GH","GH","GGGGHH","GGHHII",
  "QQGGGHHHIIJJKKLLMMNNOOPP","BBFFAA","A","OOPPQQ","EEEEFF"
}, *dieanim[MN_TN]={
  "IIIJJJKKKLLLMMM","IIIJJJKKKLLL","HHHIIIJJJKKK","HHHIIIJJJKKK",
  "HHHIIIJJJKKKLLLMMMNNNOOO","HHHIIIJJJKKKLLLMMM",
  "IIIJJJKKKLLLMMMNNN","IIIJJJKKKLLLMMMNNN","GGGHHHIIIJJJKKK",
  "FFFGGGHHHIIIJJJKKK","HHHIIIJJJKKKLLLMMM",
  "JJJJKKKKLLLLMMMMNNNNOOOOPPPPQQQQRRRR","JJJKKKLLLMMMNNNOOO",
  "KKKLLLMMMNNNOOOPPPRRRSSS","MMMNNNOOOPPP","RRRSSSTTTUUUVVVWWWXXXYYY",
  "DDDD","CCCDDDEEEFFFGGG","D","HHHHIIIIJJJJKKKKLLLLMMMM"
}, *slopanim[MN_TN]={
  "","NNNOOOPPPRRRSSSTTT","MMMNNNOOOPPPRRRSSSTTT","MMMNNNOOOPPPRRRSSSTTT","",
  "OOOPPPQQQRRRSSS","","","","","","","","","","","","","","OOPPQQRRSSTTUUVV"
}, *deadanim[MN_TN]={
  "N","M","L","L","P","N","O","O","L","","","S","P","T","Q","Z","C","","D","N"
}, *messanim[MN_TN]={
  "","U","U","U","","T","","","","","","","","","","","","","","W"
};

int hit_xv,hit_yv;

BN_DATA_EWRAM static void *spr[MN_TN][29*2],*fspr[8],*sgun[2];
BN_DATA_EWRAM static pcm_ref_t fsnd,pauksnd,trupsnd;
BN_DATA_EWRAM static void *gspr[13][8];
BN_DATA_EWRAM static char sprd[MN_TN][29*2];
BN_DATA_EWRAM static pcm_ref_t snd[MN_TN][5],impsitsnd[2],impdthsnd[2],firsnd,slopsnd,gsnd[4];
BN_DATA_EWRAM static void *pl_spr[2];
BN_DATA_EWRAM static pcm_ref_t swgsnd,pchsnd,telesnd;
BN_DATA_EWRAM static pcm_ref_t positsnd[3],podthsnd[3];
BN_DATA_EWRAM static mn_t mn[MAXMN];
static int mnum,gsndt;
static mnsz_t mnsz[MN_TN+1]={
//rad  ht  life  pain rv jv  slop min_pn
    0,  0,    0,    0, 0, 0,    0,    0,	// none
   15, 28,   60,   20, 7,10,	0,   10,	// demon
   10, 28,   25,   15, 3,10,   30,    0,	// imp
   10, 28,   15,   10, 3,10,   30,    0,	// zomby
   10, 28,   20,   10, 3,10,   30,    0,	// sergeant
   20, 55,  500,   70, 5,10,	0,   50,	// cyberdemon
   12, 28,   60,   20, 3,10,   30,   10,	// chaingunner
   12, 32,  150,   40, 3,10,    0,   30, 	// baron of hell
   12, 32,   75,   40, 3,10,    0,   30, 	// hell knight
   15, 28,  100,   10, 4, 4,    0,    0,	// cacodemon
    8, 18,   60,   10, 4, 4,    0,    0,	// lost soul
   15, 28,  100,   10, 4, 4,    0,    0,	// pain elemental
   64, 50,  500,   70, 4,10,    0,   50,	// spider mastermind
   25, 27,  150,   20, 4,10,    0,    0,	// arachnotron
   18, 30,  200,   40, 3, 7,    0,   20,	// mancubus
   17, 36,  200,   40, 6,11,    0,   20,	// revenant
   17, 36,  150,   30, 7,12,    0,   10,	// archvile
    5,  5,   35,   20,14, 6,    0,   10,	// fish
    5, 17,   20,    0, 7, 6,    0,    0,	// barrel
   17, 38,   20,   40, 3, 6,    0,   20,	// robot
    8, 26,  400,   70, 8,10,   30,   50		// man
};

// void MN_savegame(int h) {
//   int n;
//
//   for(n=MAXMN;--n;) if(mn[n].t) break;
//   ++n;write(h,&n,4);
// //  printf("Monsters: %d\n",n);
//   write(h,mn,n*sizeof(mn[0]));
//   write(h,&mnum,4);write(h,&gsndt,4);
// }
//
// static void setst(int,int);
//
// void MN_loadgame(int h) {
//   int n,c;
//
//   read(h,&n,4);
// //  printf("Monsters: %d\n",n);
//   read(h,mn,n*sizeof(mn[0]));
// //  for(c=0;c<n;++c) {
// //    printf("#%d: %d (%d,%d, %d,%d, %d) %d %d life=%d\n",c,mn[c].t,
// //      mn[c].o.x,mn[c].o.y,
// //      mn[c].o.r,mn[c].o.h,
// //      mn[c].d,mn[c].st,mn[c].s,mn[c].life);
// //  }
//   read(h,&mnum,4);read(h,&gsndt,4);
// //  printf("mnum=%d\n",mnum);
//   for(n=0;n<MAXMN;++n) if(mn[n].t) {
//     c=mn[n].ac;setst(n,mn[n].st);mn[n].ac=c;
//   }
// }
//
// #define GGAS_TOTAL (MN__LAST-MN_DEMON+16+10)
//
void MN_alloc(void) {
  int i,j;
  static char sn[MN_TN][5][7]={
	{"DMACT","DMPAIN","SGTATK","SGTSIT","SGTDTH"},
	{"BGACT","POPAIN","CLAW","",""},
	{"POSACT","POPAIN","","",""},
	{"POSACT","POPAIN","","",""},
	{"","DMPAIN","HOOF","CYBSIT","CYBDTH"},
	{"POSACT","POPAIN","","",""},
	{"","DMPAIN","","BRSSIT","BRSDTH"},
	{"","DMPAIN","","KNTSIT","KNTDTH"},
	{"DMACT","DMPAIN","","CACSIT","CACDTH"},
	{"DMACT","DMPAIN","SKLATK","SKLATK","FIRXPL"},
	{"DMACT","PEPAIN","","PESIT","PEDTH"},
	{"","DMPAIN","METAL","SPISIT","SPIDTH"},
	{"BSPACT","DMPAIN","BSPWLK","BSPSIT","BSPDTH"},
	{"DMACT","MNPAIN","MANATK","MANSIT","MANDTH"},
	{"SKEACT","POPAIN","SKEATK","SKESIT","SKEDTH"},
	{"VILACT","VIPAIN","VILATK","VILSIT","VILDTH"},
	{"","","BITE1","",""},
	{"","","","","BAREXP"},
	{"BSPACT","","BSPWLK","BSPSIT","BSPDTH"},
	{"HAHA1","PLPAIN","","STOP1","PDIEHI"}
  },msn[MN_TN][5]={
	"SARG","TROO","POSS","SPOS","CYBR","CPOS","BOSS","BOS2","HEAD","SKUL",
	"PAIN","SPID","BSPI","FATT","SKEL","VILE","FISH","BAR1","ROBO","PLAY"
  };
  static char gsn[6]="GOOD0";
  static int mms[MN_TN]={
	14*2,21*2,21*2,21*2,16*2,20*2,15*2,15*2,12*2,11*2,13*2,19*2,16*2,
	20*2,17*2,29*2,6*2,2*2,17*2,23*2
  };

//  logo("size of monster: %d\n",sizeof(mn[0]));
  sgun[0]=Z_getspr("PWP4",0,1,NULL);
  sgun[1]=Z_getspr("PWP4",1,1,NULL);
  for(j=0;j<MN_TN;++j) {
    for(i=0;i<mms[j];++i) {
    	// TODO: check optimization here
    	if (j == MN_SOUL-1 || j == MN_FISH-1) {
    		spr[j][i]=Z_getspr_ewram(msn[j],i/2,(i&1)+1,&sprd[j][i],spr[j][i]);
    	} else {
    		spr[j][i]=Z_getspr(msn[j],i/2,(i&1)+1,&sprd[j][i]);
    	}
    }
    if(j==MN_BARREL-1)
      for(i=4;i<14;++i) spr[j][i]=Z_getspr("BEXP",i/2-2,(i&1)+1,&sprd[j][i]);
    for(i=0;i<5;++i)
      if(sn[j][i][0]) snd[j][i]=Z_getsnd(sn[j][i]);
	else snd[j][i]=pcm_ref_t{};
	// else snd[j][i]=NULL;
    // logo_gas(j+5,GGAS_TOTAL); // TODO: GAS
  }
  for(i=0;i<8;++i) fspr[i]=Z_getspr("FIRE",i,0,NULL);
  pl_spr[0]=Z_getspr("PLAY",'N'-'A',0,NULL);
  pl_spr[1]=Z_getspr("PLAY",'W'-'A',0,NULL);
  strcpy(gsn,"PLPx");
  for(i=0;i<13;++i) {
    gsn[3]=i<10?i+'0':i+'A'-10;
    for(j=0;j<8;++j) gspr[i][j]=Z_getspr(gsn,j,0,NULL);
  }
  impsitsnd[0]=Z_getsnd("BGSIT1");
  impsitsnd[1]=Z_getsnd("BGSIT2");
  impdthsnd[0]=Z_getsnd("BGDTH1");
  impdthsnd[1]=Z_getsnd("BGDTH2");
  positsnd[0]=Z_getsnd("POSIT1");
  positsnd[1]=Z_getsnd("POSIT2");
  positsnd[2]=Z_getsnd("POSIT3");
  podthsnd[0]=Z_getsnd("PODTH1");
  podthsnd[1]=Z_getsnd("PODTH2");
  podthsnd[2]=Z_getsnd("PODTH3");
  fsnd=Z_getsnd("FLAME");
  firsnd=Z_getsnd("FIRSHT");
  slopsnd=Z_getsnd("SLOP");
  swgsnd=Z_getsnd("SKESWG");
  pchsnd=Z_getsnd("SKEPCH");
  telesnd=Z_getsnd("TELEPT");
  pauksnd=Z_getsnd("PAUK1");
  trupsnd=Z_getsnd("UTRUP");
  strcpy(gsn,"GOODx");
  for(i=0;i<4;++i) {gsn[4]=i+'1';gsnd[i]=Z_getsnd(gsn);}
}

void MN_init(void) {
  int i;

  for(i=0;i<MAXMN;++i) {mn[i].t=0;mn[i].st=SLEEP;}
  gsndt=mnum=0;
}

static void setst(int i,int st) {
  char *a;
  int t;

  switch(mn[i].st) {
    case DIE: case DEAD:
      if(st!=DEAD && st!=REVIVE) return;
  }
  mn[i].ac=0;
  t=mn[i].t-1;
  switch(mn[i].st=st) {
	case SLEEP: mn[i].initialized=0; a=sleepanim[t];break;
	case PAIN: a=painanim[t];break;
	case WAIT: a=waitanim[t];break;
	case CLIMB:
	case RUN: case RUNOUT:
	case GO: a=goanim[t];break;
	case SHOOT:
	  if(t==MN_SKEL-1) {a="KKKKJJ";break;}
	  if(t==MN_ROBO-1) {a="MN";break;}
	case ATTACK: a=attackanim[t];
	  if(st==ATTACK && t==MN_VILE-1) a="[[\\\\]]";
	  break;
	case DIE:
	  if(g_map==9 && t==MN_BSP-1) Z_sound(pauksnd,128);
	  a=dieanim[t];break;
	case DEAD:
	  a=deadanim[t];
	  if(mn[i].ap==slopanim[t]) a=messanim[t];
	  if(t==MN_BARREL-1) {mn[i].t=0;}
	  break;
	case REVIVE:
	  a=(mn[i].ap==messanim[t])?slopanim[t]:dieanim[t];
	  mn[i].ac=strlen(a)-1;
	  mn[i].o.r=mnsz[t+1].r;mn[i].o.h=mnsz[t+1].h;
	  mn[i].life=mnsz[t+1].l;mn[i].ammo=mn[i].pain=0;
	  ++mnum;
	  break;
  }
  mn[i].ap=a;
}

static int last=0;

int MN_spawn(int x,int y,unsigned char d,int t) {
  int i;

  if(g_dm && nomon && t<MN_PL_DEAD) return -1;
  i=last;
  for(;;) {
    if(++i>=MAXMN) i=0;
    if(i==last) break;
    if(!mn[i].t) goto ok;
  }
  i=last+1;
  for(;;) {
    if(i>=MAXMN) i=0;
    if(i==last) break;
    if(mn[i].t>=MN_PL_DEAD) goto ok;
    i++;
  }
  return -1;
ok:
  last++;
  if(last>=MAXMN) last=0; else last=last;
  mn[i].o.x=x;mn[i].o.y=y;
  mn[i].o.xv=mn[i].o.yv=mn[i].o.vx=mn[i].o.vy=0;
  mn[i].d=d;mn[i].t=t;
  mn[i].st=SLEEP;
  if(t<MN_PL_DEAD) {
    mn[i].o.r=mnsz[t].r;mn[i].o.h=mnsz[t].h;
    mn[i].life=mnsz[t].l;
    setst(i,SLEEP);mn[i].s=random(18);
    ++mnum;
  }else {mn[i].o.r=8;mn[i].o.h=6;mn[i].life=0;mn[i].st=DEAD;mn[i].s=0;}
  mn[i].aim=-3;mn[i].atm=0;
  mn[i].pain=0;
  mn[i].ammo=0;
  mn[i].ftime=0;
  mn[i].initialized=0;
  return i;
}

int MN_spawn_deadpl(obj_t *o,unsigned char c,int t) {
  int i;
  static short gsz[13][2]={
    {3,6},
    {3,7},
    {4,13},
    {3,7},
    {3,8},
    {4,6},
    {2,5},
    {2,5},
    {1,3},
    {1,3},
    {1,3},
    {1,4},
    {1,3}
  };

  if((i=MN_spawn(o->x,o->y,c,t+MN_PL_DEAD))==-1) return -1;
  mn[i].o=*o;
  if(t>=2) {
    mn[i].o.r=gsz[t-2][0];
    mn[i].o.h=gsz[t-2][1];
    mn[i].s=rand()&0xff;
    mn[i].ammo=(random(2)*2-1)*(random(20)+12);
  }
  return i;
}

static int isfriend(int a,int b) {
  if(a==MN_BARREL || b==MN_BARREL) return 1;
  if(a==b) switch(a) {
    case MN_IMP: case MN_DEMON:
    case MN_BARON: case MN_KNIGHT:
    case MN_CACO: case MN_SOUL:
    case MN_MANCUB: case MN_SKEL:
    case MN_FISH:
      return 1;
  }
  if(a==MN_SOUL && b==MN_PAIN) return 1;
  if(b==MN_SOUL && a==MN_PAIN) return 1;
  return 0;
}

static int MN_findnewprey(int i) {
  int a,b,l;

  a=!PL_isdead(&pl1);
  if(_2pl) b=!PL_isdead(&pl2); else b=0;
  if(a) {
	if(b) mn[i].aim=(bn::abs(mn[i].o.x-pl1.o.x)+bn::abs(mn[i].o.y-pl1.o.y)
	    <= bn::abs(mn[i].o.x-pl2.o.x)+bn::abs(mn[i].o.y-pl2.o.y))?-1:-2;
	else mn[i].aim=-1;
  }else{
	if(b) mn[i].aim=-2;
	else{
	  for(a=0,b=32000,mn[i].aim=-3;a<MAXMN;++a)
	    if(mn[a].t && mn[a].st!=DEAD && a!=i && !isfriend(mn[a].t,mn[i].t))
	      if((l=bn::abs(mn[i].o.x-mn[a].o.x)+bn::abs(mn[i].o.y-mn[a].o.y))<b)
	        {mn[i].aim=a;b=l;}
	  if(mn[i].aim<0) {mn[i].atm=MAX_ATM;return 0;} else mn[i].atm=0;
	}
  }
  return 1;
}

int Z_getobjpos(int i,obj_t *o) {
  if(i==-1) {*o=pl1.o;return !PL_isdead(&pl1);}
  if(_2pl) if(i==-2) {*o=pl2.o;return !PL_isdead(&pl2);}
  if(i>=0 && i<MAXMN) if(mn[i].t && mn[i].st!=DEAD)
	{*o=mn[i].o;return 1;}
  return 0;
}

static pcm_ref_t& wakeupsnd(int t) {
  switch(t) {
	case MN_IMP: return impsitsnd[random(2)];
	case MN_ZOMBY: case MN_SERG: case MN_CGUN:
	  return positsnd[random(3)];
  }
  return snd[t-1][3];
}

static pcm_ref_t& dthsnd(int t) {
  switch(t) {
	case MN_IMP: return impdthsnd[random(2)];
	case MN_ZOMBY: case MN_SERG: case MN_CGUN:
	  return podthsnd[random(3)];
  }
  return snd[t-1][4];
}

static int canshoot(int t) {
  switch(t) {
	case MN_DEMON: case MN_FISH: case MN_BARREL:
	  return 0;
  }
  return 1;
}

static int shoot(int i,obj_t *o,int n) {
  int xd,yd,m;

  if(mn[i].ammo<0) return 0;
  if(!n) switch(mn[i].t) {
	case MN_FISH: case MN_BARREL:
	case MN_DEMON: return 0;
	case MN_CGUN:
	case MN_BSP:
	case MN_ROBO:
	  if(++mn[i].ammo>=50) mn[i].ammo=(mn[i].t==MN_ROBO)?-200:-50;
	  break;
	case MN_MAN:
	  break;
	case MN_MANCUB:
	  if(++mn[i].ammo>=5) mn[i].ammo=-50;
	  break;
	case MN_SPIDER:
	  if(++mn[i].ammo>=100) mn[i].ammo=-50;
	  break;
	case MN_CYBER:
	  if(rand()&1) return 0;
	  if(++mn[i].ammo>=10) mn[i].ammo=-50;
	  break;
	case MN_BARON: case MN_KNIGHT:
	  if(rand()&7) return 0;
	  break;
	case MN_SKEL:
	  if(rand()&31) return 0;
	  break;
	case MN_VILE:
	  if(rand()&7) return 0;
	  break;
	case MN_PAIN:
	  if(rand()&7) return 0;
	  break;
	default:
	  if(rand()&15) return 0;
  }
  if(!Z_look(&mn[i].o,o,mn[i].d)) return 0;
  mn[i].atm=0;
  mn[i].tx=o->x+(o->xv+o->vx)*6;mn[i].ty=o->y-o->h/2+(o->yv+o->vy)*6;
  if(bn::abs(mn[i].tx-mn[i].o.x)<bn::abs(mn[i].ty-mn[i].o.y+mn[i].o.h/2)) return 0;
//  if(Z_sign(mn[i].tx-mn[i].o.x)!=mn[i].d*2-1) return 0;
  switch(mn[i].t) {
	case MN_IMP: case MN_BARON: case MN_KNIGHT: case MN_CACO:
	  setst(i,SHOOT);Z_sound(firsnd,128);break;
	case MN_SKEL:
	  setst(i,SHOOT);Z_sound(snd[MN_SKEL-1][2],128);break;
	case MN_VILE:
	  mn[i].tx=o->x;mn[i].ty=o->y;
	  setst(i,SHOOT);Z_sound(fsnd,128);
	  Z_sound(snd[MN_VILE-1][2],128);break;
	case MN_SOUL:
	  setst(i,ATTACK);Z_sound(snd[MN_SOUL-1][2],128);
	  yd=mn[i].ty-mn[i].o.y+mn[i].o.h/2;xd=mn[i].tx-mn[i].o.x;
	  if(!(m=bn::max(bn::abs(xd),bn::abs(yd)))) m=1;
	  mn[i].o.xv=xd*16/m;mn[i].o.yv=yd*16/m;
	  break;
	case MN_MANCUB: if(mn[i].ammo==1) Z_sound(snd[MN_MANCUB-1][2],128);
	case MN_ZOMBY: case MN_SERG: case MN_BSP: case MN_ROBO:
	case MN_CYBER: case MN_CGUN: case MN_SPIDER:
	case MN_PAIN: case MN_MAN:
	  setst(i,SHOOT);break;
	default:
	  return 0;
  }
  return 1;
}

static int kick(int i,obj_t *o) {
  switch(mn[i].t) {
	case MN_FISH:
	  setst(i,ATTACK);return 1;
	case MN_DEMON:
	  setst(i,ATTACK);Z_sound(snd[0][2],128);return 1;
	case MN_IMP:
	  setst(i,ATTACK);Z_sound(snd[1][2],128);return 1;
	case MN_SKEL:
	  setst(i,ATTACK);Z_sound(swgsnd,128);return 1;
	case MN_ROBO:
	  setst(i,ATTACK);Z_sound(swgsnd,128);return 1;
	case MN_BARON: case MN_KNIGHT: case MN_CACO: case MN_MANCUB:
//	  o->xv=o->vx=o->yv=o->vy=0;
	  return shoot(i,o,1);
  }
  return 0;
}

static int iscorpse(obj_t *o,int n) {
  int i;

  if(!n) if(rand()&7) return -3;
  for(i=0;i<MAXMN;++i) if(mn[i].t) if(mn[i].st==DEAD)
    if(Z_overlap(o,&mn[i].o)) switch(mn[i].t) {
      case MN_SOUL: case MN_PAIN:
      case MN_CYBER: case MN_SPIDER:
      case MN_PL_DEAD: case MN_PL_MESS:
      case MN_VILE: case MN_BARREL:
        continue;
      default:
        return i;
    }
  return -3;
}

void MN_act(void) {
  int i,st,sx,sy,t;
  int gx,gy;
  static obj_t o;
  static int pt_x=0,pt_xs=1,pt_y=0,pt_ys=1;

  if(bn::abs(pt_x+=pt_xs) > 123) pt_xs=-pt_xs;
  if(bn::abs(pt_y+=pt_ys) > 50) pt_ys=-pt_ys;
  if(gsndt>0) if(--gsndt==0) {
	Z_sound(gsnd[random(4)],128);
  }
  for(i=0;i<MAXMN;++i) {
    mn_t &m = mn[i];
    if((t=m.t)==0) continue;
    obj_t *const mo = &m.o;
    /* DEAD: труп лежит, только анимация. Но при vx/vy (от взрыва) — нужна физика. */
    if(m.st==DEAD) {
      if(mo->vx || mo->vy || mo->xv || mo->yv) {
        z_mon=1; Z_moveobj(mo); z_mon=0;
      }
      ++m.ac; if(!m.ap[m.ac]) m.ac=0;
      continue;
    }
    /* DIE: анимация смерти, нужна физика (падение). Упрощённая ветка без SW_press и логики состояний. */
    if(m.st==DIE) {
      z_mon=1; st=Z_moveobj(mo); z_mon=0;
      ++m.ac;
      if(t==MN_BARREL) {
        if(m.ac==2) Z_explode(mo->x,mo->y-8,30,m.aim);
        if(!m.ap[m.ac]) { m.ac=0; m.t=0; continue; }
      } else if(!m.ap[m.ac]) {
        m.ac=0; setst(i,DEAD);
        if(t==MN_PAIN || t==MN_SOUL) m.ftime=0;
        if(t==MN_PAIN) {
          if((sx=MN_spawn(mo->x-15,mo->y,0,MN_SOUL))!=-1) setst(sx,GO);
          if((sx=MN_spawn(mo->x+15,mo->y,1,MN_SOUL))!=-1) setst(sx,GO);
          if((sx=MN_spawn(mo->x,mo->y-10,1,MN_SOUL))!=-1) setst(sx,GO);
        }
      }
      continue;
    }
    /* Early exit для SLEEP с initialized: первый тик — полная ветка, далее только анимация + Z_look раз в 18 кадров.
       Если под монстром пропала земля — не делаем early exit, идём в полную ветку (Z_moveobj) чтобы он падал. */
    if(m.st==SLEEP && m.initialized && Z_canstand(mo->x,mo->y,mo->r)) {
      ++m.ac;
      if(!m.ap[m.ac]) m.ac=0;
      if(++m.s>=18) { m.s=0;
	if(Z_look(mo,&pl1.o,m.d))
	  {setst(i,GO);m.aim=-1;m.atm=0;Z_sound(wakeupsnd(t),128);}
	if(_2pl) if(Z_look(mo,&pl2.o,m.d))
	  {setst(i,GO);m.aim=-2;m.atm=0;Z_sound(wakeupsnd(t),128);}
      }
      continue;
    }
    switch(t) {
	case MN_FISH:
	  if(!Z_inwater(mo->x,mo->y,mo->r,mo->h)) break;
	case MN_SOUL: case MN_PAIN: case MN_CACO:
	  if(m.st!=DIE && m.st!=DEAD) --mo->yv;
	  break;
    }
    if(t>=MN_PL_HEAD1) {
      t-=MN_PL_HEAD1;
      o=*mo;
      st=Z_moveobj(mo);
      if(st&(Z_HITCEIL|Z_HITLAND)) {
        gy=o.yv+o.vy;
        if(bn::abs(gy)>20) gy=(gy>0)?20:-20;
        mo->vy=0;
        mo->yv=-gy/2;
      }
      if(st&Z_HITWALL) {
        gy=o.yv+o.vy;
        if(bn::abs(gy)>20) gy=(gy>0)?20:-20;
        mo->xv=0;
        mo->vx=-gy/2;
      }
      gx=bn::abs(mo->xv+mo->vx-o.xv-o.vx);
      gy=bn::abs(mo->yv+mo->vy-o.yv-o.vy);
      gx=gy+gx>2?gy+gx-2:0;
      if(gx>15) gx=15;
      m.ammo+=(random(2)*8-4)*gx;
      if(gx==0 && bn::abs(o.xv+o.vx)<2 && bn::abs(o.yv+o.vy)<2) m.ammo/=2;
      if(st&Z_HITLAND)
        if(bn::abs(o.yv+o.vy)>2)
          mo->vx+=m.ammo/10;
      m.s=m.s+m.ammo&0xFF;
      m.ammo=Z_dec(m.ammo,1);
    } else {
      z_mon=1;st=Z_moveobj(mo);z_mon=0;
    }
    const mnsz_t *const ms = &mnsz[t];
    const bool in_water = (st & Z_INWATER) != 0;
    const bool is_fish = (t == MN_FISH);
    if(m.st!=DEAD && m.st!=DIE) BM_mark(mo,BM_MONSTER);
    if(st&Z_FALLOUT) {
      if(t==MN_ROBO) g_exit=1;
      m.t=0;--mnum;continue;
    }
    if(st&Z_HITWATER) Z_splash(mo,mo->r+mo->h);
    SW_press(mo->x,mo->y,mo->r,mo->h,8,i);
    if(m.ftime) {
      --m.ftime;
      // SMK_flame(mo->x,mo->y-mo->h/2, mo->xv+mo->vx,mo->yv+mo->vy, mo->r/2,mo->h/2,rand()%(200*2+1)-200,-500,1,m.fobj); // TODO: SMK FX
    }
    if(in_water) m.ftime=0;
    if(in_water) if(!(rand()&31)) switch(t) {
      case MN_FISH:
        if(rand()&3) break;
      case MN_ROBO: case MN_BARREL:
      case MN_PL_DEAD: case MN_PL_MESS:
        FX_bubble(mo->x+((rand()&1)*2-1)*random(mo->r+1), mo->y-random(mo->h+1),0,0,1);
        break;
      default:
        FX_bubble(mo->x,mo->y-mo->h*3/4,0,0,5);
  		break;
    }
    // mo->h=ms->h;
    if(t==MN_BARREL) {
      // BM_mark(mo,BM_MONSTER);
      if(!m.ap[++m.ac]) {
        m.ac=0;if(m.st==DIE || m.st==DEAD) {m.t=0;}
      }else if(m.st==DIE && m.ac==2) Z_explode(mo->x,mo->y-8,30,m.aim);
      continue;
    }
    if(t==MN_SOUL) if(st&Z_HITAIR) Z_set_speed(mo,16);
    if(m.ammo<0) ++m.ammo;
    if(mo->yv<0)
	if(in_water) mo->yv=-4;
    ++m.atm;
    switch(m.st) {
   case PAIN:
	if(m.pain>=ms->mp)
	  {m.pain=ms->mp;Z_sound(snd[t-1][1],128);}
	if((m.pain-=5)<=ms->minp)
	  {setst(i,GO);m.pain=0;m.ammo=-9;}
	break;
   case SLEEP:
	m.initialized=1;
	if(++m.s>=18) m.s=0; else break;
	if(Z_look(mo,&pl1.o,m.d))
	  {setst(i,GO);m.aim=-1;m.atm=0;Z_sound(wakeupsnd(t),128);}
	if(_2pl) if(Z_look(mo,&pl2.o,m.d))
	  {setst(i,GO);m.aim=-2;m.atm=0;Z_sound(wakeupsnd(t),128);}
	break;
   case WAIT:
	if(--m.s<0) setst(i,GO);
	break;
   case GO:
        if(st&Z_BLOCK) {m.d^=1;setst(i,RUNOUT);m.s=40;break;}
	if(t==MN_VILE) if(iscorpse(mo,0)>=0) {
	  setst(i,ATTACK);mo->xv=0;break;
	}
	if(!Z_getobjpos(m.aim,&o) || m.atm>MAX_ATM)
	  if(!MN_findnewprey(i)) {
		m.aim=-3;
		o.x=mo->x+pt_x;o.y=mo->y+pt_y;
		o.xv=o.vx=o.yv=o.vy=o.r=0;o.h=1;
	  }else Z_getobjpos(m.aim,&o);
	  if(Z_overlap(mo,&o)) {
	    m.atm=0;
	    if(kick(i,&o)) break;
	  }
	  sx=o.x-mo->x;
	  sy=o.y-o.h/2-mo->y+mo->h/2;
	  if(!(st&Z_BLOCK)) if(bn::abs(sx)<20)
	    if(!is_fish) {setst(i,RUN);m.s=15;m.d=rand()&1;break;}
	  if(st&Z_HITWALL) {
		if(SW_press(mo->x,mo->y,mo->r,mo->h,2,i))
		  {setst(i,WAIT);m.s=4;break;}
		switch(t) {
		  case MN_CACO: case MN_SOUL: case MN_PAIN: case MN_FISH:
			break;
		  default:
			if(Z_canstand(mo->x,mo->y,mo->r))
			  {mo->yv=-ms->jv;setst(i,CLIMB);break;}
		}break;
	  }
	  m.d=(sx>0)?1:0;
	  if(canshoot(t))
		if(bn::abs(sx)>bn::abs(sy)) if(shoot(i,&o,0)) break;
	  // if(st&Z_BLOCK) {mo->xv=0;m.ap=waitanim[t-1];m.ac=0;break;}
	  switch(t) {
		case MN_FISH:
		  if(!in_water) {
		    if(Z_canstand(mo->x,mo->y,mo->r)) {
		      mo->yv=-6;
		      mo->vx+=rand()%17-8;
		    }setst(i,PAIN);m.pain+=50;break;
		  }
		case MN_CACO: case MN_SOUL: case MN_PAIN:
		  if(bn::abs(sy)>4) mo->yv=(sy<0)?-4:4; else mo->yv=0;
		  if(is_fish) if(mo->yv<0)
		    if(!Z_inwater(mo->x,mo->y-8,mo->r,mo->h))
		      {mo->yv=0;setst(i,RUN);m.d=rand()&1;m.s=20;}
		  break;
		default:
		  if(sy<-20) if(Z_canstand(mo->x,mo->y,mo->r))
			if(!(rand()&3)) mo->yv=-ms->jv;
	  }
	if(++m.s>=8) {
	  m.s=0;
	  if(!(rand()&7)) Z_sound(snd[t-1][0],128);
	}
	mo->xv=(m.d?1:-1)*ms->rv;
	if(in_water) mo->xv/=2; else if(is_fish) mo->xv=0;
	break;
   case RUN:
        if(st&Z_BLOCK) {setst(i,RUNOUT);m.d^=1;m.s=40;break;}
	if(--m.s<=0 || ((st&Z_HITWALL) && mo->yv+mo->vy==0)) {
	  setst(i,GO);m.s=0;if(st&(Z_HITWALL|Z_BLOCK)) m.d^=1;
	  if(!(rand()&7)) Z_sound(snd[t-1][0],128);
	}
	mo->xv=(m.d?1:-1)*ms->rv;
	if(in_water) mo->xv/=2; else if(is_fish) mo->xv=0;
	break;
   case RUNOUT:
        if(!(st&Z_BLOCK) && m.s>0) m.s=0;
	if(--m.s<=-18) {
	  setst(i,GO);m.s=0;if(st&(Z_HITWALL|Z_BLOCK)) m.d^=1;
	  if(!(rand()&7)) Z_sound(snd[t-1][0],128);
	}
	mo->xv=(m.d?1:-1)*ms->rv;
	if(in_water) mo->xv/=2; else if(is_fish) mo->xv=0;
	break;
   case CLIMB:
	if(mo->yv+mo->vy>=0 || !(st&Z_HITWALL)) {
	  setst(i,GO);m.s=0;
	  if(st&(Z_HITWALL|Z_BLOCK)) {m.d^=1;setst(i,RUN);m.s=15;}
	}
	mo->xv=(m.d?1:-1)*ms->rv;
	if(in_water) mo->xv/=2; else if(is_fish) mo->xv=0;
	break;
   case ATTACK:
   case SHOOT:
	if(t==MN_SOUL) {if(st&(Z_HITWALL|Z_HITCEIL|Z_HITLAND)) setst(i,GO); break;}
	if(!is_fish) mo->xv=Z_dec(mo->xv,1);
	if(t==MN_VILE && m.st==SHOOT) {
          if(!Z_getobjpos(m.aim,&o)) {setst(i,GO);break;}
          if(!Z_look(mo,&o,m.d)) {setst(i,GO);break;}
          if(Z_inwater(o.x,o.y,o.r,o.h)) {setst(i,GO);break;}
          m.tx=o.x;m.ty=o.y;
          Z_hitobj(m.aim,2,i,HIT_SOME);
	}break;
  }
  if(m.st==REVIVE) {
    if(--m.ac==0) setst(i,GO);
  }else ++m.ac;
  if(!m.ap[m.ac]) switch(m.st) {
	case ATTACK:
	  switch(t) {
		case MN_SOUL: m.ac=0;
		case MN_IMP:
		case MN_DEMON:
		  if(Z_hit(mo,15,i,HIT_SOME)) if(t==MN_SOUL) setst(i,GO);
		  break;
		case MN_FISH:
		  if(Z_hit(mo,10,i,HIT_SOME))
		    Z_sound(snd[MN_FISH-1][2],128);
		  break;
		case MN_SKEL: case MN_ROBO:
		  o=*mo;o.xv=m.d?50:-50;
		  if(Z_hit(&o,50,i,HIT_SOME)) Z_sound(pchsnd,128);
		  break;
		case MN_VILE:
		  sx=iscorpse(mo,1);
		  if(sx==-3) break;
		  if(!mn[sx].t || mn[sx].st!=DEAD) break;
            	  setst(sx,REVIVE);Z_sound(slopsnd,128);
		  hit_xv=hit_yv=0;MN_hit(i,5,-3,HIT_SOME);
		  break;
	  }if(t!=MN_SOUL && m.st!=DIE) setst(i,GO);
	  break;
	case SHOOT:
	  switch(t) {
		case MN_IMP:
		  WP_ball1(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_ZOMBY:
		  WP_pistol(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_SERG:
		  WP_shotgun(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_MAN:
		  WP_dshotgun(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i);
		  m.ammo=-36;break;
		case MN_CYBER:
		  WP_rocket(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_SKEL:
		  WP_revf(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i,m.aim);
		  break;
		case MN_CGUN:
		case MN_SPIDER:
		  WP_mgun(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_BSP:
		  WP_aplasma(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_ROBO:
		  WP_plasma(mo->x+(m.d*2-1)*15,mo->y-30,m.tx,m.ty,i);
		  break;
		case MN_MANCUB:
		  WP_manfire(mo->x+(m.d*2-1)*mo->r,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_BARON: case MN_KNIGHT:
		  WP_ball7(mo->x,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_CACO:
		  WP_ball2(mo->x,mo->y-mo->h/2,m.tx,m.ty,i);
		  break;
		case MN_PAIN:
		  if((sx=MN_spawn(mo->x,mo->y,m.d,MN_SOUL))==-1) break;
		  Z_getobjpos(mn[sx].aim=m.aim,&o);mn[sx].atm=0;
		  shoot(sx,&o,1);
		  break;
	  }
	  if(t==MN_CGUN || t==MN_SPIDER || t==MN_BSP || t==MN_MANCUB || t==MN_ROBO)
	     if(!Z_getobjpos(m.aim,&o)) MN_findnewprey(i);
		else if(shoot(i,&o,0)) break;
	  setst(i,GO);break;
	default: m.ac=0;
  }
  switch(m.st) {
	case GO: case RUN: case CLIMB: case RUNOUT:
	  if(t==MN_CYBER || t==MN_SPIDER || t==MN_BSP) {
	    if(m.ac==0 || m.ac==6) Z_sound(snd[t-1][2],128);
	  }else if(t==MN_ROBO)
	    if(m.ac==0 || m.ac==12) Z_sound(snd[t-1][2],128);
  }
  }
}

void MN_mark(void) {
  int i;

  for(i=0;i<MAXMN;++i) {
  	// if(mn[i].t!=0 && mn[i].st!=DEAD && mn[i].st!=DIE)
  	// 	BM_mark(&mn[i].o,BM_MONSTER);
  	// Optimization:
  	mn_t *const m = &mn[i];
  	if(m->t!=0 && m->st!=DEAD && m->st!=DIE) {
  		BM_mark(&m->o,BM_MONSTER);
  	}
  }
}

void MN_draw(void) {
  int i;

  for(i=0;i<MAXMN;++i) if(mn[i].t) {
	const obj_t *const mo = &mn[i].o;
	if(!R_obj_in_view(mo->x,mo->y,mo->r,mo->h)) continue;
	if(mn[i].t>=MN_PL_DEAD) {
	  if(mn[i].t>=MN_PL_HEAD1) {
	    Z_drawmanspr(mn[i].o.x,mn[i].o.y,gspr[mn[i].t-MN_PL_HEAD1][mn[i].s>>5],0,mn[i].d);
	  } else {
	    Z_drawmanspr(mn[i].o.x,mn[i].o.y,pl_spr[mn[i].t-MN_PL_DEAD],0,mn[i].d);
	  }
	  continue;
	}
	if((mn[i].t!=MN_SOUL && mn[i].t!=MN_PAIN) || mn[i].st!=DEAD) {
	  if(mn[i].t!=MN_MAN)
	    Z_drawspr(mn[i].o.x,mn[i].o.y,
		spr[mn[i].t-1][(mn[i].ap[mn[i].ac]-'A')*2+mn[i].d],
		sprd[mn[i].t-1][(mn[i].ap[mn[i].ac]-'A')*2+mn[i].d]);
	  else{
	    if(mn[i].ap[mn[i].ac]=='E' || mn[i].ap[mn[i].ac]=='F')
	      Z_drawspr(mn[i].o.x,mn[i].o.y,sgun[mn[i].ap[mn[i].ac]-'E'],mn[i].d);
	    Z_drawmanspr(mn[i].o.x,mn[i].o.y,
		spr[mn[i].t-1][(mn[i].ap[mn[i].ac]-'A')*2+mn[i].d],
		sprd[mn[i].t-1][(mn[i].ap[mn[i].ac]-'A')*2+mn[i].d],MANCOLOR);
	  }
	}
	if(mn[i].t==MN_VILE && mn[i].st==SHOOT)
	  if(R_obj_in_view(mn[i].tx,mn[i].ty,8,8))
	    Z_drawspr(mn[i].tx,mn[i].ty,fspr[mn[i].ac/3],0);
  }
}

static int MN_hit(int n,int d,int o,int t) {
  int i;

  if(mn[n].st==DEAD || mn[n].st==DIE) return 0;
  if(o==n) {
	if(t!=HIT_ROCKET && t!=HIT_ELECTRO) return 0;
	if(mn[n].t==MN_CYBER || mn[n].t==MN_BARREL) return 1;
  }
  if(o>=0) {
	if(mn[o].t==MN_SOUL && mn[n].t==MN_PAIN) return 0;
	if(mn[o].t==mn[n].t) switch(mn[n].t) {
	  case MN_IMP: case MN_DEMON:
	  case MN_BARON: case MN_KNIGHT:
	  case MN_CACO: case MN_SOUL:
	  case MN_MANCUB: case MN_SKEL:
	  case MN_FISH:
		return 0;
	}
  }
  if(t==HIT_FLAME) if(mn[n].ftime && mn[n].fobj==o) {if(g_time&31) return 1;}
    else {mn[n].ftime=255;mn[n].fobj=o;}
  if(t==HIT_ELECTRO) if(mn[n].t==MN_FISH)
    {setst(n,RUN);mn[n].s=20;mn[n].d=rand()&1;return 1;}
  if(t==HIT_TRAP) mn[n].life=-100;
  if(mn[n].t==MN_ROBO) d=0;
  if((mn[n].life-=d)<=0) --mnum;
  if(!mn[n].pain) mn[n].pain=3;
  mn[n].pain+=d;
  if(mn[n].st!=PAIN) {
	if(mn[n].pain>=mnsz[mn[n].t].minp) setst(n,PAIN);
  }
  if(mn[n].t!=MN_BARREL) {
	  DOT_blood(mn[n].o.x,mn[n].o.y-mn[n].o.h/2,hit_xv,hit_yv,d*2);
  }
  mn[n].aim=o;mn[n].atm=0;
  if(mn[n].life<=0) {
	if(mn[n].t!=MN_BARREL)
	  if(o==-1) ++pl1.kills;
	  else if(o==-2) ++pl2.kills;
	setst(n,DIE);
	switch(mn[n].t) {
	  case MN_ZOMBY: i=I_CLIP;break;
	  case MN_SERG: i=I_SGUN;break;
	  case MN_CGUN: i=I_MGUN;break;
	  case MN_MAN: i=I_KEYR;break;
	  default: i=0;
	}if(i) IT_spawn(mn[n].o.x,mn[n].o.y,i);
	mn[n].o.xv=0;mn[n].o.h=6;//mn[n].o.r=8;
	if(mn[n].life<=-mnsz[mn[n].t].sp)
	  switch(mn[n].t) {
		case MN_IMP: case MN_ZOMBY: case MN_SERG: case MN_CGUN:
		case MN_MAN:
		  mn[n].ap=slopanim[mn[n].t-1];
		  Z_sound(slopsnd,128);
		  break;
		case MN_BSP: if(g_map==9) break;
		default:
		  Z_sound(dthsnd(mn[n].t),128);
	  }
	else if(mn[n].t!=MN_BSP || g_map!=9) Z_sound(dthsnd(mn[n].t),128);
	mn[n].life=0;
  }else if(mn[n].st==SLEEP) {setst(n,GO);mn[n].pain=mnsz[mn[n].t].mp;}
  return 1;
}

#define hit(o,x,y) (y<=o.y && y>o.y-o.h && x>=o.x-o.r && x<=o.x+o.r)

BN_CODE_IWRAM int Z_gunhit(int x,int y,int o,int xv,int yv,int lx,int ly,int sx,int sy) {
  int i,cxl,cyl;
  obj_t *m;

  if(o!=-1) if(hit(pl1.o,x,y)) if(PL_hit(&pl1,3,o,HIT_SOME))
    {pl1.o.vx+=xv;pl1.o.vy+=yv;return -1;}
  if(_2pl && o!=-2) if(hit(pl2.o,x,y)) if(PL_hit(&pl2,3,o,HIT_SOME))
    {pl2.o.vx+=xv;pl2.o.vy+=yv;return -2;}
  cxl=(x>>5)<<5; cyl=(y>>5)<<5;
  for(i=0;i<MAXMN;++i) if(mn[i].t && mn[i].st!=DEAD && mn[i].st!=DIE && o!=i) {
    m=&mn[i].o;
    if(m->x+m->r<cxl || m->x-m->r>cxl+31) continue;
    if(m->y<cyl || m->y-m->h>=cyl+32) continue;
    if(sx==1 && m->x+m->r<lx) continue;
    if(sx==-1 && m->x-m->r>lx) continue;
    if(sy==1 && m->y<ly) continue;
    if(sy==-1 && m->y-m->h>ly) continue;
    if(hit(mn[i].o,x,y)) if(MN_hit(i,3,o,HIT_SOME))
      {m->vx+=xv;m->vy+=yv;return 1;}
  }
  return 0;
}

static void goodsnd(void) {
  if(!g_dm) return;
  gsndt=18;
}

int Z_hit(obj_t *o,int d,int own,int t) {
  int i;

  hit_xv=o->xv+o->vx;
  hit_yv=o->yv+o->vy;
  if(Z_overlap(o,&pl1.o)) if(PL_hit(&pl1,d,own,t)) {
	pl1.o.vx+=(o->xv+o->vx)*((t==HIT_BFG)?8:1)/4;
	pl1.o.vy+=(o->yv+o->vy)*((t==HIT_BFG)?8:1)/4;
	if(t==HIT_BFG) goodsnd();
	return -1;
  }
  if(_2pl) if(Z_overlap(o,&pl2.o)) if(PL_hit(&pl2,d,own,t)) {
	pl2.o.vx+=(o->xv+o->vx)*((t==HIT_BFG)?8:1)/4;
	pl2.o.vy+=(o->yv+o->vy)*((t==HIT_BFG)?8:1)/4;
	if(t==HIT_BFG) goodsnd();
	return -2;
  }
//  if(!mnum) return 0;
  for(i=0;i<MAXMN;++i) if(mn[i].t)
    if(Z_overlap(o,&mn[i].o)) if(MN_hit(i,d,own,t)) {
	  mn[i].o.vx+=(o->xv+o->vx)*((t==HIT_BFG)?8:1)/4;
	  mn[i].o.vy+=(o->yv+o->vy)*((t==HIT_BFG)?8:1)/4;
	  return 1;
    }
  return 0;
}

void MN_killedp(void) {
  int i;

  for(i=0;i<MAXMN;++i) if(mn[i].t==MN_MAN)
    if(mn[i].st!=DEAD && mn[i].st!=DIE && mn[i].st!=SLEEP)
      Z_sound(trupsnd,128);
}

int Z_hitobj(int obj,int d,int own,int t) {
  hit_xv=hit_yv=0;
  if(obj==-1) return PL_hit(&pl1,d,own,t);
  else if(obj==-2 && _2pl) return PL_hit(&pl2,d,own,t);
  else if(obj<0 || obj>=MAXMN) return 0;
  if(mn[obj].t) return MN_hit(obj,d,own,t);
  return 0;
}

void Z_explode(int x,int y,int rad,int own) {
  long r;
  int dx,dy,m,i;

  if(x<-100 || x>FLDW*CELW+100) return;
  if(y<-100 || y>FLDH*CELH+100) return;
  r=(long)rad*rad;
  dx=pl1.o.x-x;dy=pl1.o.y-pl1.o.h/2-y;
  if((long)dx*dx+(long)dy*dy<r) {
    if(!(m=bn::max(bn::abs(dx),bn::abs(dy)))) m=1;
	pl1.o.vx+=hit_xv=dx*10/m;
	pl1.o.vy+=hit_yv=dy*10/m;
	PL_hit(&pl1,100*(rad-m)/rad,own,HIT_ROCKET);
  }
  if(_2pl) {
    dx=pl2.o.x-x;dy=pl2.o.y-pl2.o.h/2-y;
    if((long)dx*dx+(long)dy*dy<r) {
      if(!(m=bn::max(bn::abs(dx),bn::abs(dy)))) m=1;
	  pl2.o.vx+=hit_xv=dx*10/m;
	  pl2.o.vy+=hit_yv=dy*10/m;
      PL_hit(&pl2,100*(rad-m)/rad,own,HIT_ROCKET);
    }
  }
  for(i=0;i<MAXMN;++i) if(mn[i].t) {
    dx=mn[i].o.x-x;dy=mn[i].o.y-mn[i].o.h/2-y;
    if((long)dx*dx+(long)dy*dy<r) {
      if(!(m=bn::max(bn::abs(dx),bn::abs(dy)))) m=1;
	  mn[i].o.vx+=hit_xv=dx*10/m;
	  mn[i].o.vy+=hit_yv=dy*10/m;
	  MN_hit(i,mn[i].o.r*10*(rad-m)/rad,own,HIT_ROCKET);
    }
  }
}

void Z_bfg9000(int x,int y,int own) {
  int dx,dy,i;

  hit_xv=hit_yv=0;
  if(x<-100 || x>FLDW*CELW+100) return;
  if(y<-100 || y>FLDH*CELH+100) return;
  dx=pl1.o.x-x;dy=pl1.o.y-pl1.o.h/2-y;
  if(own!=-1) if((long)dx*dx+(long)dy*dy<16000)
   if(Z_cansee(x,y,pl1.o.x,pl1.o.y-pl1.o.h/2)) {
    if(PL_hit(&pl1,50,own,HIT_SOME)) {
	    WP_bfghit(pl1.o.x,pl1.o.y-pl1.o.h/2,own);
    }
  }
  if(_2pl) {
	  dx = pl2.o.x - x;
	  dy = pl2.o.y - pl2.o.h / 2 - y;
	  if (own != -2) {
		  if ((long) dx * dx + (long) dy * dy < 16000) {
			  if (Z_cansee(x, y, pl2.o.x, pl2.o.y - pl2.o.h / 2)) {
				  if (PL_hit(&pl2, 50, own, HIT_SOME)) {
					  WP_bfghit(pl2.o.x, pl2.o.y - pl2.o.h / 2, own);
				  }
			  }
		  }
	  }
  }
//  if(!mnum) return;
  for(i=0;i<MAXMN;++i)
	  if (mn[i].t && own != i) {
		  dx = mn[i].o.x - x;
		  dy = mn[i].o.y - mn[i].o.h / 2 - y;
		  if ((long) dx * dx + (long) dy * dy < 16000) {
			  if (Z_cansee(x, y, mn[i].o.x, mn[i].o.y - mn[i].o.h / 2)) {
				  if (MN_hit(i, 50, own, HIT_SOME)) {
					  WP_bfghit(mn[i].o.x,mn[i].o.y-mn[i].o.h/2,own);
				  }
			  }
		  }
	  }
}

int Z_chktrap(int t,int d,int o,int ht) {
  int i,s;

  hit_xv=hit_yv=0;
  s=0;
  if(Z_istrapped(pl1.o.x,pl1.o.y,pl1.o.r,pl1.o.h)) {
    s=1;
	if(t) PL_hit(&pl1,d,o,ht);
  }
  if(_2pl) if(Z_istrapped(pl2.o.x,pl2.o.y,pl2.o.r,pl2.o.h)) {
	s=1;
	if(t) PL_hit(&pl2,d,o,ht);
  }
//  if(!mnum) return s;
  for(i=0;i<MAXMN;++i) if(mn[i].t && mn[i].st!=DEAD)
    if(Z_istrapped(mn[i].o.x,mn[i].o.y,mn[i].o.r,mn[i].o.h)) {
      s=1;
	  if(t) MN_hit(i,d,o,ht);
	}
  return s;
}

void Z_teleobj(int o,int x,int y) {
  obj_t *p;

  if(o==-1) p=&pl1.o;
  else if(o==-2) p=&pl2.o;
  else if(o>=0 && o<MAXMN) p=&mn[o].o;
  else return;
  FX_tfog(p->x,p->y);FX_tfog(x,y);
  p->x=x;
  p->y=y;
  p->xv=p->yv=p->vx=p->vy=0;
  Z_sound(telesnd,128);
}

void MN_warning(int l,int t,int r,int b) {
  int i;

  for(i=0;i<MAXMN;++i) if(mn[i].t && mn[i].t!=MN_CACO && mn[i].t!=MN_SOUL
      && mn[i].t!=MN_PAIN && mn[i].t!=MN_FISH)
    if(mn[i].st!=DIE && mn[i].st!=DEAD && mn[i].st!=SLEEP)
      if(mn[i].o.x+mn[i].o.r>=l && mn[i].o.x-mn[i].o.r<=r
      && mn[i].o.y>=t && mn[i].o.y-mn[i].o.h<=b)
        if(Z_canstand(mn[i].o.x,mn[i].o.y,mn[i].o.r))
          mn[i].o.yv=-mnsz[mn[i].t].jv;
}

obj_t *MN_getobj(int i) {
  if(i<0 || i>=MAXMN) return NULL;
  if(!mn[i].t) return NULL;
  return &mn[i].o;
}
