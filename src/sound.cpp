#include "sound.h"
#include "bn_log.h"
#include "bn_audio.h"
#include "bn_config_audio.h"
#include "bn_music.h"
#include "bn_music_items.h"
#include "bn_sound_items.h"
#include "bn_sound.h"

#include "enc.h"

short snd_type = ST_DMA;
bn::optional<bn::music_item> current_music_ref = bn::nullopt;

static bool name_equals(const char* map_name_8, const char* c_str) {
    for (int i = 0; i < 8; ++i) {
        char w = map_name_8[i];
        char c = c_str[i];
        if (w == c) continue;
        if ((w == '\0' || w == ' ') && (c == '\0' || c == ' ')) continue;
        return false;
    }
    return true;
}

const bn::music_item& get_xm_ref(const char* music_name) {
    BN_LOG("get_xm_ref: [", cp866_to_utf8(music_name), "]");
    if (name_equals(music_name, "ALLRIGHT")) return bn::music_items::allright;
    if (name_equals(music_name, "BEST"))     return bn::music_items::best;
    if (name_equals(music_name, "GLAD"))      return bn::music_items::glad;
    if (name_equals(music_name, "MENU"))     return bn::music_items::menu;
    if (name_equals(music_name, "INTERMUS")) return bn::music_items::intermus;
    if (name_equals(music_name, "SUPER"))    return bn::music_items::super;
    if (name_equals(music_name, "\x80\x91\x00\x00\x00\x00\x00\x00") || name_equals(music_name, "\x80\x91") || name_equals(music_name, "AC")) return bn::music_items::as;       // АС: в картах CP866 (80 91), вариант CP1251-shift (AC)
    if (name_equals(music_name, "\x80\x92\x80\x91") || name_equals(music_name, "ATAC")) return bn::music_items::atas;         // АТАС
    if (name_equals(music_name, "\x80\x89\x8E\x89") || name_equals(music_name, "\x80\x79\x8E\x79")) return bn::music_items::ayoy;  // АЙОЙ: в картах CP866 (80 89 8E 89), вариант CP1251-shift (79)
    if (name_equals(music_name, "\x81\x8E\x86\x5F\x8C\x93\x87")) return bn::music_items::bozh_muz;      // БОЖ_МУЗ
    if (name_equals(music_name, "\x81\x9B\x91\x92\x90\x8E\x92\x80")) return bn::music_items::bystrota;     // БЫСТРОТА
    if (name_equals(music_name, "\x91\x5F\x82\x9B\x98\x80\x8A")) return bn::music_items::s_vyshak;       // С_ВЫШАК
    if (name_equals(music_name, "\x8A\x8B\x80\x91\x91")) return bn::music_items::klass;        // КЛАСС
    if (name_equals(music_name, "\x84\x93\x98\x85\x82\x8D\x80\x9F")) return bn::music_items::dushevnaya;   // ДУШЕВНАЯ
    if (name_equals(music_name, "\x95\x85\x95\x85\x95\x85")) return bn::music_items::hehehe;        // ХЕХЕХЕ
    if (name_equals(music_name, "\x95\x8E\x95\x8E\x95\x8E")) return bn::music_items::hohoho;        // ХОХОХО
    if (name_equals(music_name, "\x8A\x8E\x8D\x85\x96")) return bn::music_items::konets;        // КОНЕЦ
    if (name_equals(music_name, "\x8C\x8E\x99\x9C")) return bn::music_items::moshch;        // МОЩЬ
    if (name_equals(music_name, "\x8F\x80\x8B\x80\x92\x80\x23\x36")) return bn::music_items::palata_6;      // ПАЛАТА#6
    if (name_equals(music_name, "\x8F\x90\x88\x8A\x8E\x8B")) return bn::music_items::prikol;        // ПРИКОЛ
    if (name_equals(music_name, "\x8F\x90\x8E\x81\x80")) return bn::music_items::proba;           // ПРОБА
    if (name_equals(music_name, "\x8F\x90\x8E\x91\x92\x8E\x92\x80")) return bn::music_items::prostota;    // ПРОСТОТА
    if (name_equals(music_name, "\x91\x8E\x8B\x8E")) return bn::music_items::solo;          // СОЛО
    if (name_equals(music_name, "\x91\x8F\x8E\x8A")) return bn::music_items::spok;          // СПОК
    if (name_equals(music_name, "\x91\x92\x90\x80\x95")) return bn::music_items::strah;        // СТРАХ
    if (name_equals(music_name, "\x92\x88\x98\x9C")) return bn::music_items::tish;           // ТИШЬ
    if (name_equals(music_name, "\x92\x9B\x5F\x8F\x8E\x81\x85\x84")) return bn::music_items::ty_pobed;      // ТЫ_ПОБЕД
    if (name_equals(music_name, "\x92\x9B\x5F\x92\x90\x93\x8F")) return bn::music_items::ty_trup;      // ТЫ_ТРУП
    if (name_equals(music_name, "\x93\x86\x80\x91")) return bn::music_items::uzhas;          // УЖАС
    if (name_equals(music_name, "\x82\x87\x81\x93\x97\x8A\x80")) return bn::music_items::vzbuchka;     // ВЗБУЧКА
    BN_ERROR("get_xm_ref: unknown music name [", cp866_to_utf8(music_name), "]");
}

const pcm_ref_t get_pcm_ref(const char* sound_name) {
    #include "sound_ref.cpp.inc"
}

void S_startmusic(void) {
    if (!current_music_ref.has_value()) {
        BN_ERROR("S_startmusic: no music loaded");
        return;
    }
    bn::music::play(current_music_ref.value(), bn::fixed(0.3));
}

// остановить музыку
void S_stopmusic(void) {
    bn::music::stop();
}



// Симуляция оригинального драйвера: по каналу храним sound_handle, слот свободен если !handle.active().
static bn::optional<bn::sound_handle> s_channel_handles[BN_CFG_AUDIO_MAX_SOUND_CHANNELS];

void S_sound_tick(void) {
    for (int i = 0; i < BN_CFG_AUDIO_MAX_SOUND_CHANNELS; ++i) {
        if (s_channel_handles[i].has_value() && !s_channel_handles[i]->active())
            s_channel_handles[i].reset();
    }
}

void S_play(const bn::sound_item& s, short c, unsigned r, short v) {
    if (snd_type == -1) return;
    for (int i = 0; i < BN_CFG_AUDIO_MAX_SOUND_CHANNELS; ++i) {
        if (!s_channel_handles[i].has_value() || !s_channel_handles[i]->active()) {
            s_channel_handles[i] = bn::sound::play(s, bn::fixed(v).division(256), bn::fixed(r).division(1024), bn::fixed(0));
            return;
        }
    }
    // нет свободного канала — пропускаем звук
}

void S_stop(short c) {
    if (snd_type == -1) return;
    bn::sound::stop_all();
}