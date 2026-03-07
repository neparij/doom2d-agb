#ifndef DOOM2D_MENU_H
#define DOOM2D_MENU_H
// Game menus

typedef struct{
    unsigned char type;
    int n,cur,x;
    char *ttl;
    char **m;
    unsigned char *t;
}menu_t;

void GMV_say(char *);
void GMV_stop(void);

void GM_init(void);
int GM_act(void);
int GM_draw(void);

void G_code(void);

void G_keyf(unsigned); // expose keycodes for menu
extern menu_t *mnu;
extern int last_bg_st;
extern bool inter_stats_drawn;
/** Блокировка управления на 1 кадр при закрытии меню. */
extern int menu_input_block_frames;
extern unsigned short keybuf[32];

#endif //DOOM2D_MENU_H