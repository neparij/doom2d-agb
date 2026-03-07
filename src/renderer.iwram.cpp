#include "renderer.h"
#include "view.h"
#include "bn_memory.h"

#include <cstdint>
#include <cstring>

#include "bn_core.h"
#include "bn_display.h"
#include "bn_log.h"
#include "debug.h"
#include "files.h"
#include "../butano/butano/hw/include/bn_hw_display.h"
#include "../butano/butano/hw/include/bn_hw_dma.h"
#include "../butano/butano/src/bn_display_manager.h"

bn::optional<bn::bg_palette_ptr> bg_palette(bn::nullopt);
bn::optional<bn::palette_bitmap_bg_ptr> canvas(bn::nullopt);
bn::optional<bn::palette_bitmap_bg_painter> canvas_painter(bn::nullopt);

constexpr int TILE_SIZE = 8;
constexpr int TILES_H = 31-0;
constexpr int TILES_V = 21-5;
// constexpr int VIEW_PIXEL_W = bn::bitmap_bg::palette_width();
// constexpr int VIEW_PIXEL_H = bn::bitmap_bg::palette_height();
constexpr int VIEW_PIXEL_W = (TILES_H - 1) * TILE_SIZE;
constexpr int VIEW_PIXEL_H = (TILES_V - 1) * TILE_SIZE;
constexpr int VIEW_PIXEL_FULLSCREEN_W = bn::display::width();
constexpr int VIEW_PIXEL_FULLSCREEN_H = bn::display::height();

// Буфер для горизонтальной полосы тайлов 8x8 (как в main_v6): один blit на серию одинаковых тайлов.
// BN_DATA_EWRAM static uint8_t tile_strip_buf[VIEW_PIXEL_W * TILE_SIZE];
// static uint8_t tile_strip_buf[(VIEW_PIXEL_W - 8) * TILE_SIZE];
// BN_DATA_EWRAM static bool drawfld_mark[TILES_H+MAXTXW*2];

alignas(4) static uint32_t pixel_strip_buf[VIEW_PIXEL_W / 4];
alignas(4) static uint8_t tex_line_buf[VIEW_PIXEL_W];
alignas(4) uint8_t water_remap[3][256];

// Сдвиг canvas по X (0..3): камера выравнивается по 4 пикселя для DMA, сдвиг компенсирует отображение.
int canvas_offset_x = 0;

