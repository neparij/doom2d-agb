#include "renderer.h"
#include <cstdint>
#include <cstring>

#include "files.h"

#include "bn_log.h"
#include "bn_display.h"
#include "bn_bg_palettes.h"
#include "bn_sprite_ptr.h"
#include "stbf_font_sprite_font.h"
#include "stcfn_font_sprite_font.h"
#include "stt_font_sprite_font.h"
#include "wi_font_sprite_font.h"


bn::optional<bn::sprite_text_generator> text_stbf;
bn::optional<bn::sprite_text_generator> text_stcfn;
bn::optional<bn::sprite_text_generator> text_stt;
bn::optional<bn::sprite_text_generator> text_wi;
bn::vector<bn::sprite_ptr, 13> st_gfx_sprites;
bn::vector<bn::sprite_ptr, 64> st_text_sprites;
bn::vector<bn::sprite_ptr, 64> menu_text_sprites;
bn::vector<bn::sprite_ptr, 2> menu_cursor_sprites;
bn::vector<bn::sprite_ptr, 64> inter_text_sprites;

void R_Init(void) {
    BN_LOG("R_Init: Renderer initialization started");

    bn::color pal_buf[256];
    char playpal_name[8];
    memcpy(playpal_name, "PLAYPAL", 8);
    F_loadpal(F_getresid(playpal_name), pal_buf, 0, 256);

    char colormap_name[8];
    memcpy(colormap_name, "COLORMAP", 8);
    const int colormap_id = F_getresid(colormap_name);
    F_loadres(colormap_id, &water_remap[0][0], 256 * 8, 256 * 3);

    bn::span<const bn::color> pal_span(pal_buf, 256);
    bn::bg_palette_item pal_item(pal_span, bn::bpp_mode::BPP_8);
    bg_palette = bn::bg_palette_ptr::create(pal_item);
    canvas = bn::palette_bitmap_bg_ptr::create(bg_palette.value());
    canvas_painter = bn::palette_bitmap_bg_painter(canvas.value());

    text_stbf = bn::sprite_text_generator(stbf_font_sprite_font);
    text_stcfn = bn::sprite_text_generator(stcfn_font_sprite_font);
    text_stt = bn::sprite_text_generator(stt_font_sprite_font);
    text_wi = bn::sprite_text_generator(wi_font_sprite_font);

    text_stbf->set_one_sprite_per_character(true);
    text_stcfn->set_one_sprite_per_character(true);
    text_stt->set_one_sprite_per_character(true);
    text_wi->set_one_sprite_per_character(true);

    st_gfx_sprites.clear();
    st_text_sprites.clear();
    menu_text_sprites.clear();
    menu_cursor_sprites.clear();
    inter_text_sprites.clear();
}

void R_clear_sprites(void) {
    st_gfx_sprites.clear();
    st_text_sprites.clear();
    menu_text_sprites.clear();
    menu_cursor_sprites.clear();
    inter_text_sprites.clear();
}


extern int sky_type;
extern int lt_time;

void V_apply_palette_effects(int h) {
    bn::bg_palettes::set_brightness(0.0);
    bn::bg_palettes::set_fade_intensity(0);
    bn::bg_palettes::set_grayscale_intensity(0.0);
    bn::bg_palettes::set_inverted(false);

    if (sky_type == 2) {
        if (lt_time == -4 || lt_time == -2) {
            bn::bg_palettes::set_brightness(0.5);
        }
    }
    if (h == 6) {
        // Invul
        bn::bg_palettes::set_grayscale_intensity(1.0);
        bn::bg_palettes::set_inverted(true);
    } else if (h > 0) {
        // h 1 to 5 inclusive
        constexpr bn::fixed max_payne(0.9);
        const bn::fixed pain_fade = bn::fixed(0.2).multiplication(h);
        bn::bg_palettes::set_fade(bn::color(31, 0, 0), pain_fade > max_payne ? max_payne : pain_fade);
    }
}

// TODO: Remove duplicated
constexpr int TILE_SIZE = 8;
constexpr int TILES_H = 31-0;
constexpr int TILES_V = 21-5;
constexpr int VIEW_PIXEL_W = (TILES_H - 1) * TILE_SIZE;
constexpr int VIEW_PIXEL_H = (TILES_V - 1) * TILE_SIZE;
constexpr int VIEW_PIXEL_FULLSCREEN_W = bn::display::width();
constexpr int VIEW_PIXEL_FULLSCREEN_H = bn::display::height();
void V_apply_cropping_stripes(uint32_t* buffer_base, const int color_index) {
    // VRAM: 16-bit writes, 2 pixels per word. Low byte = left pixel, high byte = right.
    uint16_t* buf = reinterpret_cast<uint16_t*>(buffer_base);
    constexpr int row_stride = VIEW_PIXEL_W / 2;  // words per row

    // Left stripe:
    if (canvas_offset_x != 0) {
        const int n = canvas_offset_x;
        const int n_even = n & ~1;  // max even count
        const uint16_t pair = (uint16_t)(color_index | (color_index << 8));

        for (int row = 0; row < VIEW_PIXEL_H; ++row) {
            uint16_t* row_ptr = buf + row * row_stride;

            // Write full pairs
            for (int i = 0; i < n_even; i += 2) {
                row_ptr[i / 2] = pair;
            }

            // Odd remainder: last pixel + preserve first canvas pixel
            if (n & 1) {
                uint16_t* w = row_ptr + n_even / 2;
                *w = (uint16_t)((*w & 0xFF00) | color_index);
            }
        }
    }

    // Right stripe: 4 pixels at VIEW_PIXEL_W - 8 + canvas_offset_x
    const int right_start = VIEW_PIXEL_W - 8 + canvas_offset_x;
    const uint16_t pair = (uint16_t)(color_index | (color_index << 8));

    for (int row = 0; row < VIEW_PIXEL_H; ++row) {
        uint16_t* row_ptr = buf + row * row_stride;
        int col = right_start;

        if (col & 1) {
            // Odd start: step left, write (original | color_index)
            uint16_t* w = row_ptr + (col - 1) / 2;
            *w = (uint16_t)((*w & 0x00FF) | (color_index << 8));
            col++;
        }

        // Pairs of pixels
        while (col < right_start + 3) {
            row_ptr[col / 2] = pair;
            col += 2;
        }

        if (col < right_start + 4) {
            // Last pixel: preserve pixel to the right
            uint16_t* w = row_ptr + col / 2;
            *w = (uint16_t)((*w & 0xFF00) | color_index);
        }
    }
}


// TODO: setgamma
void setgamma(int g) {
  int t;
 //
 //  if(g>4) g=4;
 //  if(g<0) g=0;
 //  gamma=g;
 //  for(t=0;t<256;++t) {
	// std_pal[t][0]=gamcor[gamma][main_pal[t][0]];
	// std_pal[t][1]=gamcor[gamma][main_pal[t][1]];
	// std_pal[t][2]=gamcor[gamma][main_pal[t][2]];
 //  }
 //  VP_setall(std_pal);
}