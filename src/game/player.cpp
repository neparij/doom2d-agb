#include "glob.h"
// #include <stdlib.h>
// #include <string.h>
#include "files.h"
#include "memory.h"
// #include "vga.h"
// #include "error.h"
// #include "keyb.h"
#include "view.h"
#include "dots.h"
// #include "smoke.h"
#include "weapons.h"
#include "monster.h"
#include "fx.h"
#include "items.h"
#include "switch.h"
#include "player.h"

#include <cstring>

#include "bn_format.h"
#include "bn_log.h"
#include "misc.h"
#include "renderer.h"
#include "sound.h"

#include "bn_sprite_items_airbar.h"
#include "bn_sprite_items_keys.h"
#include "bn_sprite_items_stbar_top_0.h"
#include "bn_sprite_items_stbar_top_1.h"
#include "bn_sprite_items_stbar_top_2.h"
#include "bn_sprite_items_stbar_top_3.h"
#include "bn_sprite_items_stbar_bottom_0.h"
#include "bn_sprite_items_stbar_bottom_1.h"
#include "bn_sprite_items_stbar_bottom_2.h"
#include "bn_sprite_items_stbar_bottom_3.h"

extern int hit_xv,hit_yv;

void Z_drawstkeys(unsigned char);
void Z_drawstlives(char);

void MN_killedp(void);

#define PL_RAD 8
#define PL_HT 26

#define PL_SWUP 4
#define PL_FLYUP 4

#define PL_AIR 360
#define PL_AQUA_AIR 1091
#define PL_MAXAIR 1091

unsigned char p_immortal=0,p_fly=0;

int PL_JUMP=10,PL_RUN=8;

int wp_it[11]={0,I_CSAW,0,I_SGUN,I_SGUN2,I_MGUN,I_LAUN,I_PLAS,I_BFG,I_GUN2,0};

enum{STAND,GO,DIE,SLOP,DEAD,MESS,OUT,FALL,BLOW};

typedef void fire_f(int,int,int,int,int);

int Z_getacid(int x,int y,int r,int h);

player_t pl1,pl2;
static int aitime;
static pcm_ref_t aisnd[3];
static pcm_ref_t pdsnd[5];

static void *spr[27*2];
static pcm_ref_t snd[11];
static char sprd[27*2];
static void *wpn[11][6];
static unsigned char goanim[]="BDACDA",
   dieanim[]="HHHHIIIIJJJJKKKKLLLLMMMM",
   slopanim[]="OOPPQQRRSSTTUUVVWW";

// void PL_savegame(int h) {
//   write(h,&pl1,sizeof(pl1));
//   if(_2pl) write(h,&pl2,sizeof(pl2));
//   write(h,&PL_JUMP,4);write(h,&PL_RUN,4);write(h,&p_immortal,1);
// }
//
// void PL_loadgame(int h) {
//   read(h,&pl1,sizeof(pl1));
//   if(_2pl) read(h,&pl2,sizeof(pl2));
//   read(h,&PL_JUMP,4);read(h,&PL_RUN,4);read(h,&p_immortal,1);
// }
//
static int nonz(int a) {return (a)?a:1;}

static int firediry(player_t *p) {
  if(p->f&PLF_UP) return -42;
  if(p->f&PLF_DOWN) return 19;
  return 0;
}

