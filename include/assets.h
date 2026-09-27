#ifndef PACMAN_ASSETS_H
#define PACMAN_ASSETS_H

#include <efi.h>

typedef struct {
    const UINT8 *pixels;
    UINTN width, height;
    UINTN frame_width, frame_height;
    UINTN columns, rows;
} SpriteSheet;

extern const SpriteSheet asset_pacman;
extern const SpriteSheet asset_ghost_red;
extern const SpriteSheet asset_ghost_blue;
extern const SpriteSheet asset_ghost_green;
extern const SpriteSheet asset_ghost_orange;
extern const SpriteSheet asset_ghost_yellow;
extern const SpriteSheet asset_coin;
extern const SpriteSheet asset_coin_transparent;
extern const SpriteSheet asset_big_coin;
extern const SpriteSheet asset_big_coin_transparent;
extern const SpriteSheet asset_tileset;

#endif
