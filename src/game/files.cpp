#include "glob.h"
// #include <stdio.h>
// #include <conio.h>
// #include <malloc.h>
// #include <dos.h>
#include <cstring>
#include <cstdint>
// #include <sys\stat.h>
// #include "vga.h"
// #include "error.h"
// #include "sound.h"
// #include "snddrv.h"
#include "memory.h"
#include "view.h"
#include "items.h"
#include "switch.h"
#include "files.h"

#include "bn_assert.h"
#include "bn_color.h"
#include "bn_log.h"
#include "bn_memory.h"
#include "enc.h"
#include "os.h"
#include "map.h"
#include "sound.h"

char *S_getinfo(void);

extern void *snd_drv;

typedef struct{
  unsigned char n,i,v,d;
}dmv;

unsigned char seq[255],seqn;
dmv *pat=NULL;
unsigned *patp;
void **dmi;

static int inum=0;

void G_savegame(int);
void W_savegame(int);
void DOT_savegame(int);
void SMK_savegame(int);
void FX_savegame(int);
void IT_savegame(int);
void MN_savegame(int);
void PL_savegame(int);
void SW_savegame(int);
void WP_savegame(int);

void G_loadgame(int);
void W_loadgame(int);
void DOT_loadgame(int);
void SMK_loadgame(int);
void FX_loadgame(int);
void IT_loadgame(int);
void MN_loadgame(int);
void PL_loadgame(int);
void SW_loadgame(int);
void WP_loadgame(int);

unsigned char savname[7][24],savok[7];

int d_start,d_end,m_start,m_end,s_start,s_end,wad_num;

/* Компактный индекс: для каждого лумпа r храним только номер WAD и индекс в его директории; данные читаем из ROM. */
BN_DATA_EWRAM static uint8_t wad_file[MAX_WAD];
BN_DATA_EWRAM static uint16_t wad_entry[MAX_WAD];
BN_DATA_EWRAM static uint32_t wad_dir_ofs[MAX_WADS];
BN_DATA_EWRAM static uint32_t wad_numlumps[MAX_WADS];

BN_DATA_EWRAM char wads[MAX_WADS][_MAX_PATH];
const uint8_t* wad_data[MAX_WADS] = {0};
unsigned wad_size[MAX_WADS] = {0};

static int wadh[MAX_WADS];

