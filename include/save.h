#ifndef DOOM2D_SAVE_H
#define DOOM2D_SAVE_H

#define SV_SLOTS 7

void SV_init(void);
void SV_commit(void);
void SV_save_slot(int i);
void SV_restore_player(int i);
int SV_slot_used(int i);
unsigned char SV_slot_map(int i);
const char *SV_slot_name(int i);

#endif
