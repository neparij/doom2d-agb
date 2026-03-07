#ifndef DOOM2D_DOTS_H
#define DOOM2D_DOTS_H
// Dots

// #define MAXDOT 400
#define MAXDOT 64// !GBALIMIT

void DOT_init(void);
void DOT_alloc(void);
void DOT_act(void);
void DOT_draw(void);
void DOT_add(int x,int y,char xv,char yv,unsigned char color,unsigned char time);
void DOT_blood(int,int,int,int,int);
void DOT_spark(int,int,int,int,int);
void DOT_water(int,int,int,int,int,int);

#endif //DOOM2D_DOTS_H