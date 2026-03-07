#include "bn_core.h"
// #include "bn_audio.h"
#include "bn_keypad.h"
#include "bn_display.h"
#include "bn_fixed.h"
#include "bn_fixed_point.h"
#include "bn_size.h"
#include "bn_optional.h"
#include "bn_sprite_ptr.h"
#include "bn_sprite_item.h"
#include "bn_camera_ptr.h"
#include "bn_affine_bg_ptr.h"
#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_builder.h"
#include "bn_regular_bg_map_ptr.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_tiles_item.h"
#include "bn_regular_bg_tiles_ptr.h"
#include "bn_bg_palette_ptr.h"
#include "bn_bg_palette_item.h"
#include "bn_tile.h"
#include "bn_timer.h"
#include "bn_timers.h"
#include "bn_color.h"
#include "bn_span.h"
#include "bn_vector.h"
#include "bn_assert.h"
#include "bn_log.h"
#include "bn_string.h"


#include "doom2d_wad.h"
#include <cstring>
#include <tonc_memmap.h>

#include "bn_format.h"

#include "bn_music.h"
#include "bn_music_items.h"
#include "bn_sound.h"
#include "bn_sound_items.h"


#include "bn_bitmap_bg.h"
#include "bn_palette_bitmap_bg_painter.h"
#include "bn_palette_bitmap_bg_ptr.h"
#include "bn_palette_bitmap_roi.h"
#include "bn_bg_palettes.h"

#include "bn_math.h"
#include "bn_regular_bg_position_hbe_ptr.h"
// #include "bn_sound_item.h"
#include "bn_sprite_text_generator.h"

#include "common_fixed_8x16_sprite_font.h"
// #include "../butano/butano/src/bn_bgs_manager.h"
#include "bn_sp_direct_bitmap_bg_painter.h"
#include "../butano/butano/hw/include/bn_hw_dma.h"
#include "../butano/butano/hw/include/bn_hw_irq.h"
// #include "../butano/butano/src/bn_display_manager.h"


#include "bn_sprite_items_stbar_top_0.h"
#include "bn_sprite_items_stbar_top_1.h"
#include "bn_sprite_items_stbar_top_2.h"
#include "bn_sprite_items_stbar_top_3.h"
#include "bn_sprite_items_stbar_bottom_0.h"
#include "bn_sprite_items_stbar_bottom_1.h"
#include "bn_sprite_items_stbar_bottom_2.h"
#include "bn_sprite_items_stbar_bottom_3.h"
#include "build_info.h"
#include "memory.h"
#include "menu.h"

#include "game/glob.h"
#include "game/files.h"
#include "game/view.h"
#include "game/monster.h"
#include "game/player.h"
#include "misc.h"
#include "sound.h"
#include "renderer.h"
#include "debug.h"
// #define ALLOW_SKIP_FRAMES  // если G_draw не успел до след. таймера — пропустить отрисовку (макс. 1 раз подряд)

constexpr int TILE_SIZE = 8;
constexpr int TILES_H = 31-0;
constexpr int TILES_V = 21-5;
// constexpr int VIEW_PIXEL_W = bn::bitmap_bg::palette_width();
// constexpr int VIEW_PIXEL_H = bn::bitmap_bg::palette_height();
constexpr int VIEW_PIXEL_W = (TILES_H - 1) * TILE_SIZE;
constexpr int VIEW_PIXEL_H = (TILES_V - 1) * TILE_SIZE;

// Map cells: 32x32, floor at bottom and a simple platform
bn::vector<bn::sprite_ptr, 16> _text_sprites;
bn::sprite_text_generator _text_generator = bn::sprite_text_generator(common::fixed_8x16_sprite_font);

static const char* HEX = "0123456789ABCDEF";

bn::string<3> to_hex(const uint8_t b) {
    bn::string<3> s;
    s += HEX[(b >> 4) & 0xF];
    s += HEX[b & 0xF];
    return s;
}

bn::string_view to_hex_address(const int p) {
    return bn::format<11>("0x{}{}{}{}",
                          to_hex(p >> 24 & 0xFF),
                          to_hex(p >> 16 & 0xFF),
                          to_hex(p >> 8 & 0xFF),
                          to_hex(p & 0xFF));
}

char g_ticks = 0;
constexpr char G_TARGET_FRAMERATE = 18;
constexpr char G_TICKS_SKIP = 60 / G_TARGET_FRAMERATE;
bn::fixed inc_cpu_usage;
constexpr bn::fixed G_CPU_USAGE_DIVISOR = bn::fixed(60).division(G_TARGET_FRAMERATE);

short mus_vol=128;
int gamma=0;
void setgamma(int);

static bn::fixed display_brightness = bn::fixed(0.0);
static bn::fixed display_contrast = bn::fixed(0.0);
static bn::fixed pain_fade = bn::fixed(0.0);
static bn::optional<bn::sprite_ptr> ui_sprites[8];