inline uint32_t read_le32(const uint8_t* p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
inline uint16_t read_le16(const uint8_t* p) {
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}


static void framebuf_clear() {
    // framebuffer in VRAM:
    uint16_t* buffer = bn::display_manager::bitmap_page();
    constexpr int framebuffer_words = VIEW_PIXEL_W * VIEW_PIXEL_H / 4;
    memset32(buffer, 0, framebuffer_words);
}

void BN_CODE_IWRAM draw_impl(const uint8_t* src_row, uint8_t* dst_row, const int page_x, int item_width) {
    auto dst = reinterpret_cast<uint16_t*>(dst_row + (page_x & ~1));
    int item_x = 0;
    int blit_width = item_width;

    if (page_x & 1) {
        const uint8_t* src = src_row;
        if (uint8_t src_x = *src)
            *dst = (*dst & 0xFF) | (src_x << 8);
        ++item_x;
        --blit_width;
        ++dst;
    }

    if (blit_width) {
        const uint8_t* src = src_row + item_x;
        uint16_t* dst_x = dst;
        for (int ix = 0; ix < blit_width; ix += 2) {
            const uint8_t src_a = *src;
            const uint8_t dst_a = src_a ? src_a : *dst_x & 0xFF;
            ++src;

            if (blit_width - ix == 1) {
                *dst_x = (src_a ? src_a : *dst_x & 0xFF) | (*dst_x & 0xFF00);
                break;
            }

            const uint8_t src_b = *src;
            ++src;

            if (src_a && src_b)
                *dst_x = (uint16_t)src_a | ((uint16_t)src_b << 8);
            else {
                const uint8_t dst_b = src_b ? src_b : *dst_x >> 8;
                *dst_x = dst_a | ((uint16_t)dst_b << 8);
            }
            ++dst_x;
        }
    }
}

void BN_CODE_IWRAM framebuf_blit(int dst_x, int dst_y,
                                        const uint8_t *src_ptr,
                                        int src_width, int src_height,
                                        int src_x, int src_y,
                                        int w, int h,
                                        const bool transparent,
                                        const bool flip_horizontal) {

    const int view_w = g_st == GS_GAME ? VIEW_PIXEL_W : VIEW_PIXEL_FULLSCREEN_W;
    const int view_h = g_st == GS_GAME ? VIEW_PIXEL_H : VIEW_PIXEL_FULLSCREEN_H;

    if (dst_x >= view_w || dst_y >= view_h || w <= 0 || h <= 0) return;
    if (dst_x + w <= 0 || dst_y + h <= 0) return;
    int clip_l = dst_x < 0 ? -dst_x : 0;
    int clip_t = dst_y < 0 ? -dst_y : 0;
    int clip_r = (dst_x + w > view_w) ? (dst_x + w - view_w) : 0;
    int clip_b = (dst_y + h > view_h) ? (dst_y + h - view_h) : 0;

    if (flip_horizontal) {
        src_x += clip_r;
    } else {
        src_x += clip_l;
    }

    src_y += clip_t;
    dst_x += clip_l; dst_y += clip_t;
    w -= clip_l + clip_r; h -= clip_t + clip_b;
    if (w <= 0 || h <= 0) return;

    for (int row = 0; row < h; ++row) {
        const uint8_t* src_row = src_ptr + (src_y + row) * src_width + src_x;

        // VRAM buffer
        uint8_t* dst_row = (uint8_t*)bn::display_manager::bitmap_page() + (dst_y + row) * bn::display::width();
        if (transparent) {

            // VRAM VARIANT (u16): TODO: оптимизировать, чтобы не читать-писать по одному пикселю, а блоками. Сложность в том, что нужно сохранять прозрачные пиксели в VRAM (не затирать их нулями).
             if (!flip_horizontal) {
                 draw_impl(src_row, dst_row, dst_x, w);
            } else {
                // Fill (uint32_t) pixel_strip_buf with flipped pixels from src_row, then draw to VRAM:
                for (int col = 0; col < w; col += 4) {
                    uint32_t combined = 0;
                    for (int i = 0; i < 4; ++i) {
                        const uint8_t v = src_row[w - 1 - (col + i)];
                        combined |= (v << (i * 8));
                    }
                    pixel_strip_buf[col / 4] = combined;
                }
                draw_impl((uint8_t*)pixel_strip_buf, dst_row, dst_x, w);
             }
        } else {
            // Opaque: use DMA when aligned for speed. Prefer 4-aligned copy_words, then 2-aligned copy_half_words.
            const uintptr_t src_a = (uintptr_t)src_row;
            const uintptr_t dst_a = (uintptr_t)(dst_row + dst_x);
            if ((src_a % 4 == 0) && (dst_a % 4 == 0) && (w % 4 == 0)) {
                bn::hw::dma::copy_words(src_row, w / 4, (uint32_t*)(dst_row + dst_x));
            } else if ((src_a % 2 == 0) && (dst_a % 2 == 0) && (w % 2 == 0)) {
                bn::hw::dma::copy_half_words(src_row, w / 2, dst_row + dst_x);
            } else {
                BN_LOG("⚠️ UNALIGNED DRAW FOR OPAQUE BLIT!");
                for (int col = 0; col < w; ++col) {
                    dst_row[col + dst_x] = src_row[col];
                }
            }
        }
    }
}

// Flips current framebuf page and copies to the other page for display.
void framebuf_flip_and_copy() {
    const uint32_t* current = reinterpret_cast<const uint32_t*>(bn::display_manager::bitmap_page());
    bn::display_manager::flip_bitmap_page_now();

    // Copy drawn frame to new draw buffer (4bpp: 240*160/2 bytes = 4800 words)
    uint32_t* new_page = reinterpret_cast<uint32_t*>(bn::display_manager::bitmap_page());
    constexpr int framebuffer_words = (VIEW_PIXEL_FULLSCREEN_W * VIEW_PIXEL_FULLSCREEN_H) / 4;
    bn::hw::dma::copy_words(current, framebuffer_words, new_page);
}

void R_clear(void) {
    framebuf_clear();
}

void R_set_canvas_offset_x(int x) {
    canvas_offset_x = x;
}

void R_blit() {
    // Позицию canvas задаём после отрисовки и blit — чтобы применить к уже готовому кадру (меньше дёрганий при смене кратности).
    if (g_st==GS_GAME) {
#ifdef D2D_DEBUG_ENABLE

        if (dbg_draw_water) V_apply_water_filter_to(reinterpret_cast<uint32_t*>(bn::display_manager::bitmap_page()), VIEW_PIXEL_W / 4);
        V_apply_cropping_stripes(reinterpret_cast<uint32_t*>(bn::display_manager::bitmap_page()), 0);
#else
        V_apply_water_filter_to(reinterpret_cast<uint32_t*>(bn::display_manager::bitmap_page()), VIEW_PIXEL_W / 4);
        V_apply_cropping_stripes(reinterpret_cast<uint32_t*>(bn::display_manager::bitmap_page()), 0);
#endif

        canvas->set_position(4 - canvas_offset_x, 12);
    } else {
        canvas->set_position(0, 0);
    }
    canvas_painter->flip_page_later();
}

// V_spr: same header (w,h,ox,oy at +0,+2,+4,+6), draw at (x - ox, y - oy), no flip
void V_spr(int x, int y, void *p) {
    const int src_width = (int)read_le16((const uint8_t*)p + 0);
    const int src_height = (int)read_le16((const uint8_t*)p + 2);
    const int ox = (int)(int16_t)read_le16((const uint8_t*)p + 4);
    const int oy = (int)(int16_t)read_le16((const uint8_t*)p + 6);
    framebuf_blit(x - ox, y - oy,
                (const uint8_t*)p + 8, src_width, src_height,
                0, 0, src_width, src_height,
                true, false);
}

// V_spr2: flipped horizontally, top-left = (x - width + ox, y - oy)
void V_spr2(int x, int y, void *p) {
    const int src_width = (int)read_le16((const uint8_t*)p + 0);
    const int src_height = (int)read_le16((const uint8_t*)p + 2);
    const int ox = (int)(int16_t)read_le16((const uint8_t*)p + 4);
    const int oy = (int)(int16_t)read_le16((const uint8_t*)p + 6);
    framebuf_blit(x - src_width + ox, y - oy,
                (const uint8_t*)p + 8, src_width, src_height,
                0, 0, src_width, src_height,
                true, true);
}

void V_manspr(int x, int y, void *p) {
    const int src_width = (int)read_le16((const uint8_t*)p + 0);
    const int src_height = (int)read_le16((const uint8_t*)p + 2);
    const int ox = (int)(int16_t)read_le16((const uint8_t*)p + 4);
    const int oy = (int)(int16_t)read_le16((const uint8_t*)p + 6);
    framebuf_blit(x - ox, y - oy,
                (const uint8_t*)p + 8, src_width, src_height,
                0, 0, src_width, src_height,
                true, false);
}

// Flipped horizontally: asm does x = x - [width] + [ox], so top-left = (x - width + ox, y - oy)
void V_manspr2(int x, int y, void *p) {
    const int src_width = (int)read_le16((const uint8_t*)p + 0);
    const int src_height = (int)read_le16((const uint8_t*)p + 2);
    const int ox = (int)(int16_t)read_le16((const uint8_t*)p + 4);
    const int oy = (int)(int16_t)read_le16((const uint8_t*)p + 6);
    framebuf_blit(x - src_width + ox, y - oy,
                (const uint8_t*)p + 8, src_width, src_height,
                0, 0, src_width, src_height,
                true, true);
}

// вывести точку цвета c в координатах (x,y). VRAM — только 16-битная запись по выровненному адресу.
void V_dot(short x,short y,unsigned char c) {
    if (x < 0 || x >= VIEW_PIXEL_W || y < 0 || y >= VIEW_PIXEL_H) return;
    uint8_t* row = (uint8_t*)bn::display_manager::bitmap_page() + y * bn::display::width();
    uint16_t* dst = (uint16_t*)(row + (x & ~1));  // выровненное слово, в котором лежит пиксель (x,y)
    if (x & 1)
        *dst = (*dst & 0xFF) | (c << 8);   // нечётный x — старший байт слова
    else
        *dst = (*dst & 0xFF00) | c;        // чётный x — младший байт
}

// Original V_setrect: set screen rectangle (scrx, scry, scrw, scrh) for clipping
static int scrx, scry, scrw, scrh;

void V_setrect(int x, int w, int y, int h) {
    scrx = x;
    scrw = w;
    scry = y;
    scrh = h;
}

void V_clr(int x, int w, int y, int h, unsigned char color) {
    // if (x >= VIEW_PIXEL_W || y >= VIEW_PIXEL_H || w <= 0 || h <= 0) return;
    // if (x + w <= 0 || y + h <= 0) return;
    // int x0 = x < 0 ? 0 : x;
    // int y0 = y < 0 ? 0 : y;
    // int x1 = x + w > VIEW_PIXEL_W ? VIEW_PIXEL_W : x + w;
    // int y1 = y + h > VIEW_PIXEL_H ? VIEW_PIXEL_H : y + h;
    // for (int row = y0; row < y1; ++row) {
    //     uint8_t *dst = frame_buf + row * VIEW_PIXEL_W + x0;
    //     for (int col = x0; col < x1; ++col)
    //         *dst++ = color;
    // }
    framebuf_clear(); // TODO: correct implementation of V_clr (with color fill and clipping), currently used only for sky clear which is fully covered by sky texture, so just clear whole framebuf for simplicity.
}

// Draw texture at (x,y) without offset (same 8-byte header as sprite; used for non-transparent walls)
void V_pic(int x, int y, void *p) {
    const int src_width = (int)read_le16((const uint8_t*)p + 0);
    const int src_height = (int)read_le16((const uint8_t*)p + 2);
    framebuf_blit(x, y,
                (const uint8_t*)p + 8, src_width, src_height,
                0, 0, src_width, src_height,
                false, false);
}
// Draw texture at (x,y) without offset
// Draws both buffers at once
void V_pic_2buf(int x, int y, void *p) {
    const int src_width = (int)read_le16((const uint8_t*)p + 0);
    const int src_height = (int)read_le16((const uint8_t*)p + 2);
    framebuf_blit(x, y,
                (const uint8_t*)p + 8, src_width, src_height,
                0, 0, src_width, src_height,
                false, false);
    framebuf_flip_and_copy();
}

static constexpr int Z_WD = 232;  // match view.cpp WD for tile math (declared early for V_pic_tiled_masked)
static constexpr int Z_WD_HALF = Z_WD / 2;
static constexpr int Z_WD_DIV8 = Z_WD / 8;

// GBA EWRAM range: textures passed here (e.g. sky) must be in EWRAM for fast reads, not ROM.
static constexpr uintptr_t EWRAM_START = 0x02000000u;
static constexpr uintptr_t EWRAM_END   = 0x02040000u;

// Draw texture by tiles only where cell_mask[cell_y][cell_x] != 0. Same view math as Z_drawfld; merges consecutive tiles per row.
// pic must be from M_lock_ewram (EWRAM); if from M_lock (ROM) blit will be slower and this assert will catch it in debug.
void V_pic_tiled_masked(void* pic, int pic_origin_x, int pic_origin_y, const unsigned char* cell_mask) {
    const uintptr_t addr = (uintptr_t)pic;
    // BN_ASSERT(addr >= EWRAM_START && addr < EWRAM_END, "V_pic_tiled_masked: texture must be in EWRAM (use M_lock_ewram for sky), ptr=", (void*)pic); // TODO: fast sky
    const int tex_w = (int)read_le16((const uint8_t*)pic + 0);
    const int tex_h = (int)read_le16((const uint8_t*)pic + 2);
    const uint8_t* tex = (const uint8_t*)pic + 8;

    const int dx = (w_x_draw - Z_WD / 2) & 7;
    const int sx0 = ((w_x_draw - Z_WD / 2) >> 3) + 1 - MAXTXW;
    const int ex = sx0 + MAXTXW + Z_WD / 8 + 1;
    int scr_x_base = -dx + (1 - MAXTXW) * CELW;

    const int dy = (w_y - w_ht / 2) & 7;
    int sy = ((w_y - w_ht / 2) >> 3) + 1 - MAXTXH;
    const int ey = sy + (w_ht >> 3) + 9;
    int scr_y = -dy + 1 + w_o + (1 - MAXTXH) * CELH;

    for (; sy < ey; ++sy, scr_y += CELH) {
        if (sy < 0 || sy >= FLDH) continue;
        int sx = sx0;
        int x = scr_x_base;
        while (sx < ex) {
            if (sx < 0 || sx >= FLDW || cell_mask[sy * FLDW + sx] == 0) {
                ++sx; x += CELW;
                continue;
            }
            int sx_start = sx;
            while (sx < ex && sx >= 0 && sx < FLDW && cell_mask[sy * FLDW + sx] != 0) {
                ++sx; x += CELW;
            }
            int sx_end = sx - 1;
            const int run_w = (sx_end - sx_start + 1) * CELW;
            const int dst_x = scr_x_base + (sx_start - sx0) * CELW;
            int roi_src_x = dst_x - pic_origin_x;
            int roi_src_y = scr_y - pic_origin_y;
            int clip_w = run_w, clip_h = CELH;
            int blit_dst_x = dst_x, blit_dst_y = scr_y;
            if (roi_src_x < 0) { clip_w += roi_src_x; blit_dst_x -= roi_src_x; roi_src_x = 0; }
            if (roi_src_y < 0) { clip_h += roi_src_y; blit_dst_y -= roi_src_y; roi_src_y = 0; }
            if (roi_src_x + clip_w > tex_w) clip_w = tex_w - roi_src_x;
            if (roi_src_y + clip_h > tex_h) clip_h = tex_h - roi_src_y;
            if (clip_w > 0 && clip_h > 0) {
                framebuf_blit(blit_dst_x, blit_dst_y, tex, tex_w, tex_h,
                              roi_src_x, roi_src_y, clip_w, clip_h, false, false);
            }
        }
    }
}

// Заполняет горизонтальную полосу из num_tiles тайлов 8x8 (tex — пиксели без заголовка). DMA по строкам.
// Заполняет полосу: num_tiles копий текстуры tile_w x TILE_SIZE (tile_w кратен TILE_SIZE: 8,16,24,32...).
static void fill_tile_strip(uint8_t* buf, const uint8_t* tex, int num_tiles, int tile_w) {
    constexpr int th = TILE_SIZE;
    for (int r = 0; r < th; ++r) {
        const uint8_t* src = tex + r * tile_w;
        uint8_t* dst = buf + r * VIEW_PIXEL_W;
        for (int i = 0; i < num_tiles; ++i) {
            if ((tile_w % 4) == 0)
                bn::hw::dma::copy_words(src, tile_w / 4, dst);
            else if ((tile_w % 2) == 0)
                bn::hw::dma::copy_half_words(src, tile_w / 2, dst);
            else
                for (int k = 0; k < tile_w; ++k) dst[k] = src[k];
            dst += tile_w;
        }
    }
}

// Z_drawfld: draw field tiles (back or front). Серии одинаковых 8x8 тайлов рисуем одним blit (как main_v6).
extern void *walp[256];
extern unsigned int walf[256];
//
// void Z_drawfld(unsigned char *field) {
//     const int dx = (w_x_draw - Z_WD / 2) & 7;
//     const int sx0 = ((w_x_draw - Z_WD / 2) >> 3) + 1 - MAXTXW;
//     const int ex = sx0 + MAXTXW + Z_WD / 8 + 1;
//     int scr_x_base = -dx + (1 - MAXTXW) * CELW;
//
//     const int dy = (w_y - w_ht / 2) & 7;
//     int sy = ((w_y - w_ht / 2) >> 3) + 1 - MAXTXH;
//     const int ey = sy + (w_ht >> 3) + 9;
//     int scr_y = -dy + 1 + w_o + (1 - MAXTXH) * CELH;
//
//     constexpr int max_strip_tiles = VIEW_PIXEL_W / TILE_SIZE;
//
//     for (; sy < ey; ++sy, scr_y += CELH) {
//         if (sy < 0 || sy >= FLDH) continue;
//         int sx = sx0;
//         int x = scr_x_base;
//         while (sx < ex) {
//             if (sx < 0 || sx >= FLDW) {
//                 ++sx; x += CELW;
//                 continue;
//             }
//             const unsigned char c = field[sy * FLDW + sx];
//             if (c == 0) {
//                 ++sx; x += CELW;
//                 continue;
//             }
//             void *p = walp[c];
//             if ((uintptr_t)p <= 3u) {
//                 ++sx; x += CELW;
//                 continue;
//             }
//             const uint8_t* pic = (const uint8_t*)p;
//             const int tw = (int)read_le16(pic + 0), th = (int)read_le16(pic + 2);
//             int run_end = sx + 1;
//             if (tw == TILE_SIZE && th == TILE_SIZE) {
//                 while (run_end < ex && run_end < FLDW && field[sy * FLDW + run_end] == c)
//                     ++run_end;
//             }
//             const int run_len = run_end - sx;
//             if (run_len >= 2 && tw == TILE_SIZE && th == TILE_SIZE) {
//                 const int draw_len = run_len > max_strip_tiles ? max_strip_tiles : run_len;
//                 const int strip_w = draw_len * CELW;
//                 fill_tile_strip(tile_strip_buf, pic + 8, draw_len);
//                 if (walf[c] & 1) {
//                     framebuf_blit(x, scr_y, tile_strip_buf, VIEW_PIXEL_W, TILE_SIZE,
//                              0, 0, strip_w, TILE_SIZE, true, false);
//                 } else {
//                     framebuf_blit(x, scr_y, tile_strip_buf, VIEW_PIXEL_W, TILE_SIZE,
//                              0, 0, strip_w, TILE_SIZE, false, false);
//                 }
//
//                 sx += draw_len;
//                 x += draw_len * CELW;
//             } else {
//                 // if (walf[c] & 1)
//                 //     V_spr(x, scr_y, p);
//                 // else
//                 //     V_pic(x, scr_y, p);
//                 ++sx;
//                 x += CELW;
//             }
//         }
//     }
// }

void V_apply_water_filter_to(uint32_t* buffer_base, int row_stride_words) {
    const int dx = (w_x_draw - Z_WD / 2) & 7;
    const int sx0 = ((w_x_draw - Z_WD / 2) >> 3) + 1 - MAXTXW;
    const int ex = sx0 + MAXTXW + Z_WD / 8 + 1;
    int scr_x_base = -dx + (1 - MAXTXW) * CELW;
    const int dy = (w_y - w_ht / 2) & 7;
    int sy = ((w_y - w_ht / 2) >> 3) + 1 - MAXTXH;
    const int ey = sy + (w_ht >> 3) + 9;
    int scr_y = -dy + 1 + w_o + (1 - MAXTXH) * CELH;

    for (; sy < ey; ++sy, scr_y += CELH) {
        if (sy < 0 || sy >= FLDH) continue;
        int sx = sx0;
        int x = scr_x_base;
        while (sx < ex) {
            if (sx < 0 || sx >= FLDW) {
                ++sx; x += CELW;
                continue;
            }
            const unsigned char c = fldf[sy][sx];
            if (c == 0) {
                ++sx; x += CELW;
                continue;
            }
            const uintptr_t wt = (uintptr_t)walp[c];
            if (wt < 1u || wt > 3u) {
                ++sx; x += CELW;
                continue;
            }
            const int type = (int)(wt - 1);
            if (x + CELW <= 0 || x >= VIEW_PIXEL_W || scr_y + CELH <= 0 || scr_y >= VIEW_PIXEL_H) {
                ++sx; x += CELW;
                continue;
            }
            // Серия подряд идущих тайлов воды того же типа (как run в Z_drawfld).
            int run_end = sx + 1;
            while (run_end < ex && run_end < FLDW) {
                const unsigned char nc = fldf[sy][run_end];
                if (nc == 0) break;
                const uintptr_t nwt = (uintptr_t)walp[nc];
                if (nwt < 1u || nwt > 3u) break;
                if ((int)(nwt - 1) != type) break;
                ++run_end;
            }
            const int run_tiles = run_end - sx;
            const int strip_w_px = run_tiles * CELW;
            const int row0 = scr_y < 0 ? -scr_y : 0;
            const int row1 = scr_y + CELH > VIEW_PIXEL_H ? VIEW_PIXEL_H - scr_y : CELH;
            const int col0 = x < 0 ? -x : 0;
            const int col1 = x + strip_w_px > VIEW_PIXEL_W ? VIEW_PIXEL_W - x : strip_w_px;
            const int strip_copy_w = col1 - col0;
            if (strip_copy_w <= 0) {
                sx = run_end;
                x += strip_w_px;
                continue;
            }
            const uint8_t* remap = water_remap[type];
            const int strip_nw = strip_copy_w >> 2;

            // По одной строке: DMA → remap в полоске (1 строка) → DMA обратно
            for (int row = row0; row < row1; ++row) {
                uint32_t* vram_row = buffer_base + (scr_y + row) * row_stride_words + ((x + col0) >> 2);
                bn::hw::dma::copy_words(vram_row, strip_nw, pixel_strip_buf);
                for (int i = 0; i < strip_nw; ++i) {
                    pixel_strip_buf[i] = remap[pixel_strip_buf[i] & 0xFF] | (remap[(pixel_strip_buf[i] >> 8) & 0xFF] << 8) | (remap[(pixel_strip_buf[i] >> 16) & 0xFF] << 16) | (remap[(pixel_strip_buf[i] >> 24) & 0xFF] << 24);
                }
                bn::hw::dma::copy_words(pixel_strip_buf, strip_nw, vram_row);

            }

            sx = run_end;
            x += strip_w_px;
        }
    }
}

static void BN_CODE_IWRAM blit_row_transparent(const uint32_t* src, uint16_t* dst, int nwords) {
    for (int j = 0; j < nwords; ++j) {
        const uint32_t px4 = src[j];
        if (px4 == 0) { dst += 2; continue; }
        const uint16_t lo = px4 & 0xFFFF;
        const uint16_t hi = px4 >> 16;
        if (lo) {
            if ((lo & 0xFF) && (lo >> 8)) { dst[0] = lo; }
            else { uint16_t v = dst[0]; if (lo & 0xFF) v = (v & 0xFF00) | (lo & 0xFF); if (lo >> 8) v = (v & 0x00FF) | (lo & 0xFF00); dst[0] = v; }
        }
        if (hi) {
            if ((hi & 0xFF) && (hi >> 8)) { dst[1] = hi; }
            else { uint16_t v = dst[1]; if (hi & 0xFF) v = (v & 0xFF00) | (hi & 0xFF); if (hi >> 8) v = (v & 0x00FF) | (hi & 0xFF00); dst[1] = v; }
        }
        dst += 2;
    }
}

void BN_CODE_IWRAM Z_drawfld(unsigned char *field) {
    const int dx = (w_x_draw - Z_WD_HALF) & 7;
    const int sx0 = ((w_x_draw - Z_WD_HALF) >> 3) + 1 - MAXTXW;
    const int ex = sx0 + MAXTXW + Z_WD_DIV8 + 1;
    const int scr_x_base = -dx + (1 - MAXTXW) * CELW;

    const int dy = (w_y - w_ht_half) & 7;
    int sy = ((w_y - w_ht_half) >> 3) + 1 - MAXTXH;
    const int ey = sy + (w_ht >> 3) + 9;
    int scr_y = -dy + 1 + w_o + (1 - MAXTXH) * CELH;

    uint8_t* const vram = (uint8_t*)bn::display_manager::bitmap_page();
    const int sx_lo = sx0 < 0 ? 0 : sx0;
    const int sx_hi = ex < FLDW ? ex : FLDW;
    const int x_base = scr_x_base - (sx0 << 3);

    for (; sy < ey; ++sy, scr_y += CELH) {
        if (sy < 0 || sy >= FLDH) continue;
        const unsigned char* const frow = field + sy * FLDW;
        int sx = sx_lo;

        while (sx < sx_hi) {
            const unsigned char c = frow[sx];
            if (c == 0 || (uintptr_t)walp[c] <= 3u) { ++sx; continue; }

            const uint8_t* const pic = (const uint8_t*)walp[c];
            const int tw = (int)read_le16(pic);
            const int th = (int)read_le16(pic + 2);
            if (tw < 1 || th < 1) { ++sx; continue; }
            const int ox = (int)(int16_t)read_le16(pic + 4);
            const int oy = (int)(int16_t)read_le16(pic + 6);
            const int cpt = (tw + 7) >> 3;
            const uint8_t* const tex = pic + 8;
            const bool transparent = (walf[c] & 1) != 0;
            const bool can_batch = ((tw & 7) == 0) && ox == 0 && oy == 0;

            if (can_batch) {
                int num_tex = 1;
                if (cpt == 1) {
                    int end_sx = sx + 1;
                    while (end_sx < sx_hi && frow[end_sx] == c) ++end_sx;
                    num_tex = end_sx - sx;
                } else {
                    int check = sx + cpt;
                    while (check + cpt <= sx_hi && frow[check] == c) {
                        ++num_tex;
                        check += cpt;
                    }
                }
                const int run_cells = num_tex * cpt;
                const int x = x_base + (sx << 3);
                const int strip_px = num_tex * tw;

                int el = x < 0 ? 0 : x;
                el = (el + 3) & ~3;
                int er = x + strip_px;
                if (er > VIEW_PIXEL_W) er = VIEW_PIXEL_W;
                er &= ~3;
                const int cw = er - el;

                if (cw > 0) {
                    const int buf_off = el - x;
                    int py0 = scr_y < 0 ? -scr_y : 0;
                    int py1 = scr_y + th > VIEW_PIXEL_H ? VIEW_PIXEL_H - scr_y : th;

                    if (py0 < py1) {
                        const int nw = cw >> 2;
                        uint8_t* vr = vram + (scr_y + py0) * 240 + el;
                        const uint8_t* tr = tex + py0 * tw;

                        {
                            const int tw_words = tw >> 2;
                            const int off_words = (buf_off >> 2) % tw_words;
                            for (int py = py0; py < py1; ++py) {
                                const uint32_t* src = (const uint32_t*)tr;
                                uint32_t* d = (uint32_t*)tex_line_buf;
                                int rem = nw;
                                int skip = off_words;
                                while (rem > 0) {
                                    int chunk = tw_words - skip;
                                    if (chunk > rem) chunk = rem;
                                    const uint32_t* s = src + skip;
                                    for (int k = 0; k < chunk; ++k)
                                        d[k] = s[k];
                                    d += chunk;
                                    rem -= chunk;
                                    skip = 0;
                                }
                                if (!transparent) {
                                    bn::hw::dma::copy_words(tex_line_buf, nw, vr);
                                } else {
                                    blit_row_transparent((const uint32_t*)tex_line_buf, (uint16_t*)vr, nw);
                                }
                                vr += 240; tr += tw;
                            }
                        }
                    }
                }
                sx += run_cells;
            } else {
                framebuf_blit(x_base + (sx << 3) - ox, scr_y - oy,
                              tex, tw, th, 0, 0, tw, th, transparent, false);
                sx += cpt;
            }
        }
    }
}