static void fire(player_t *p,int k) {
  static fire_f *ff[11]={
    WP_pistol,WP_pistol,WP_pistol,WP_shotgun,WP_dshotgun,
    WP_mgun,WP_rocket,WP_plasma,WP_bfgshot,WP_shotgun,WP_pistol};
  static int ft[11]={5,2,6,18,36,2,12,2,0,2,1};

  if(p->cwpn) return;
  if(p->wpn==8) {
    if(!p->fire)
      if((k&PLK_F) && p->cell>=40)
	{Z_sound(snd[5],128);p->fire=21;p->cell-=40;p->drawst|=PL_DRAWWPN;return;}
      else return;
    if(p->fire==1) p->cwpn=12;
    else return;
  }else if(p->wpn==1) {
    if(!p->csnd) {
      if(!(k&PLK_F)) {Z_sound(snd[7],128);p->csnd=13;return;}
    }
    if((k&PLK_F) && !p->fire) {
      p->fire=2;
	  WP_chainsaw(p->o.x+((p->d)?4:-4),p->o.y,(g_dm)?9:3,p->id);
      if(!p->csnd) {Z_sound(snd[8],128);p->csnd=29;}
    }return;
  }else if(p->fire) return;
  if((k&PLK_F) || p->wpn==8) {
    switch(p->wpn) {
      case 2: case 5:
	if(!p->ammo) return;
	--p->ammo;p->drawst|=PL_DRAWWPN;break;
      case 3: case 9:
	if(!p->shel) return;
	--p->shel;p->drawst|=PL_DRAWWPN;break;
      case 4:
	if(p->shel<2) return;
	p->shel-=2;p->drawst|=PL_DRAWWPN;break;
      case 6:
	if(!p->rock) return;
	--p->rock;p->drawst|=PL_DRAWWPN;break;
      case 7:
	if(!p->cell) return;
	--p->cell;p->drawst|=PL_DRAWWPN;break;
      case 10:
	if(!p->fuel) return;
	--p->fuel;p->drawst|=PL_DRAWWPN;break;
    }
    if(p->wpn==10)
      WP_ognemet(p->o.x,p->o.y-15,p->o.x+((p->d)?30:-30),p->o.y-15+firediry(p),
        p->o.xv+p->o.vx,p->o.yv+p->o.vy,p->id);
    else if(p->wpn>=1) ff[p->wpn] (p->o.x,p->o.y-15,p->o.x+((p->d)?30:-30),
      p->o.y-15+firediry(p),p->id);
    else WP_punch(p->o.x+((p->d)?4:-4),p->o.y,3,p->id);
    p->fire=ft[p->wpn];
    if(p->wpn>=2) p->f|=PLF_FIRE;
  }
}

static void chgwpn(player_t *p,int k) {
  if(p->cwpn) return;
  if(p->fire && p->wpn!=1) return;
  if(k&PLK_WL) {
	do{ if(--p->wpn<0) p->wpn=10; }while(!(p->wpns&(1<<p->wpn)));
	p->cwpn=3;
  }else if(k&PLK_WR) {
	do{ if(++p->wpn>10) p->wpn=0; }while(!(p->wpns&(1<<p->wpn)));
	p->cwpn=3;
  }
  if(p->cwpn) {
	p->drawst|=PL_DRAWWPN;p->fire=0;
	if(p->wpn==1) Z_sound(snd[6],128);
  }
}

static void jump(player_t *p,int st,int k) {
  if(Z_canbreathe(p->o.x,p->o.y,p->o.r,p->o.h)) {
	if(p->air<PL_AIR) {p->air=PL_AIR;p->drawst|=PL_DRAWAIR;}
  }else {
	if(--p->air < -9) {
	  p->air=0;
	  PL_hit(p,10,-3,HIT_WATER);
	}else if((p->air&31)==0) {
	  FX_bubble(p->o.x,p->o.y-20,0,0,5);
	}
	p->drawst|=PL_DRAWAIR;
  }
  if(k&PLK_J) {
    if(p_fly) {
      p->o.yv=-PL_FLYUP;
    }else{
      if(Z_canstand(p->o.x,p->o.y,p->o.r)) p->o.yv=-PL_JUMP;
      else if(st&Z_INWATER) p->o.yv=-PL_SWUP;
    }
  }
}

int PL_isdead(player_t *p) {
  switch(p->st) {
	case DEAD: case MESS:
	case OUT:
	  return 1;
  }
  return 0;
}

void PL_init(void) {
  p_immortal=0;
  PL_JUMP=10;PL_RUN=8;
  aitime=0;
}

void PL_alloc(void) {
   int i,j;
  static char nm[][7]={
	"OOF",
	"PLPAIN",
	"PLDETH",
	"SLOP",
	"PDIEHI",
	"BFG",
	"SAWUP",
	"SAWIDL",
	"SAWFUL",
	"SAWHIT",
	"PLFALL"
  };
  static char s[6];

// //  logo("  players");
  for(i=0;i<27;++i) {
	spr[i*2]=Z_getspr("PLAY",i,1,sprd+i*2);
	spr[i*2+1]=Z_getspr("PLAY",i,2,sprd+i*2+1);
  }
   memcpy(s,"PWPx",4);
   for(i=1;i<11;++i) {
     s[3]=((i<10)?'0':('A'-10))+i;
     for(j=0;j<6;++j) wpn[i][j]=Z_getspr(s,j,1,NULL);
   }
   for(i=0;i<11;++i) snd[i]=Z_getsnd(nm[i]);
   memcpy(s,"AIx",4);
   for(i=0;i<3;++i) {
     s[2]=i+'1';
     aisnd[i]=Z_getsnd(s);
   }
   memcpy(s,"PLDTHx",6);
   for(i=0;i<5;++i) {
     s[5]=i+'1';
     pdsnd[i]=Z_getsnd(s);
   }
}