// // Заливает полосы по бокам буфера (для скрытия offset полотна при canvas_offset_x).
// static void BN_CODE_IWRAM draw_cropping_stripes(const int color_index, const int canvas_offset_x, const bool debug) {
//     // Left stripe:
//     int color_idx = debug ? 7 * 16 : color_index;
//     if (canvas_offset_x != 0) {
//         uint8_t* dst_row = frame_buf;
//         for (int row = 0; row < VIEW_PIXEL_H; ++row) {
//             for (int col = 0; col < canvas_offset_x; ++col) {
//                 dst_row[col] = color_idx;
//             }
//             dst_row += VIEW_PIXEL_W;
//         }
//     }
//
//     color_idx = debug ? 11 * 16 : color_index;
//     // Right stripe:
//     uint8_t* dst_row = frame_buf + VIEW_PIXEL_W - 8 + canvas_offset_x;
//     for (int row = 0; row < VIEW_PIXEL_H; ++row) {
//         for (int col = 0; col < 4; ++col) {
//             dst_row[col] = color_idx;
//         }
//         dst_row += VIEW_PIXEL_W;
//     }
// }

bool initialized = false;
volatile bool game_tick_pending = false;  // таймер только выставляет; отрисовка в main (VRAM только не в ISR)
static bool draw_pending = false;    // кадр готов к отрисовке (после G_act)
#ifdef ALLOW_SKIP_FRAMES
static bool skip_next_draw = false;  // пропустить отрисовку следующего кадра
#endif

void BN_CODE_IWRAM ISR_VBlank() {
}

// GBA Timer 1: 4000104=L (counter/reload), 4000106=H (control). H: bits 0-1 prescaler, 6=IRQ, 7=start.
#define REG_IF (*(volatile uint16_t*)0x4000202)
#define IRQ_TIMER1 (1u << 3)

// Период 18.2 Hz при prescaler 1024: 16384/18.2 ≈ 900 тиков, reload = 65536 - 900
constexpr uint16_t TM1_RELOAD_18H2 = 0x10000u - (16777216u / 1024 * 10 / 182);

void on_timer1() {
    // REG_IF = IRQ_TIMER1;
    // REG_TM1CNT_L = TM1_RELOAD_18H2;

    if (!initialized) {
        return;
    }

    game_tick_pending = true;
}

void init_timers() {
    BN_LOG("init_timers: setting up Timer 1 for game ticks");
    // 18.2 Hz: CPU 16.78 MHz, prescaler 1024 → 16384 Hz
    const uint16_t reload = TM1_RELOAD_18H2;
    REG_TM1CNT_H = 0;                    // стоп перед настройкой
    REG_TM1CNT_L = reload;
    bn::hw::irq::set_isr(bn::hw::irq::id::TIMER1, on_timer1);
    bn::hw::irq::enable(bn::hw::irq::id::TIMER1);
    // Prescaler 1024 (3) | IRQ (1<<6) | Start (1<<7) — без этого таймер не запускается
    REG_TM1CNT_H = (3 << 0) | (1 << 6) | (1 << 7);
}

char * strcpy(char *dst, const char *src) {
    int n = 0;
    while (n < _MAX_PATH - 1 && src[n] != '\0') {
        dst[n] = src[n];
        ++n;
    }
    dst[n] = '\0';
    return dst;
}

char * strncpy(char *dst, const char *src, unsigned n) {
    unsigned i = 0;
    while (i < n && src[i] != '\0') {
        dst[i] = src[i];
        ++i;
    }
    while (i < n) {
        dst[i] = '\0';
        ++i;
    }
    return dst;
}

int	rand (void) {
    return random(0x7FFF);
}

