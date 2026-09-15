#ifndef DOOM2D_GLOB_H
#define DOOM2D_GLOB_H
// Globals

#ifndef NULL
#define NULL 0
#endif

#define ON 1
#define OFF 0
#define TRUE 1
#define FALSE 0

// TODO: maybe change to tonc random
// Portable PRNG (no libc rand); see rnd_max in misc.cpp
unsigned rnd_max(unsigned n);
#define random(n) (rnd_max(n))

void logo_init(void);
void logo(const char *s);
void logo_i(const char *s, int n);
void logo_gas(int cur, int all);

extern int gamma;
extern int snd_card;
extern int _cpu;

extern unsigned char g_st; // Exposing game state for rendering hacks.

#endif //DOOM2D_GLOB_H