void PL_restore(player_t *p) {
  p->o.xv=p->o.yv=p->o.vx=p->o.vy=0;
  p->o.r=PL_RAD;p->o.h=PL_HT;
  p->pain=0;
  p->invl=p->suit=0;
  switch(p->st) {
    case DEAD: case MESS: case OUT:
    case DIE: case SLOP: case FALL:
      p->life=100;p->armor=0;p->air=PL_AIR;
      p->wpns=5;
      p->wpn=2;
      p->ammo=50;p->fuel=p->shel=p->rock=p->cell=0;
      p->amul=1;
  }
  p->st=STAND;
  p->fire=p->cwpn=p->csnd=0;
  p->f=0;
  p->drawst=0xFF;
  p->looky=0;
  p->mon=-1;
  p->keys=(g_dm)?0x70:0;
}

void PL_reset(void) {
  pl1.st=pl2.st=DEAD;
  pl1.frag=pl2.frag=0;
}

void PL_spawn(player_t *p,int x,int y,char d) {
  PL_restore(p);
  p->o.x=x;p->o.y=y;p->d=d;
  p->kills=p->secrets=0;
}

int PL_hit(player_t *p,int d,int o,int t) {
  if(!d) return 0;
  switch(p->st) {
    case DIE: case SLOP:
	case DEAD: case MESS:
    case OUT: case FALL:
      return 0;
  }
  if(t==HIT_TRAP) {if(!p_immortal) {p->armor=0;p->life=-100;}}
  else if(t!=HIT_ROCKET && t!=HIT_ELECTRO) {
    if(p->id==-1) {if(o==-1) return 0;}
	else if(o==-2) return 0;
  }
 if(t!=HIT_WATER && t!=HIT_ELECTRO)
	DOT_blood(p->o.x,p->o.y-15,hit_xv,hit_yv,d*2);
	else if(t==HIT_WATER) FX_bubble(p->o.x,p->o.y-20,0,0,d/2);
  if(p_immortal || p->invl) return 1;
  p->hit+=d;
  p->hito=o;
  return 1;
}

static obj_t *PL_itemobj(player_t *p) {
  static obj_t o;
  if(p->st!=BLOW) return &p->o;
  o.x=p->o.x;o.y=p->o.y;o.xv=o.yv=0;
  o.vx=rand()%51-25;o.vy=-rand()%25-10;
  return &o;
}

static void PL_blowup(player_t *p) {
  static obj_t o;
  static char head,body,legs,hands,guts;
  int x,y;

  o.x=p->o.x;o.y=p->o.y-(p->o.h/2);
  o.xv=o.yv=0;o.r=3;o.h=6;

  head=1;
  body=1;
  legs=random(2)+1;
  hands=random(2)+1;
  x=random(10);
  y=30+x+(2-hands)*3+(2-legs)*4;
  guts=y;
  while(head||body||legs||hands||guts>0) {
    o.vx=rand()%51-25;o.vy=-rand()%25-10;
    if(guts>0) {
      --guts;
      MN_spawn_deadpl(&o,p->color,10+random(5));
    } else if(head && body && hands && (rand()&3)==0) {
      --head;
      --body;
      --hands;
      p->mon=MN_spawn_deadpl(&o,p->color,4);
    } else if(head) {
      --head;
      p->mon=MN_spawn_deadpl(&o,p->color,2+random(2));
    } else if(legs) {
      --legs;
      MN_spawn_deadpl(&o,p->color,8+random(2));
    } else if(body) {
      --body;
      MN_spawn_deadpl(&o,p->color,5+random(2));
    } else if(hands) {
      --hands;
      MN_spawn_deadpl(&o,p->color,7);
    }
  }
}

void IT_spawnobj(obj_t *o,int t);

