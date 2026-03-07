#ifndef DOOM2D_WEAPONS_H
#define DOOM2D_WEAPONS_H
// Weapons

#define MAXWPN 300

void WP_init(void);
void WP_alloc(void);
void WP_act(void);
void WP_draw(void);
void WP_punch(int,int,int,int);
int WP_chainsaw(int,int,int,int);
void WP_gun(int,int,int,int,int,int);
void WP_pistol(int,int,int,int,int);
void WP_mgun(int,int,int,int,int);
void WP_rocket(int,int,int,int,int);
void WP_revf(int,int,int,int,int,int);
void WP_plasma(int,int,int,int,int);
void WP_ball1(int,int,int,int,int);
void WP_ball2(int,int,int,int,int);
void WP_ball7(int,int,int,int,int);
void WP_aplasma(int,int,int,int,int);
void WP_manfire(int,int,int,int,int);
void WP_bfgshot(int,int,int,int,int);
void WP_bfghit(int,int,int);
void WP_shotgun(int,int,int,int,int);
void WP_dshotgun(int,int,int,int,int);
void WP_ognemet(int x,int y,int xd,int yd,int xv,int yv,int o);

#endif //DOOM2D_WEAPONS_H