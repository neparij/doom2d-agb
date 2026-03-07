#ifndef DOOM2D_SWITCH_H
#define DOOM2D_SWITCH_H
// Switches

enum{
    SW_NONE,SW_EXIT,SW_EXITS,SW_OPENDOOR,SW_SHUTDOOR,SW_SHUTTRAP,
    SW_DOOR,SW_DOOR5,SW_PRESS,SW_TELE,SW_SECRET,SW_LIFTUP,SW_LIFTDOWN,SW_TRAP,
    SW_LIFT
  };

void SW_init(void);
void SW_alloc(void);
int SW_load(const unsigned char* p, unsigned size);
void SW_act(void);
int SW_press(int x,int y,int r,int h,unsigned char t,int o);

void SW_cheat_open(void);

#endif //DOOM2D_SWITCH_H