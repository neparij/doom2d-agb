#ifndef DOOM2D_RENDERER_H
#define DOOM2D_RENDERER_H
#include "view.h"
#include "bn_bg_palette_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_palette_bitmap_bg_painter.h"
#include "bn_palette_bitmap_bg_ptr.h"
#include "bn_sprite_text_generator.h"

#define WD 232   // GBA viewport width; must match view.cpp and Z_drawfld for tile/sprite alignment
#define HT w_ht

/** Culling: 1 если объект (x,y,r,h) пересекает viewport. margin — запас для спрайтов. */
static inline int R_obj_in_view(int x, int y, int r, int h) {
  const int margin = 16;
  if (x + r < w_x - WD/2 - margin) return 0;
  if (x - r > w_x + WD/2 + margin) return 0;
  if (y < w_y - HT/2 - margin) return 0;
  if (y - h > w_y + HT/2 + margin) return 0;
  return 1;
}

extern uint8_t water_remap[3][256];
extern bn::optional<bn::sprite_text_generator> text_stbf;
extern bn::optional<bn::sprite_text_generator> text_stcfn;
extern bn::optional<bn::sprite_text_generator> text_stt;
extern bn::optional<bn::sprite_text_generator> text_wi;
extern bn::vector<bn::sprite_ptr, 13> st_gfx_sprites;
extern bn::vector<bn::sprite_ptr, 64> st_text_sprites;
extern bn::vector<bn::sprite_ptr, 64> menu_text_sprites;
extern bn::vector<bn::sprite_ptr, 2> menu_cursor_sprites;
extern bn::vector<bn::sprite_ptr, 64> inter_text_sprites;

void R_Init(void);

void R_clear(void);

/** Clear all UI sprite vectors (menu, status bar). Call before game start. */
void R_clear_sprites(void);

/** Возвращает 1, если имя стены (8 байт CP866 из карты) входит в список прозрачных стен. */
int is_wallname_transparent(const char* map_name_8_cp866);

/** Устанавливает сдвиг canvas по X (0..3). Вызывать до R_blit при выровненной по 4 пикселя камере. */
void R_set_canvas_offset_x(int x);

void R_blit(void);

void V_setrect(int x, int w, int y, int h);
void V_clr(int x, int w, int y, int h, unsigned char color);
void V_pic(int, int, void *);
void V_pic_2buf(int, int, void *);
/** Рисует текстуру по тайлам только там, где cell_mask[cell_y][cell_x]!=0. pic — текстура с 8-байт заголовком, cell_mask — FLDH*FLDW. */
void V_pic_tiled_masked(void* pic, int pic_origin_x, int pic_origin_y, const unsigned char* cell_mask);
void V_spr(int, int, void *);
void V_spr2(int, int, void *);

void V_manspr(int, int, void *);

void V_manspr2(int, int, void *);

// вывести точку цвета c в координатах (x,y)
void V_dot(short x,short y,unsigned char c);

void V_apply_palette_effects(int);

void V_apply_water_filter_to(uint32_t* buffer_base, int row_stride_words);

void V_apply_cropping_stripes(uint32_t* buffer_base, int color_index);

void framebuf_blit(int dst_x, int dst_y,
                   const uint8_t *src_ptr,
                   int src_width, int src_height,
                   int src_x, int src_y,
                   int w, int h,
                   const bool transparent,
                   const bool flip_horizontal);

void framebuf_flip_and_copy();

extern bn::optional<bn::bg_palette_ptr> bg_palette;
extern bn::optional<bn::palette_bitmap_bg_ptr> canvas;
extern bn::optional<bn::palette_bitmap_bg_painter> canvas_painter;
extern int canvas_offset_x;


#endif //DOOM2D_RENDERER_H