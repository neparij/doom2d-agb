#include "glob.h"
#include "view.h"
#include "bmap.h"

#include "bn_memory.h"

unsigned char fld_need_remap=1;

BN_DATA_EWRAM unsigned char bmap[FLDH_QUARTER][FLDW_QUARTER];

static int bm_dx0, bm_dy0, bm_dx1, bm_dy1;
static unsigned char bm_full=1;

void BM_dirty_fld(int x0, int y0, int x1, int y1) {
    if(x0<0) x0=0;
    if(y0<0) y0=0;
    if(x1>FLDW) x1=FLDW;
    if(y1>FLDH) y1=FLDH;
    if(x0>=x1 || y0>=y1) return;
    fld_need_remap=1;
    if(bm_full) return;
    if(bm_dx0>=bm_dx1) {
        bm_dx0=x0; bm_dy0=y0; bm_dx1=x1; bm_dy1=y1;
        return;
    }
    if(x0<bm_dx0) bm_dx0=x0;
    if(y0<bm_dy0) bm_dy0=y0;
    if(x1>bm_dx1) bm_dx1=x1;
    if(y1>bm_dy1) bm_dy1=y1;
}

void BM_remapfld(void) {
    const unsigned char mask_clear_wall = (unsigned char)(255 - BM_WALL);
    unsigned bx0, by0, bx1, by1, bx, by, dy, dx;

    if(bm_full || bm_dx0>=bm_dx1) {
        bx0=0; by0=0; bx1=FLDW_QUARTER; by1=FLDH_QUARTER;
    } else {
        bx0=(unsigned)bm_dx0>>2;
        by0=(unsigned)bm_dy0>>2;
        bx1=((unsigned)bm_dx1+3u)>>2;
        by1=((unsigned)bm_dy1+3u)>>2;
        if(bx1>FLDW_QUARTER) bx1=FLDW_QUARTER;
        if(by1>FLDH_QUARTER) by1=FLDH_QUARTER;
    }
    for(by=by0;by<by1;++by) {
        for(bx=bx0;bx<bx1;++bx) {
            int has_wall=0;
            for(dy=0;dy<4u && !has_wall;++dy)
                for(dx=0;dx<4u && !has_wall;++dx) {
                    unsigned char v=fld[by*4+dy][bx*4+dx];
                    if(v==1 || v==2) has_wall=1;
                }
            if(has_wall)
                bmap[by][bx]|=BM_WALL;
            else
                bmap[by][bx]&=mask_clear_wall;
        }
    }
    fld_need_remap=0;
    bm_full=0;
    bm_dx0=FLDW; bm_dy0=FLDH; bm_dx1=0; bm_dy1=0;
}

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
