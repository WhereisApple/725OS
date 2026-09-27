#ifndef PACMAN_SCORE_STORE_H
#define PACMAN_SCORE_STORE_H

#include <efi.h>

#define PACMAN_NAME_LENGTH 16

typedef struct {
    UINT32 version;
    UINT32 high_score;
    CHAR16 high_score_name[PACMAN_NAME_LENGTH];
    CHAR16 last_player_name[PACMAN_NAME_LENGTH];
} PersistentScore;

void score_store_load(PersistentScore *record);
EFI_STATUS score_store_save(const PersistentScore *record);
void score_store_set_name(PersistentScore *record, const CHAR16 *name);

#endif
