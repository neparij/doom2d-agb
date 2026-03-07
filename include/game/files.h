#ifndef DOOM2D_FILES_H
#define DOOM2D_FILES_H
// File operations

#define MAX_WADS 20
#define MAX_WAD 2000
#include <cstdint>

#include "os.h"

typedef char wadname[8];

typedef struct {
    int o,l;
    char n[8];
}wad_t;

typedef struct {
    int o,l;
    char n[8];
    int f;
}mwad_t;

/** Чтение из ROM: по глобальному индексу лумпа r возвращаем данные из директории WAD (без копирования в RAM). */
int F_wad_get_f(int r);
unsigned F_wad_get_o(int r);
unsigned F_wad_get_l(int r);
void F_wad_get_name(int r, char n[8]);

void F_startup(void);
void F_addwad(const char *fn, const unsigned char *data, unsigned size);
void F_initwads(void);
void F_allocres(void);
//void F_preload(void);
void F_readstr(int,char *,int);
void F_readstrz(int,char *,int);
void F_loadres(int,void *,unsigned int,unsigned int);
void F_loadpal(int,void *,unsigned int,unsigned int);
int F_getresid(char *);
void F_getresname(char *,int);
int F_findres(char *);
int F_getsprid(char[4],int,int);
int F_getreslen(int);
void F_set_snddrv(void);
void F_loadmap(char[8]);
void F_loadmus(char[8]);
void F_freemus(void);

extern char wads[MAX_WADS][_MAX_PATH];
extern int wad_num;
extern const uint8_t* wad_data[MAX_WADS];
extern unsigned wad_size[MAX_WADS];

#endif //DOOM2D_FILES_H