void PL_damage(player_t *p) {
  int i;

  if(!p->hit && p->life>0) return;
  switch(p->st) {
    case DIE: case SLOP:
	case DEAD: case MESS:
    case OUT: case FALL:
	  return;
  }
  i=p->hit*p->life/nonz(p->armor*3/4+p->life);
  p->pain+=p->hit;
  p->drawst|=PL_DRAWLIFE|PL_DRAWARMOR;
  if((p->armor-=p->hit-i)<0) {p->life+=p->armor;p->armor=0;}
  if((p->life-=i)<=0) {
    if(p->life>-30) {p->st=DIE;p->s=0;Z_sound(pdsnd[rand()%5],128);}
	else {p->st=BLOW;p->s=20;Z_sound(snd[3],128);MN_killedp();PL_blowup(p);}
//	IT_drop_ammo(I_AMMO,p->ammo,p->o.x,p->o.y);
//	IT_drop_ammo(I_SBOX,p->shel,p->o.x,p->o.y);
//	IT_drop_ammo(I_RBOX,p->rock,p->o.x,p->o.y);
//	IT_drop_ammo(I_CELP,p->cell,p->o.x,p->o.y);
	if(p->amul>1) IT_spawnobj(PL_itemobj(p),I_BPACK);
	if(!g_dm) {
	  if(p->keys&16) IT_spawnobj(PL_itemobj(p),I_KEYR);
	  if(p->keys&32) IT_spawnobj(PL_itemobj(p),I_KEYG);
	  if(p->keys&64) IT_spawnobj(PL_itemobj(p),I_KEYB);
	}
	for(i=1,p->wpns>>=1;i<11;++i,p->wpns>>=1)
	if(i!=2) if(p->wpns&1) if(wp_it[i]) IT_spawnobj(PL_itemobj(p),wp_it[i]);
	if(p->st==BLOW) p->st=FALL;
	p->wpns=5;p->wpn=2;
	p->f|=PLF_PNSND;
	p->drawst|=PL_DRAWWPN;
    if(g_dm && _2pl) {
	  if(p->id==-1) {
		if(p->hito==-2) {++pl2.kills;++pl2.frag;/*if(pl1.frag<=0) --pl1.frag;*/}
		else if(p->hito==-1) --pl1.frag;
	  }else{
	    if(p->hito==-1) {++pl1.kills;++pl1.frag;/*if(pl2.frag<=0) --pl2.frag;*/}
	    else if(p->hito==-2) --pl2.frag;
	  }
	  pl1.drawst|=PL_DRAWFRAG;
	  if(_2pl) pl2.drawst|=PL_DRAWFRAG;
	}
	p->life=0;return;
  }
  return;
}
//
void PL_cry(player_t *p) {
  Z_sound(snd[(p->pain>20)?1:0],128);
  p->f|=PLF_PNSND;
}

