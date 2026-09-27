#ifndef PACMAN_FRAMEBUFFER_H
#define PACMAN_FRAMEBUFFER_H

#include <efi.h>
#include "game.h"
#include "../storage/score_store.h"

BOOLEAN framebuffer_init(void);
void framebuffer_draw_name_screen(const CHAR16 *name, const PersistentScore *record);
void framebuffer_draw_game(const GameState *game, const CHAR16 *player_name,
                           const PersistentScore *record);
void framebuffer_draw_cursor(INTN x, INTN y);
BOOLEAN framebuffer_restart_hit(INTN x, INTN y);
BOOLEAN framebuffer_home_hit(INTN x, INTN y);
void framebuffer_shutdown(void);

#endif
