#include "glob.h"
// #include <stdio.h>
// #include <stdlib.h>
// #include <malloc.h>
// #include <string.h>
// #include "error.h"
#include "files.h"
#include "memory.h"

#include <cstring>

#include "bn_log.h"
#include "bn_memory.h"

extern int d_start,d_end;

static unsigned char m_active=FALSE;

BN_DATA_EWRAM static void *resp[MAX_WAD];
BN_DATA_EWRAM static short resl[MAX_WAD];

void M_startup(void) {
   if(m_active) return;
   BN_LOG("M_startup: setting up memory");
   memset(resp,0,sizeof(resp));
   memset(resl,0,sizeof(resl));
   BN_LOG("  available EWRAM: ", bn::memory::available_alloc_ewram());
   m_active=TRUE;
}

// static void allocres(int h) {
//   int *p,s;
//
//   if(h>d_start && h<d_end) s=1; else s=0;
//   if(!(p=static_cast<int *>(malloc(wad[h].l+4+s*8))))
//     BN_ERROR("M_lock: not enough memory");
//   *p=h;
//   ++p;
//   resp[h]=p;
//   if(s) {
//     p[0]=p[1]=p[2]=p[3]=0;
//     F_loadres(h,p,0,2);
//     F_loadres(h,p+1,2,2);
//     F_loadres(h,p+2,4,2);
//     F_loadres(h,p+3,6,2);
//     F_loadres(h,p+4,8,wad[h].l-8);
//   }else F_loadres(h,p,0,wad[h].l);
// }

void *M_lock(int h) {
    // Just return the pointer to resource in ROM:
    if(h==-1 || h==0xFFFF) return NULL;
    h&=-1-0x8000;
    if(h>=MAX_WAD) BN_ERROR("M_lock: strange resource number: ", h);
    if(!resl[h]) {
        const int f = F_wad_get_f(h);
        const uint8_t* data = wad_data[f] + F_wad_get_o(h);
        resp[h] = (void*)data;
    }
    ++resl[h];
    return resp[h];
}

enum { EWRAM_HEADER_SIZE = sizeof(int) };

void *M_lock_ewram(int h, void* ewram_ptr) {
    if (h==-1 || h==0xFFFF) return NULL;
    h&=-1-0x8000;
    if (h>=MAX_WAD) BN_ERROR("M_lock_ewram: strange resource number: ", h);
    if (!resl[h]) {
        const int f = F_wad_get_f(h);
        const unsigned lump_len = F_wad_get_l(h);
        const uint8_t* data = wad_data[f] + F_wad_get_o(h);

        // TODO: CHECK CAREFULLY!
        // BN_ASSERT(ewram_ptr == nullptr, "M_lock_ewram: ewram_ptr should be null when locking a resource for the first time");
        M_unlock_ewram(nullptr, ewram_ptr); // Just in case, free the EWRAM pointer if it was not null (should not happen, but better safe than sorry).
        // TODO: clear resources on different cases!!!

        BN_LOG("M_lock_ewram: EWRAM available before allocation: ", bn::memory::available_alloc_ewram(), " for resource #", h, " size=", lump_len);
        void* base = bn::memory::ewram_alloc(EWRAM_HEADER_SIZE + lump_len);
        if (base) {
            *(int*)base = h;
            void* data_ptr = (char*)base + EWRAM_HEADER_SIZE;
            memcpy(data_ptr, data, lump_len);
            BN_LOG("M_lock_ewram: allocated EWRAM for resource #", h, " size=", lump_len, " available EWRAM after allocation: ", bn::memory::available_alloc_ewram());
            resp[h] = data_ptr;
        } else {
            BN_ERROR("M_lock_ewram: not enough EWRAM for resource #", h, " size=", lump_len);
        }
    }
    ++resl[h];
    return resp[h];
}

void M_unlock(void *p) {
    // No need to free anything for ROM-backed resources (M_lock returns pointer into WAD).
    // Original stored handle at p[-1] (allocres put *base=h); we don't have that, so find h by resp[i]==p.
    if(!p) return;
    int h = -1;
    for (int i = 0; i < MAX_WAD; ++i)
        if (resp[i] == p) { h = i; break; }
    if (h < 0 || h >= MAX_WAD) BN_ERROR("M_unlock: unknown pointer (not from M_lock), or strange resource number: ", h);
    if(!resl[h]) return;
    --resl[h];
}

void M_unlock_ewram(void *p, void* ewram_ptr) {
    // Butano: ewram_free(ptr) must receive exactly the pointer returned by ewram_alloc — no other value.
    if (p) {
        void* base = (char*)p - EWRAM_HEADER_SIZE;  // base == value returned by ewram_alloc
        int h = *(int*)base;
        h &= -1 - 0x8000;
        if (h >= MAX_WAD) BN_ERROR("M_unlock_ewram: strange resource number: ", h);
        BN_LOG("M_unlock_ewram: unlocking resource #", h, " available EWRAM before free: ", bn::memory::available_alloc_ewram());
        if (resl[h]) {
            --resl[h];
        }
        bn::memory::ewram_free(base);
    }
    (void)ewram_ptr;
    BN_LOG("M_unlock_ewram: available EWRAM after free: ", bn::memory::available_alloc_ewram());
}
