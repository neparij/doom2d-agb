#include "save.h"

#include <cstring>

#include "bn_sram.h"
#include "bn_log.h"

#include "view.h"
#include "player.h"
#include "renderer.h"
#include "sound.h"

static constexpr char kMagic[8] = {'D', '2', 'D', 'G', 'B', 'A', '1', '\0'};

struct SV_settings {
    short snd_vol;
    short mus_vol;
    short pal_level;
};

struct SV_slot {
    unsigned char used;
    unsigned char map;
    char name[8];
    short life;
    short armor;
    short ammo;
    short shel;
    short rock;
    short cell;
    short fuel;
    unsigned short wpns;
    unsigned char wpn;
    unsigned char amul;
    unsigned char keys;
    unsigned char lives;
};

struct SV_cart {
    char magic_head[8];
    SV_settings settings;
    SV_slot slots[SV_SLOTS];
    char magic_tail[8];
};

static SV_cart cart;

static_assert(sizeof(SV_cart) <= 32 * 1024, "save blob does not fit in SRAM");

static int magic_ok(const SV_cart &c) {
    return memcmp(c.magic_head, kMagic, 8) == 0 && memcmp(c.magic_tail, kMagic, 8) == 0;
}

static short clamp128(short v) {
    if (v < 0) return 0;
    if (v > 128) return 128;
    return v;
}

static void fill_settings_from_ram() {
    cart.settings.snd_vol = snd_vol;
    cart.settings.mus_vol = mus_vol;
    cart.settings.pal_level = pal_level;
}

static void apply_settings_to_ram() {
    snd_vol = clamp128(cart.settings.snd_vol);
    mus_vol = clamp128(cart.settings.mus_vol);
    pal_level = clamp128(cart.settings.pal_level);
}

void SV_init(void) {
    bn::sram::read(cart);
    if (!magic_ok(cart)) {
        BN_LOG("SV_init: SRAM unformatted, writing defaults");
        bn::sram::clear(bn::sram::size());
        memset(&cart, 0, sizeof(cart));
        memcpy(cart.magic_head, kMagic, 8);
        memcpy(cart.magic_tail, kMagic, 8);
        fill_settings_from_ram();
        bn::sram::write(cart);
        return;
    }
    apply_settings_to_ram();
    BN_LOG("SV_init: SRAM loaded");
}

void SV_commit(void) {
    memcpy(cart.magic_head, kMagic, 8);
    memcpy(cart.magic_tail, kMagic, 8);
    fill_settings_from_ram();
    bn::sram::write(cart);
}

void SV_save_slot(int i) {
    if (i < 0 || i >= SV_SLOTS) return;
    SV_slot &s = cart.slots[i];
    s.used = 1;
    s.map = g_map;
    memset(s.name, 0, sizeof(s.name));
    s.name[0] = 'M';
    s.name[1] = 'A';
    s.name[2] = 'P';
    s.name[3] = static_cast<char>('0' + (g_map / 10));
    s.name[4] = static_cast<char>('0' + (g_map % 10));
    s.life = static_cast<short>(pl1.life);
    s.armor = static_cast<short>(pl1.armor);
    s.ammo = static_cast<short>(pl1.ammo);
    s.shel = static_cast<short>(pl1.shel);
    s.rock = static_cast<short>(pl1.rock);
    s.cell = static_cast<short>(pl1.cell);
    s.fuel = static_cast<short>(pl1.fuel);
    s.wpns = pl1.wpns;
    s.wpn = static_cast<unsigned char>(pl1.wpn);
    s.amul = pl1.amul;
    s.keys = 0;
    s.lives = static_cast<unsigned char>(pl1.lives);
    SV_commit();
}

void SV_restore_player(int i) {
    if (i < 0 || i >= SV_SLOTS || !cart.slots[i].used) return;
    const SV_slot &s = cart.slots[i];
    pl1.life = s.life;
    pl1.armor = s.armor;
    pl1.ammo = s.ammo;
    pl1.shel = s.shel;
    pl1.rock = s.rock;
    pl1.cell = s.cell;
    pl1.fuel = s.fuel;
    pl1.wpns = s.wpns;
    pl1.wpn = static_cast<char>(s.wpn);
    pl1.amul = s.amul;
    pl1.lives = static_cast<char>(s.lives);
    pl1.drawst = 0xFF;
}

int SV_slot_used(int i) {
    if (i < 0 || i >= SV_SLOTS) return 0;
    return cart.slots[i].used;
}

unsigned char SV_slot_map(int i) {
    if (i < 0 || i >= SV_SLOTS) return 1;
    return cart.slots[i].map;
}

const char *SV_slot_name(int i) {
    if (i < 0 || i >= SV_SLOTS || !cart.slots[i].used) return "---";
    return cart.slots[i].name;
}