int strnicmp(const char *s1, const char *s2, unsigned n) {
    for (unsigned i = 0; i < n; ++i) {
        unsigned char c1 = (unsigned char)s1[i];
        unsigned char c2 = (unsigned char)s2[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return (int)c1 - (int)c2;
        if (c1 == '\0') return 0;
    }
    return 0;
}

int memicmp(const void *s1, const void *s2, unsigned n) {
    const unsigned char *p1 = (const unsigned char *)s1;
    const unsigned char *p2 = (const unsigned char *)s2;
    for (unsigned i = 0; i < n; ++i) {
        unsigned char c1 = p1[i];
        unsigned char c2 = p2[i];
        if (c1 >= 'A' && c1 <= 'Z') c1 += 32;
        if (c2 >= 'A' && c2 <= 'Z') c2 += 32;
        if (c1 != c2) return (int)c1 - (int)c2;
    }
    return 0;
}

unsigned strlen(const char* s) {
    const char* p = s;
    while (*p) ++p;
    return (unsigned)(p - s);
}

[[noreturn]] int main() {
    bn::core::init(bn::color(0, 0, 0));
    init_timers();

    bn::bg_palettes::set_intensity(bn::fixed(0.5));

    BN_LOG("Fast EWRAM available: ", bn::memory::fast_ewram());
    BN_LOG("Slow GamePak found: ", bn::core::slow_game_pak());

    pl1.ku=0x48;pl1.kd=0x50;pl1.kl=0x4B;pl1.kr=0x4D;pl1.kf=0xB8;pl1.kj=0x9D;
    pl1.kwl=0x47;pl1.kwr=0x49;pl1.kp=0x36;
    pl1.id=-1;
    pl2.ku=0x11;pl2.kd=0x1F;pl2.kl=0x1E;pl2.kr=0x20;pl2.kf=0x3A;pl2.kj=0x0F;
    pl2.kwl=0x10;pl2.kwr=0x12;pl2.kp=0x2A;
    pl2.id=-2;
    // textmode();gotoxy(1,2);
    // //  _dos_setvect(0,dbzfunc);
    // randomize();
    F_startup();
    F_addwad("DOOM2D.WAD", doom2d_wad, (unsigned)doom2d_wad_size);
    F_initwads();
    // F_set_snddrv();
    // if(mem_chk_sz) {
    //     logo("** захапывается %dK памяти...",mem_chk_sz);
    //     logo("%s\n",(malloc(mem_chk_sz<<10))?"OK":"ОШИБКА");
    // }
    M_startup();
    F_allocres();
    // F_loadres(F_getresid("MIXMAP"),mixmap,0,0x10000);
    // F_loadres(F_getresid("COLORMAP"),clrmap,0,256*12);

    G_init();
    BN_LOG("  available EWRAM: ", bn::memory::available_alloc_ewram());
#ifdef D2D_DEBUG_ENABLE
    DBG_init();
#endif
    // logo("K_init: настройка клавиатуры\n");
    // K_slow();K_init();
    // logo("T_init: настройка таймера\n");
    // T_init();
    // logo("S_init: настройка звука\n");
    // S_init();
    // logo("V_init: настройка видео\n");
    // if(V_init()!=0) ERR_failinit("Не могу установить видеорежим VGA");
    R_Init();
    setgamma(gamma);
    // V_setscr(scrbuf);
    // harderr_inst(harderr_handler);
    GM_init();
    initialized = true;
    F_loadmus("MENU");
    S_startmusic();

    // TEST: start with normal player state (alive, with weapon). Use G_start() when menu exists.
    // G_start_test_level(); // TODO: remove
    while (true) {
        if (bn::keypad::any_pressed()) {
            unsigned pressed = 0;
            using bn::keypad::key_type;
            if (bn::keypad::a_pressed()) pressed |= unsigned(key_type::A);
            if (bn::keypad::b_pressed()) pressed |= unsigned(key_type::B);
            if (bn::keypad::select_pressed()) pressed |= unsigned(key_type::SELECT);
            if (bn::keypad::start_pressed()) pressed |= unsigned(key_type::START);
            if (bn::keypad::right_pressed()) pressed |= unsigned(key_type::RIGHT);
            if (bn::keypad::left_pressed()) pressed |= unsigned(key_type::LEFT);
            if (bn::keypad::up_pressed()) pressed |= unsigned(key_type::UP);
            if (bn::keypad::down_pressed()) pressed |= unsigned(key_type::DOWN);
            if (bn::keypad::r_pressed()) pressed |= unsigned(key_type::R);
            if (bn::keypad::l_pressed()) pressed |= unsigned(key_type::L);
            G_keyf(pressed);
        }
        if (game_tick_pending) {
            int avg_cpu_usage = inc_cpu_usage.division(G_CPU_USAGE_DIVISOR).multiplication(100).right_shift_integer();
            inc_cpu_usage = 0;
            game_tick_pending = false;
            _text_sprites.clear();
            _text_generator.set_left_alignment();
            _text_generator.generate(-120, -80+(g_st==GS_GAME ? 16 : 4), bn::format<32>("CPU:{}%", avg_cpu_usage), _text_sprites);
            if (g_st==GS_TITLE) {
                _text_generator.generate(-120, 74, bn::format<23>("v{}", D2DGBA_BUILD_STRING), _text_sprites);
            }

            G_act();
            draw_pending = true;
        }


        if (draw_pending) {
#ifdef ALLOW_SKIP_FRAMES
            if (skip_next_draw) {
                skip_next_draw = false;
                draw_pending = false;
            } else {
                G_draw();
                R_blit();
                draw_pending = false;
                if (game_tick_pending) skip_next_draw = true;
            }
#else
            G_draw();
            R_blit();
            draw_pending = false;
#endif
        }

        inc_cpu_usage += bn::core::current_cpu_usage();
        S_sound_tick();
        bn::core::update();
    }
}
