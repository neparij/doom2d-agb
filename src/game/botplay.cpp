extern "C" {
#include "glob.h"
#include "view.h"
#include "player.h"
//#include "keyb.h"
}

#include "botplay.h"

BotPlayer *BOT_new() {
    return new BotPlayer;
}

BotPlayer::BotPlayer() {}

BotPlayer::~BotPlayer() {}

int BotPlayer::getkeys(player_t *p) {
    return PLK_J;
}

