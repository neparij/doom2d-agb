#ifndef DOOM2D_MEMORY_H
#define DOOM2D_MEMORY_H
// High-level memory allocation

void M_startup(void);
void M_shutdown(void);
//void *M_mapres(int);
//void M_loadblock(word);
//void M_preload(void);
void *M_lock(int);
void M_unlock(void *);

void *M_lock_ewram(int, void* ewram_ptr);
void M_unlock_ewram(void *, void* ewram_ptr);

#endif //DOOM2D_MEMORY_H