int PL_give(player_t *p,int t) {
  int i;

  switch(p->st) {
    case DIE: case SLOP: case FALL:
    case DEAD: case MESS: case OUT:
      return 0;
  }
  switch(t) {
    case I_STIM: case I_MEDI:
      if(p->life>=100) return 0;
      if((p->life+=((t==I_MEDI)?25:10))>100) p->life=100;
      p->drawst|=PL_DRAWLIFE;return 1;
    case I_CLIP:
      if(p->ammo>=200*p->amul) return 0;
      if((p->ammo+=10)>200*p->amul) p->ammo=200*p->amul;
      p->drawst|=PL_DRAWWPN;return 1;
    case I_AMMO:
      if(p->ammo>=200*p->amul) return 0;
      if((p->ammo+=50)>200*p->amul) p->ammo=200*p->amul;
	  p->drawst|=PL_DRAWWPN;return 1;
    case I_SHEL:
      if(p->shel>=50*p->amul) return 0;
      if((p->shel+=4)>50*p->amul) p->shel=50*p->amul;
      p->drawst|=PL_DRAWWPN;return 1;
    case I_SBOX:
      if(p->shel>=50*p->amul) return 0;
      if((p->shel+=25)>50*p->amul) p->shel=50*p->amul;
      p->drawst|=PL_DRAWWPN;return 1;
    case I_ROCKET:
      if(p->rock>=50*p->amul) return 0;
      if((++p->rock)>50*p->amul) p->rock=50*p->amul;
      p->drawst|=PL_DRAWWPN;return 1;
    case I_RBOX:
      if(p->rock>=50*p->amul) return 0;
      if((p->rock+=5)>50*p->amul) p->rock=50*p->amul;
      p->drawst|=PL_DRAWWPN;return 1;
    case I_CELL:
      if(p->cell>=300*p->amul) return 0;
      if((p->cell+=40)>300*p->amul) p->cell=300*p->amul;
      p->drawst|=PL_DRAWWPN;return 1;
    case I_CELP:
      if(p->cell>=300*p->amul) return 0;
      if((p->cell+=100)>300*p->amul) p->cell=300*p->amul;
      p->drawst|=PL_DRAWWPN;return 1;
    case I_BPACK:
      if(p->amul==1) {p->amul=2;i=1;} else i=0;
      i|=PL_give(p,I_CLIP);
      i|=PL_give(p,I_SHEL);
      i|=PL_give(p,I_ROCKET);
      i|=PL_give(p,I_CELL);
      return i;
    case I_CSAW:
      if(!(p->wpns&2)) {p->wpns|=2;p->drawst|=PL_DRAWWPN;return 1;}
      return 0;
    case I_GUN2:
      i=PL_give(p,I_SHEL);
      if(!(p->wpns&512)) {p->wpns|=512;i=1;}
      p->drawst|=PL_DRAWWPN;return i;
    case I_SGUN:
      i=PL_give(p,I_SHEL);
      if(!(p->wpns&8)) {p->wpns|=8;i=1;}
      p->drawst|=PL_DRAWWPN;return i;
    case I_SGUN2:
      i=PL_give(p,I_SHEL);
      if(!(p->wpns&16)) {p->wpns|=16;i=1;}
      p->drawst|=PL_DRAWWPN;return i;
    case I_MGUN:
      i=PL_give(p,I_AMMO);
      if(!(p->wpns&32)) {p->wpns|=32;i=1;}
      p->drawst|=PL_DRAWWPN;return i;
    case I_LAUN:
      i=PL_give(p,I_ROCKET);
      i|=PL_give(p,I_ROCKET);
      if(!(p->wpns&64)) {p->wpns|=64;i=1;}
      p->drawst|=PL_DRAWWPN;return i;
    case I_PLAS:
      i=PL_give(p,I_CELL);
      if(!(p->wpns&128)) {p->wpns|=128;i=1;}
      p->drawst|=PL_DRAWWPN;return i;
    case I_BFG:
      i=PL_give(p,I_CELL);
      if(!(p->wpns&256)) {p->wpns|=256;i=1;}
      p->drawst|=PL_DRAWWPN;return i;
    case I_ARM1:
      if(p->armor>=100) return 0;
      p->armor=100;p->drawst|=PL_DRAWARMOR;return 1;
    case I_ARM2:
      if(p->armor>=200) return 0;
      p->armor=200;p->drawst|=PL_DRAWARMOR;return 1;
    case I_MEGA:
      i=0;
      if(p->life<200) {p->life=200;p->drawst|=PL_DRAWLIFE;i=1;}
      if(p->armor<200) {p->armor=200;p->drawst|=PL_DRAWARMOR;i=1;}
      return i;
    case I_SUPER:
      if(p->life<200) {p->life=bn::min(p->life+100,200);p->drawst|=PL_DRAWLIFE;return 1;}
      return 0;
    case I_INVL:
	  p->invl=PL_POWERUP_TIME;
	  return 1;
    case I_SUIT:
	  p->suit=PL_POWERUP_TIME;
	  return 1;
    case I_AQUA:
      if(p->air >= PL_AQUA_AIR) return 0;
      p->air=PL_AQUA_AIR;p->drawst|=PL_DRAWAIR;
      return 1;
    case I_KEYR:
      if(p->keys&16) return 0;
      p->keys|=16;p->drawst|=PL_DRAWKEYS;return 1;
    case I_KEYG:
      if(p->keys&32) return 0;
      p->keys|=32;p->drawst|=PL_DRAWKEYS;return 1;
    case I_KEYB:
      if(p->keys&64) return 0;
      p->keys|=64;p->drawst|=PL_DRAWKEYS;return 1;
    default:
	  return 0;
  }
}

