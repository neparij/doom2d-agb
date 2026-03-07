#ifndef DOOM2D_MUSIC_H
#define DOOM2D_MUSIC_H

#include "bn_music_item.h"
#include "bn_optional.h"
#include "bn_sound_items.h"

enum {
    ST_NONE = -1,
    ST_DAC,
    ST_DMA
};

namespace bn {
    class sound_item;
}

struct pcm_ref_t {
    pcm_ref_t()
        : item(nullptr), length(0) {
    }

    pcm_ref_t(const bn::sound_item *item, short length)
        : item(item), length(length) {
    }

    const bn::sound_item *item; // pointer so struct is copy-assignable
    int length; // lump size / 605
};

const pcm_ref_t get_pcm_ref(const char* sound_name);

const bn::music_item& get_xm_ref(const char* music_name);



// проиграть звук s на канале c (1-8), частоте r и громкости v (0-255)
// если свободного канала нет — звук пропускается (как при c==0 в оригинале)
void S_play(const bn::sound_item& s, short c, unsigned r, short v);

// вызывать перед bn::core::update(): освобождает слоты, у которых звук уже не играется
void S_sound_tick(void);

// остановить звук на канале c (1-8)
void S_stop(short c);

// установить частоту r у звука на канале c
// void S_setrate(short c,unsigned r);

// установить громкость v (0-255) у звука на канале c
// void S_setvolume(short c,int v);

// начать музыку
void S_startmusic(void);

// остановить музыку
void S_stopmusic(void);


extern short snd_type;
extern bn::optional<bn::music_item> current_music_ref;
extern short snd_vol,mus_vol;

#endif //DOOM2D_MUSIC_H