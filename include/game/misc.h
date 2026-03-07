#ifndef DOOM2D_MISC_H
#define DOOM2D_MISC_H
#include "sound.h"

// Miscellaneous functions

#define MAXDIST 2000000L

enum{
    Z_HITWALL=1,Z_HITCEIL=2,Z_HITLAND=4,Z_FALLOUT=8,
    Z_INWATER=16,Z_HITWATER=32,Z_HITAIR=64,Z_BLOCK=128
  };

void *Z_getspr(char[4],int,int,char *);
void *Z_getspr_ewram(char[4],int,int,char *,void *);
pcm_ref_t Z_getsnd(char[6]);
void Z_drawspr(int,int,void *,char);
void Z_drawmanspr(int,int,void *,char,unsigned char);
void Z_drawstair(int);
void Z_drawstprcnt(int,int);
void Z_drawstnum(int);
void Z_drawstwpn(int,int);
int Z_sound(pcm_ref_t&,int);
void Z_initst(void);
void Z_drawfld(unsigned char *);
int Z_sign(int);
int Z_dec(int,int);
int Z_canstand(int,int,int);
int Z_canfit(int x,int y,int r,int h);
void Z_teleobj(int o,int x,int y);
int Z_moveobj(obj_t *);
int Z_what_cpu(void);
int Z_gunhit(int x,int y,int o,int xv,int yv,int lx,int ly,int sx,int sy);
int Z_overlap(obj_t *,obj_t *);
int Z_look(obj_t *,obj_t *,int);
int Z_hit(obj_t *,int,int,int);
int Z_hitobj(int id,int d,int o,int t);
void Z_kickobj(obj_t *,int,int,int);
void Z_explode(int,int,int,int);
void Z_bfg9000(int,int,int);
int Z_cansee(int,int,int,int);
int Z_chktrap(int,int d,int o,int t);
int Z_istrapped(int,int,int,int);
int Z_inwater(int x,int y,int r,int h);
int Z_canbreathe(int x,int y,int r,int h);
int Z_getobjpos(int,obj_t *o);

void Z_water_trap(obj_t *);
void Z_untrap(unsigned char);

void Z_splash(obj_t *,int n);

void Z_set_speed(obj_t *,int);

void Z_calc_time(unsigned int t,unsigned short *h,unsigned short *m,unsigned short *s);

void Z_putbfch(int);
void Z_printbf(char *,...);
void Z_putsfch(int);
void Z_printsf(char *,...);
void Z_gotoxy(int,int);

#endif //DOOM2D_MISC_H