void PL_act(player_t *p,int k) {
	// TODO: uncomment all!!!!!!!
  int st;

  if(--aitime<0) aitime=0;
  if(p->st!=FALL && p->st!=OUT)
    SW_press(p->o.x,p->o.y,p->o.r,p->o.h,4|p->keys,p->id);
  if(!p->suit) if((g_time&15)==0)
    PL_hit(p,Z_getacid(p->o.x,p->o.y,p->o.r,p->o.h),-3,HIT_SOME);
  if(p->st!=FALL && p->st!=OUT) {
	if(((st=Z_moveobj(&p->o))&Z_FALLOUT) && p->o.y>=FLDH*CELH+w_ht/2) {
	  switch(p->st) {
		case DEAD: case MESS: case DIE: case SLOP:
		  p->s=5;break;
		default:
		  p->s=Z_sound(snd[10],128);
	          if(g_dm) --p->frag;
	  }p->st=FALL;
	}
  }else st=0;
  if(st&Z_HITWATER) Z_splash(&p->o,PL_RAD+PL_HT);
  if(p->f&PLF_FIRE) if(p->fire!=2) p->f-=PLF_FIRE;
  if(k&PLK_U) {p->f|=PLF_UP;p->looky-=5;}
  else{
    p->f&=0xFFFF-PLF_UP;
	if(k&PLK_D)
	  {p->f|=PLF_DOWN;p->looky+=5;}
	else {p->f&=0xFFFF-PLF_DOWN;p->looky=Z_dec(p->looky,5);}
  }
  if(k&PLK_P) SW_press(p->o.x,p->o.y,p->o.r,p->o.h,1|p->keys,p->id);
  if(p->fire) --p->fire;
  if(p->cwpn) --p->cwpn;
  if(p->csnd) --p->csnd;
  if(p->invl) --p->invl;
  if(p->suit) --p->suit;
  switch(p->st) {
    case DIE:
      p->o.h=7;
      if(!dieanim[++p->s]) {p->st=DEAD;MN_killedp();}
      p->o.xv=Z_dec(p->o.xv,1);
      break;
    case SLOP:
      p->o.h=6;
      if(!slopanim[++p->s]) {p->st=MESS;MN_killedp();}
      p->o.xv=Z_dec(p->o.xv,1);
      break;
	case GO:
	  chgwpn(p,k);fire(p,k);jump(p,st,k);
	  // if(p_fly)
	  //   SMK_gas(p->o.x,p->o.y-2,2,3,p->o.xv+p->o.vx,p->o.yv+p->o.vy,128); // TODO: FX
	  if((p->s+=abs(p->o.xv)/2) >= 24) p->s%=24;
	  if(!(k&PLK_L) && !(k&PLK_R)) {
		if(p->o.xv) p->o.xv=Z_dec(p->o.xv,1);
		else p->st=STAND;
		break;
	  }
	  if(p->o.xv<PL_RUN && (k&PLK_R)) {p->o.xv+=PL_RUN>>3;p->d=1;}
	  //   else if(PL_RUN>8) // TODO: FX
	  //     SMK_gas(p->o.x,p->o.y-2,2,3,p->o.xv+p->o.vx,p->o.yv+p->o.vy,32);
	  if(p->o.xv>-PL_RUN && (k&PLK_L)) {p->o.xv-=PL_RUN>>3;p->d=0;}
	  //   else if(PL_RUN>8) // TODO: FX
	  //     SMK_gas(p->o.x,p->o.y-2,2,3,p->o.xv+p->o.vx,p->o.yv+p->o.vy,32);
	  break;
	case STAND:
	  chgwpn(p,k);fire(p,k);jump(p,st,k);
	  // if(p_fly) // TODO: FX
	  //   SMK_gas(p->o.x,p->o.y-2,2,3,p->o.xv+p->o.vx,p->o.yv+p->o.vy,128);
	  if(k&PLK_L) {p->st=GO;p->s=0;p->d=0;}
      else if(k&PLK_R) {p->st=GO;p->s=0;p->d=1;}
      break;
    case DEAD:
    case MESS:
    case OUT:
	  p->o.xv=Z_dec(p->o.xv,1);
	  if(k&(PLK_U|PLK_D|PLK_L|PLK_R|PLK_F|PLK_J|PLK_P|PLK_WL|PLK_WR)) {
		if(p->st!=OUT) MN_spawn_deadpl(&p->o,p->color,(p->st==MESS)?1:0);
		PL_restore(p);
		if(g_dm) {G_respawn_player(p);break;}
		if(!_2pl) {
		  if(--p->lives==0) {G_start();break;}
		  else{p->o.x=dm_pos[0].x;p->o.y=dm_pos[0].y;p->d=dm_pos[0].d;}
		  p->drawst|=PL_DRAWLIVES;
		}
		if(p->id==-1)
		  {p->o.x=dm_pos[0].x;p->o.y=dm_pos[0].y;p->d=dm_pos[0].d;}
		else {p->o.x=dm_pos[1].x;p->o.y=dm_pos[1].y;p->d=dm_pos[1].d;}
	  }break;
	case FALL:
	  if(--p->s<=0) p->st=OUT;
	  break;
  }
}

