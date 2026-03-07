#include "view.h"
#include "bmap.h"

#include <cstdint>

#ifndef BN_CODE_IWRAM
#define BN_CODE_IWRAM __attribute__((section(".iwram")))
#endif

static constexpr unsigned BMAP_BYTES = (FLDH / 4) * (FLDW / 4);

void BN_CODE_IWRAM BM_clear(unsigned char f) {
  unsigned char m = (unsigned char)(~f);
  std::uint32_t mask32 = (std::uint32_t)m | ((std::uint32_t)m << 8)
      | ((std::uint32_t)m << 16) | ((std::uint32_t)m << 24);

  std::uint32_t* p = (std::uint32_t*)bmap;
  unsigned n = BMAP_BYTES / 4;
  while (n--) {
    *p++ &= mask32;
  }
  unsigned tail = BMAP_BYTES % 4;
  unsigned char* q = (unsigned char*)p;
  while (tail--) {
    *q++ &= m;
  }
}

void BN_CODE_IWRAM BM_remapfld(void) {
  const unsigned char mask_clear_wall = (unsigned char)(255 - BM_WALL);
  for (unsigned by = 0; by < FLDH / 4u; ++by) {
    for (unsigned bx = 0; bx < FLDW / 4u; ++bx) {
      bool has_wall = false;
      for (unsigned dy = 0; dy < 4u && !has_wall; ++dy) {
        for (unsigned dx = 0; dx < 4u && !has_wall; ++dx) {
          unsigned char v = fld[by * 4 + dy][bx * 4 + dx];
          if (v == 1 || v == 2) has_wall = true;
        }
      }
      if (has_wall)
        bmap[by][bx] |= BM_WALL;
      else
        bmap[by][bx] &= mask_clear_wall;
    }
  }
  fld_need_remap = 0;
}