// char f_drive[_MAX_DRIVE],f_dir[_MAX_DIR],f_name[_MAX_FNAME],f_ext[_MAX_EXT],
//   f_path[_MAX_PATH];
//
void F_startup(void) {
  BN_LOG("F_startup: initializing file system");
  memset(wads,0,sizeof(wads));
}
//
// void F_getsavnames(void) {
//   int i,h;
//   static char n[]="SAVGAME0.DAT";
//   short ver;
//
//   for(i=0;i<7;++i) {
//     n[7]=i+'0';memset(savname[i],0,24);savok[i]=0;
//     if((h=open(n,O_RDONLY|O_BINARY))==-1) continue;
//     read(h,savname[i],24);ver=-1;read(h,&ver,2);
//     close(h);savname[i][23]=0;savok[i]=(ver==2)?1:0;
//   }
// }
//
// void F_savegame(int n,char *s) {
//   int h;
//   static char fn[]="SAVGAME0.DAT";
//
//   fn[7]=n+'0';
//   if((h=open(fn,O_BINARY|O_CREAT|O_RDWR|O_TRUNC,S_IREAD|S_IWRITE))==-1) return;
//   write(h,s,24);write(h,"\2\0",2);
//   G_savegame(h);
//   W_savegame(h);
//   DOT_savegame(h);
//   SMK_savegame(h);
//   FX_savegame(h);
//   IT_savegame(h);
//   MN_savegame(h);
//   PL_savegame(h);
//   SW_savegame(h);
//   WP_savegame(h);
//   close(h);
// }
//
// void F_loadgame(int n) {
//   int h;
//   static char fn[]="SAVGAME0.DAT";
//   short ver;
//
//   fn[7]=n+'0';
//   if((h=open(fn,O_BINARY|O_RDONLY))==-1) return;
//   lseek(h,24,SEEK_SET);read(h,&ver,2);if(ver!=2) return;
//   G_loadgame(h);
//   W_loadgame(h);
//   DOT_loadgame(h);
//   SMK_loadgame(h);
//   FX_loadgame(h);
//   IT_loadgame(h);
//   MN_loadgame(h);
//   PL_loadgame(h);
//   SW_loadgame(h);
//   WP_loadgame(h);
//   close(h);
// }
//
// void F_set_snddrv(void) {
//   snd_card=(snd_card>=SDRV__END)?0:snd_card;
//   BN_LOG("F_set_snddrv: звуковая карта #%d\n",snd_card);
//   snd_drv=snd_drv_tab[snd_card];
//   BN_LOG("  %s ",S_getinfo());
//   if(snd_card) BN_LOG("(%dГц)\n",(dword)sfreq);
//     else BN_LOG("\n");
// }
//
static inline uint32_t read_le32(const uint8_t* p) {
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

/** Указатель на запись директории лумпа r в ROM (16 байт: filepos, size, name[8]). */
static inline const uint8_t* F_wad_entry_ptr(int r) {
  int f = wad_file[r];
  return wad_data[f] + wad_dir_ofs[f] + (unsigned)wad_entry[r] * 16u;
}

void F_addwad(const char *fn, const unsigned char *data, const unsigned size) {
  BN_LOG("F_addwad: adding WAD [", fn, "] size=", size);
  for (int i = 0; i < MAX_WADS; ++i)
    if (wads[i][0] == 0) {
      strcpy(wads[i], fn);
      wad_data[i] = data;
      wad_size[i] = size;
      return;
    }
  BN_ERROR("Unable to add WAD: [", fn, "]");
}

void F_initwads(void) {
  int p = 0;
  BN_LOG("F_initwads: attaching WAD files");

  for (int i = 0; i < MAX_WADS; ++i) {
    if (wads[i][0] == 0) continue;
    const uint8_t* data = wad_data[i];
    unsigned size = wad_size[i];
    if (!data || size < 12) {
      BN_ERROR("WAD without data: ", wads[i]);
    }
    BN_LOG("  attaching ", wads[i]);
    if (size < 4) BN_ERROR("Missing IWAD or PWAD signature");
    if ((data[0] != 'I' && data[0] != 'P') || data[1] != 'W' || data[2] != 'A' || data[3] != 'D')
      BN_ERROR("Missing IWAD or PWAD signature");
    uint32_t numlumps = read_le32(data + 4);
    uint32_t dir_ofs = read_le32(data + 8);
    wad_dir_ofs[i] = dir_ofs;
    wad_numlumps[i] = numlumps;
    if (dir_ofs + numlumps * 16u > size)
      BN_ERROR("WAD directory out of range");
    for (uint32_t j = 0; j < numlumps; ++j) {
      if (p >= MAX_WAD) BN_ERROR("Too many WAD entries");
      wad_file[p] = (uint8_t)i;
      wad_entry[p] = (uint16_t)j;
      ++p;
    }
  }
  wad_num = p;
  BN_LOG("Lumps loaded: ", wad_num);
}

int F_wad_get_f(int r) {
  return wad_file[r];
}
unsigned F_wad_get_o(int r) {
  return read_le32(F_wad_entry_ptr(r) + 0);
}
unsigned F_wad_get_l(int r) {
  return read_le32(F_wad_entry_ptr(r) + 4);
}
void F_wad_get_name(int r, char n[8]) {
  memcpy(n, F_wad_entry_ptr(r) + 8, 8);
}


// allocate resources
// (called from M_startup)
void F_allocres(void) {
//  int i;

  d_start=F_getresid("D_START");
  d_end=F_getresid("D_END");
  m_start=F_getresid("M_START");
  m_end=F_getresid("M_END");
  s_start=F_getresid("S_START");
  s_end=F_getresid("S_END");
}

// load resource
void F_loadres(int r, void *p, unsigned int o, unsigned int l) {
  int f = F_wad_get_f(r);
  const uint8_t* src = wad_data[f];
  unsigned size = wad_size[f];
  unsigned offset = F_wad_get_o(r) + o;
  if (offset + l > size)
    BN_ERROR("F_loadres: read past end of WAD lump");
  memcpy(p, src + offset, l);
}

void F_loadpal(int r, void *p, unsigned int o, unsigned int num) {
  // uint32_t playpal_size = 0;
  // const uint8_t* playpal = wad.find_lump("PLAYPAL", &playpal_size);
  // if (playpal && playpal_size >= PALETTE_BYTES) {
  //     const uint8_t* pal0 = playpal;  // palette 0 = first 768 bytes (256 RGB)
  //     for (int i = 0; i < 256; ++i) {
  //         uint8_t r = pal0[i * 3 + 0], g = pal0[i * 3 + 1], b = pal0[i * 3 + 2];
  //         doom_palette_0[i] = bn::color(r >> 1, g >> 1, b >> 1); // convert 8-bit to 5-bit per channel
  //     }
  // }
  int f = F_wad_get_f(r);
  const uint8_t* src = wad_data[f];
  unsigned size = wad_size[f];
  unsigned offset = F_wad_get_o(r) + o;
  if (offset + num * 3 > size)
    BN_ERROR("F_loadpal: read past end of WAD lump");
  for (unsigned i = 0; i < num; ++i) {
    uint8_t r = src[offset + i * 3 + 0];
    uint8_t g = src[offset + i * 3 + 1];
    uint8_t b = src[offset + i * 3 + 2];
    ((bn::color*)p)[i] = bn::color(r >> 1, g >> 1, b >> 1); // convert 8-bit to 5-bit per channel
  }
}

// void F_saveres(int r,void *p,dword o,dword l) {
//   int fh,oo;
//
//   oo=tell(fh=wadh[wad[r].f]);
//   if(lseek(fh,wad[r].o+o,SEEK_SET)==-1L)
//     BN_ERROR(("Ошибка при чтении файла");
//   write(fh,p,l);
//   lseek(fh,oo,SEEK_SET);
// }
//
// get resource id
int F_getresid(char *n) {
  BN_LOG("F_getresid: looking for resource [", cp866_to_utf8(n), "]");
  int i;

  for(i=0;i<wad_num;++i) {
    char buf[8];
    F_wad_get_name(i, buf);
    if(strnicmp(buf,n,8)==0) return i;
  }
  BN_ERROR("F_getresid: resource [", n, "] not found");
  return -1;
}

// get resource id
int F_findres(char *n) {
  int i;
  char buf[8];
  for(i=0;i<wad_num;++i) {
    F_wad_get_name(i, buf);
    if(strnicmp(buf,n,8)==0) return i;
  }
  return -1;
}

void F_getresname(char *n,int r) {
  BN_LOG("F_getresname: looking for name of resource #", r);
  F_wad_get_name(r, n);
}

// get sprite id
int F_getsprid(char n[4],int s,int d) {
  int i;
  unsigned char a,b;
  char buf[8];

  s+='A';d+='0';
  for(i=s_start+1;i<s_end;++i) {
    F_wad_get_name(i, buf);
    if(memicmp(buf,n,4)==0 && (buf[4]==s || buf[6]==s)) {
      if(buf[4]==s) a=buf[5]; else a=0;
      if(buf[6]==s) b=buf[7]; else b=0;
      if(a=='0') return i;
      if(b=='0') return(i|0x8000);
      if(a==d) return i;
      if(b==d) return(i|0x8000);
    }
  }
  BN_ERROR("F_getsprid: sprite not found: ", n, " s=", s, " d=", d);
  return -1;
}

int F_getreslen(int r) {
  return (int)F_wad_get_l(r);
}

// void F_nextmus(char *s) {
//   int i;
//
//   i=F_findres(s);
//   if(i<=m_start || i>=m_end) i=m_start;
//   for(++i;;++i) {
//     if(i>=m_end) i=m_start+1;
//     if(memicmp(wad[i].n,"DMI",3)!=0) break;
//   }
//   memcpy(s,wad[i].n,8);
// }
//
// reads unsigned chars from file until CR
void F_readstr(int h,char *s,int m) {
  int i;
  static char c;

  for(i=0;;) {
    c=13;
    read(h,&c,1);
    if(c==13) break;
    if(i<m) s[i++]=c;
  }
  s[i]=0;
}

// reads unsigned chars from file until NUL
void F_readstrz(int h,char *s,int m) {
  int i;
  static char c;

  for(i=0;;) {
    c=0;
    read(h,&c,1);
    if(c==0) break;
    if(i<m) s[i++]=c;
  }
  s[i]=0;
}

map_block_t blk;

void F_loadmap(char n[8]) {
  int r;
  map_header_t hdr;
  const uint8_t* lump;
  unsigned lump_left;

  W_init();
  r = F_getresid(n);
  lump = wad_data[F_wad_get_f(r)] + F_wad_get_o(r);
  lump_left = F_wad_get_l(r);

  if (lump_left < sizeof(hdr))
    BN_ERROR("F_loadmap: lump too small for header");
  memcpy(&hdr, lump, sizeof(hdr));
  lump += sizeof(hdr);
  lump_left -= sizeof(hdr);

  if (memcmp(hdr.id, "Doom2D\x1A", 8) != 0)
    BN_ERROR("F_loadmap: not a level: ", n);

  for (;;) {
    if (lump_left < sizeof(blk))
      BN_ERROR("F_loadmap: lump truncated at block");
    memcpy(&blk, lump, sizeof(blk));
    lump += sizeof(blk);
    lump_left -= sizeof(blk);

    if (blk.t == MB_END) break;
    if (blk.t == MB_COMMENT) {
      if (lump_left < (unsigned)blk.sz)
        BN_ERROR("F_loadmap: comment block overflows lump");
      lump += blk.sz;
      lump_left -= blk.sz;
      continue;
    }

    // Block size: save before G_load/W_load (they may modify blk.sz, e.g. W_load MB_WALLNAMES).
    const unsigned block_sz = (unsigned)blk.sz;
    if (lump_left < block_sz) {
      BN_ERROR("F_loadmap: block overflows lump");
    }
    if (G_load(lump, block_sz))
      { lump += block_sz; lump_left -= block_sz; continue; }
    if (W_load(lump, block_sz))
      { lump += block_sz; lump_left -= block_sz; continue; }
    if (IT_load(lump, block_sz))
      { lump += block_sz; lump_left -= block_sz; continue; }
    if (SW_load(lump, block_sz))
      { lump += block_sz; lump_left -= block_sz; continue; }

    BN_ERROR("F_loadmap: unknown block ", blk.t, " ", blk.st, " in ", n);
  }

  BN_LOG("F_loadmap: level ", n, " loaded successfully");
  BN_LOG("EWRAM free: ", bn::memory::available_alloc_ewram());
}

// TODO: check
void F_freemus(void) {
   // int i;

   // if(!pat) return;
   S_stopmusic();
   // free(pat);free(patp);
   // for(i=0;i<inum;++i) if(dmi[i]!=NULL) free(dmi[i]);
   // free(dmi);
   // pat=NULL;
  current_music_ref.reset();
}

void F_loadmus(char n[8]) {
  char buf[9];
  memcpy(buf, n, 8);
  buf[8] = '\0';
  BN_LOG("F_loadmus: loading music [", cp866_to_utf8(buf), "]");
  current_music_ref = get_xm_ref(buf);
}