static int standspr(player_t *p) {
  if(p->f&PLF_UP) return 'X';
  if(p->f&PLF_DOWN) return 'Z';
  return 'E';
}

static int wpnspr(player_t *p) {
  if(p->f&PLF_UP) return 'C';
  if(p->f&PLF_DOWN) return 'E';
  return 'A';
}

static int wytab[]={-1,-2,-1,0};

void PL_draw(player_t *p) {
   int s,w,wx,wy;

   s='A';w=0;wx=wy=0;
   switch(p->st) {
     case STAND:
       if(p->f&PLF_FIRE) {s=standspr(p)+1;w=wpnspr(p)+1;}
       else if(p->pain) {s='G';w='A';wx=p->d?2:-2;wy=1;}
       else {s=standspr(p);w=wpnspr(p);}
       break;
     case DEAD:
       s='N';
       break;
     case MESS:
       s='W';
       break;
     case GO:
       if(p->pain) {s='G';w='A';wx=p->d?2:-2;wy=1;}
       else {
         s=goanim[p->s/8];w=(p->f&PLF_FIRE)?'B':'A';
         wx=p->d?2:-2;wy=1+wytab[s-'A'];
       }
       break;
     case DIE:
       s=dieanim[p->s];
       break;
     case SLOP:
       s=slopanim[p->s];
       break;
     case OUT:
     case FALL:
       s=0;
       break;
   }
   if(p->wpn==0) w=0;
   if(w) Z_drawspr(p->o.x+wx,p->o.y+wy,wpn[p->wpn][w-'A'],p->d);
   if(s) Z_drawmanspr(p->o.x,p->o.y,spr[(s-'A')*2+p->d],sprd[(s-'A')*2+p->d],p->color);
}

void *PL_getspr(int s,int d,void **wi,int *wx,int *wy) {
  *wx=*wy=0;
  if(s<='D') {
    *wi=wpn[5][0];*wx=d?2:-2;*wy=1+wytab[s-'A'];
  } else {
    *wi=wpn[5][s=='F'];
  }
  return spr[(s-'A')*2+d];
}

static void chk_bfg(player_t *p,int x,int y) {
  int dx,dy;

  if(aitime) return;
  switch(p->st) {
    case DIE: case SLOP: case FALL:
    case DEAD: case MESS: case OUT:
      return;
  }
  dx=p->o.x-x;dy=p->o.y-p->o.h/2-y;
  if(dx*dx+dy*dy<=1600) {
    aitime=Z_sound(aisnd[rand()%3],128)*4;
  }
}

void bfg_fly(int x,int y,int o) {
//  if(!g_dm) return;
  if(o!=-1) chk_bfg(&pl1,x,y);
  if(_2pl) if(o!=-2) chk_bfg(&pl2,x,y);
  if(o==-1 || o==-2) MN_warning(x-50,y-50,x+50,y+50);
}

int ui_prev_life = -1, ui_prev_armor = -1, ui_prev_ammo = -1, ui_prev_air = -1;
char ui_prev_wpn = -1;
unsigned char ui_prev_keys = 0xFF, ui_prev_drawst = 0;

