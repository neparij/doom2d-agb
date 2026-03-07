#ifndef DOOM2D_ITEMS_H
#define DOOM2D_ITEMS_H
// Items

#define MAXITEM 300

enum{I_NONE,I_CLIP,I_SHEL,I_ROCKET,I_CELL,I_AMMO,I_SBOX,I_RBOX,I_CELP,
  I_STIM,I_MEDI,I_BPACK,I_CSAW,I_SGUN,I_SGUN2,I_MGUN,I_LAUN,I_PLAS,I_BFG,
  I_ARM1,I_ARM2,I_MEGA,I_INVL,I_AQUA,I_KEYR,I_KEYG,I_KEYB,I_SUIT,I_SUPER,
  I_RTORCH,I_GTORCH,I_BTORCH,I_GOR1,I_FCAN,I_GUN2
};

void IT_init(void);
void IT_alloc(void);
int IT_load(const unsigned char* p, unsigned size);
void IT_act(void);
void IT_draw(void);
void IT_spawn(int x,int y,int i);
void IT_drop_ammo(int i,int n,int x,int y);

#pragma pack(1)
typedef struct{
    short x;
    short y;
    short t;
    unsigned short f;
}thing_t;
#pragma pack()

extern int itm_rtime;

#endif //DOOM2D_ITEMS_H