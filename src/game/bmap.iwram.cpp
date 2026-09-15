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
