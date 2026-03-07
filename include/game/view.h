#ifndef DOOM2D_VIEW_H
#define DOOM2D_VIEW_H
// View functions

// object data structure
typedef struct{
    int x,y;		// coordinates
    int xv,yv;		// velocity
    int vx,vy;
    int r,h;		// radius, height
}obj_t;

typedef struct{
    int x,y;
    unsigned char d;
}pos_t;

enum{HIT_SOME,HIT_ROCKET,HIT_BFG,HIT_TRAP,HIT_WATER,HIT_ELECTRO,HIT_FLAME};

enum{
    GS_TITLE,GS_GAME,GS_INTER,GS_DARKEN,GS_ENDANIM,GS_END2ANIM,GS_ENDSCR,
    GS_BVIDEO,GS_EVIDEO,GS_END3ANIM
  };

#define FLDW 100
#define FLDH 100
#define FLDW_HALF (FLDW/2)
#define FLDH_HALF (FLDH/2)
#define FLDW_QUARTER (FLDW/4)
#define FLDH_QUARTER (FLDH/4)
#define CELW 8
#define CELH 8
#define MAXTXW 16
#define MAXTXH 8

#pragma pack(1)

typedef struct{
    char n[8];
    char t;
}wall_t;

#pragma pack()

void W_draw(void);
void W_act(void);

void W_init(void);

int W_load(const unsigned char* p, unsigned size);
int G_load(const unsigned char* p, unsigned size);
/** Пересобрать маску неба по fldb/fldf. Вызывать после любого изменения fldb или fldf. */
void build_sky_mask(void);

void G_init(void);
void G_start(void);
/** TEST ONLY: start level with guaranteed clean player state (PL_reset + G_start). Remove when menu exists. */
void G_start_test_level(void);
void G_act(void);
void G_draw(void);

extern unsigned char walswp[256];
extern unsigned char g_bot,_2pl,g_dm,g_st,g_exit,w_horiz,g_map;
/** Set by menu when NEW GAME selected; G_act runs G_start on next frame (so sound plays first). */
extern int g_pending_newgame;
extern int g_sttm;
extern unsigned int g_time;
extern int w_ht;
extern int w_ht_half;
extern int w_o,w_x,w_y;
/** Выровненная по 4 пикселя камера для отрисовки (DMA); canvas_offset = w_x - w_x_draw. */
extern int w_x_draw;
extern unsigned char fldb[FLDH][FLDW];
extern unsigned char fldf[FLDH][FLDW];
extern unsigned char fld[FLDH][FLDW];
extern pos_t dm_pos[];
extern int dm_pnum,dm_pl1p,dm_pl2p;
extern void* sky_ptr;

#endif //DOOM2D_VIEW_H