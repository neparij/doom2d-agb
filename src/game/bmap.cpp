#include "glob.h"
#include "view.h"
#include "bmap.h"

#include "bn_memory.h"

unsigned char fld_need_remap=1;

BN_DATA_EWRAM unsigned char bmap[FLDH_QUARTER][FLDW_QUARTER];

BN_CODE_IWRAM void BM_mark(obj_t *o,unsigned char f) {
    int x,y;
    int xs,ys,xe,ye;

    if((xs=(o->x-o->r)>>5)<0) xs=0;
    if((xe=(o->x+o->r)>>5)>=FLDW_QUARTER) xe=FLDW_QUARTER-1;
    if((ys=(o->y-o->h)>>5)<0) ys=0;
    if((ye=o->y>>5)>=FLDH_QUARTER) ye=FLDH_QUARTER-1;
    for(y=ys;y<=ye;++y)
        for(x=xs;x<=xe;++x)
            bmap[y][x]|=f;
}