void PL_drawst(player_t *p) {
    if (st_gfx_sprites.empty()) {
        st_gfx_sprites.push_back(bn::sprite_items::keys.create_sprite(96, -74, 0));
        st_gfx_sprites.push_back(bn::sprite_items::keys.create_sprite(96+9, -74, 1));
        st_gfx_sprites.push_back(bn::sprite_items::keys.create_sprite(96+9+9, -74, 2));
        st_gfx_sprites.push_back(bn::sprite_items::airbar.create_sprite(0, -80));
        st_gfx_sprites.push_back(bn::sprite_items::airbar.create_sprite(0, -80));
        st_gfx_sprites.push_back(bn::sprite_items::stbar_top_0.create_sprite(-120 + 32, -80 + 16));
        st_gfx_sprites.push_back(bn::sprite_items::stbar_top_1.create_sprite(-120 + 32 + 64, -80 + 16));
        st_gfx_sprites.push_back(bn::sprite_items::stbar_top_2.create_sprite(-120 + 32 + 128, -80 + 16));
        st_gfx_sprites.push_back(bn::sprite_items::stbar_top_3.create_sprite(-120 + 32 + 192, -80 + 16));
        st_gfx_sprites.push_back(bn::sprite_items::stbar_bottom_0.create_sprite(-120 + 32, 80 - 16));
        st_gfx_sprites.push_back(bn::sprite_items::stbar_bottom_1.create_sprite(-120 + 32 + 64, 80 - 16));
        st_gfx_sprites.push_back(bn::sprite_items::stbar_bottom_2.create_sprite(-120 + 32 + 128, 80 - 16));
        st_gfx_sprites.push_back(bn::sprite_items::stbar_bottom_3.create_sprite(-120 + 32 + 192, 80 - 16));

        for (int k = 0; k < 3; ++k) {
            st_gfx_sprites[k].set_visible(false);
            st_gfx_sprites[k].set_bg_priority(1);
        }
        ui_prev_life = ui_prev_armor = ui_prev_ammo = ui_prev_air = -1;
        ui_prev_wpn = -1;
        ui_prev_keys = 0xFF;
        ui_prev_drawst = 0;
    }

    int cur_ammo = 0;
    if (p->wpn >= 2) {
        switch (p->wpn) {
            case 2: case 5: cur_ammo = p->ammo; break;
            case 3: case 4: case 9: cur_ammo = p->shel; break;
            case 6: cur_ammo = p->rock; break;
            case 10: cur_ammo = p->fuel; break;
            case 7: case 8: cur_ammo = p->cell; break;
            default: break;
        }
    }

    const bool life_changed = p->life != ui_prev_life;
    const bool armor_changed = p->armor != ui_prev_armor;
    const bool ammo_changed = (p->wpn >= 2 && (cur_ammo != ui_prev_ammo || p->wpn != ui_prev_wpn)) ||
                              (p->wpn < 2 && ui_prev_wpn >= 2);
    const unsigned char keys_val = pl1.keys >> 4;
    const bool keys_changed = keys_val != ui_prev_keys;
    const bool air_changed = p->air != ui_prev_air;

    if (!life_changed && !armor_changed && !ammo_changed && !keys_changed && !air_changed) {// && !drawst_changed) {
        return;
    }

    // const unsigned char prev_show = ui_prev_drawst;
    ui_prev_life = p->life;
    ui_prev_armor = p->armor;
    ui_prev_ammo = cur_ammo;
    ui_prev_wpn = p->wpn;
    ui_prev_keys = keys_val;
    ui_prev_air = p->air;
    // ui_prev_drawst = p->drawst;

    const unsigned char show = p->drawst;// ? p->drawst : prev_show;

    if (air_changed) {// || drawst_changed) {
        // if (show & PL_DRAWAIR) {
            const int air_x = bn::fixed(p->air).multiplication(64).division(PL_MAXAIR).right_shift_integer();
            st_gfx_sprites[3].set_visible(true);
            st_gfx_sprites[4].set_visible(true);
            st_gfx_sprites[3].set_top_left_position(8-64+air_x, 5);
            st_gfx_sprites[4].set_top_left_position(8+air_x, 3);
        // } else {
            // st_gfx_sprites[3].set_visible(false);
            // st_gfx_sprites[4].set_visible(false);
        // }
    }

    if (keys_changed) {// || drawst_changed) {
        // if (show & PL_DRAWKEYS) {
            unsigned char k = keys_val;
            for (int n = 0; n < 3; ++n, k >>= 1) {
                st_gfx_sprites[n].set_visible(k & 1);
            }
        // } else {
            // for (int n = 0; n < 3; ++n) {
                // st_gfx_sprites[n].set_visible(false);
            // }
        // }
    }

    if (life_changed || armor_changed || ammo_changed) {// || drawst_changed) {
        st_text_sprites.clear();
        text_stt->set_right_alignment();
        // if (show & PL_DRAWLIFE) {
            text_stt->generate(-58, 64, bn::format<8>("{}%", p->life), st_text_sprites);
        // }
        // if (show & PL_DRAWARMOR) {
            text_stt->generate(33, 64, bn::format<8>("{}%", p->armor), st_text_sprites);
        // }
        // if ((show & PL_DRAWWPN) && p->wpn >= 2) {
            text_stt->generate(102, 64, bn::format<8>("{}", cur_ammo), st_text_sprites);
        // }
    }
}
