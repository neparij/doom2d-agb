#ifndef DOOM2D_BOTPLAY_H
#define DOOM2D_BOTPLAY_H

struct BotPlayer {
    BotPlayer();
    virtual ~BotPlayer();
    virtual int getkeys(player_t *p);
};

BotPlayer *BOT_new();

#endif //DOOM2D_BOTPLAY_H