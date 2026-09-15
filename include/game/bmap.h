#ifndef DOOM2D_BMAP_H
#define DOOM2D_BMAP_H
#include "bn_memory.h"
// Block map

#define BM_WALL		1
#define BM_PLR1		2
#define BM_PLR2		4
#define BM_MONSTER	8

void BM_clear(unsigned char f);
void BM_mark(obj_t *o,unsigned char f);
void BM_remapfld(void);
/** Пометить прямоугольник ячеек fld [x0,x1)×[y0,y1) для частичного BM_remapfld. */
void BM_dirty_fld(int x0, int y0, int x1, int y1);

extern BN_DATA_EWRAM unsigned char bmap[FLDH/4][FLDW/4];
extern unsigned char fld_need_remap;

#endif //DOOM2D_BMAP_H