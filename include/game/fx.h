#ifndef DOOM2D_FX_H
#define DOOM2D_FX_H
// Effects

// #define MAXFX 300
#define MAXFX 64 // !GBALIMIT

void FX_init(void);
void FX_alloc(void);
void FX_act(void);
void FX_draw(void);
void FX_tfog(int,int);
void FX_ifog(int,int);
void FX_bubble(int x,int y,int xv,int yv,int n);

#endif //DOOM2D_FX_H