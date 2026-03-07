#ifndef DOOM2D_MOSTER_H
#define DOOM2D_MOSTER_H

// Monsters

#define MAXMN 200

enum{
    MN_NONE,MN_DEMON,MN_IMP,MN_ZOMBY,MN_SERG,MN_CYBER,MN_CGUN,
    MN_BARON,MN_KNIGHT,MN_CACO,MN_SOUL,MN_PAIN,MN_SPIDER,MN_BSP,
    MN_MANCUB,MN_SKEL,MN_VILE,MN_FISH,MN_BARREL,MN_ROBO,MN_MAN,
    MN__LAST,
    MN_PL_DEAD=100,MN_PL_MESS,
    MN_PL_HEAD1,MN_PL_HEAD2,MN_PL_TORSO,MN_PL_BODY1,MN_PL_BODY2,
    MN_PL_HAND,MN_PL_LEGS1,MN_PL_LEGS2,MN_PL_GUTS1,MN_PL_GUTS2,
    MN_PL_GUTS3,MN_PL_GUTS4,MN_PL_GUTS5,
  };

void MN_init(void);
void MN_alloc(void);
/** Построить индекс монстров по ячейкам 32×32 для Z_gunhit. Вызывать в начале G_act. */
void MN_build_gunhit_index(void);
/** Проверить попадание в монстров в ячейках вокруг (cx,cy). o<0 = выстрел игрока. Возврат как Z_gunhit. */
int MN_gunhit_check_cells(int cx, int cy, int x, int y, int o, int xv, int yv);
int MN_spawn(int,int,unsigned char,int);
int MN_spawn_deadpl(obj_t *,unsigned char,int);
void MN_act(void);
void MN_mark(void);
void MN_draw(void);
void MN_warning(int l,int t,int r,int b);

// static int MN_hit(int n,int d,int o,int t);

#endif //DOOM2D_MOSTER_H