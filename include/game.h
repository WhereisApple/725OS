#ifndef PACMAN_GAME_H
#define PACMAN_GAME_H

#include <efi.h>

#define GAME_WIDTH 25
#define GAME_HEIGHT 15
#define MAX_GHOSTS 5

typedef enum {
    MOVE_NONE,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT
} MoveDirection;

typedef enum {
    GAME_PLAYING,
    GAME_LOST
} GameStatus;

typedef struct {
    CHAR8 maze[GAME_HEIGHT][GAME_WIDTH + 1];
    UINT16 respawn_ms[GAME_HEIGHT][GAME_WIDTH];
    CHAR8 respawn_kind[GAME_HEIGHT][GAME_WIDTH];
    UINTN player_x, player_y;
    UINTN ghost_x[MAX_GHOSTS], ghost_y[MAX_GHOSTS];
    UINTN ghost_count;
    UINTN score, dots_left;
    UINTN ghost_elapsed_ms, frightened_ms;
    MoveDirection facing;
    GameStatus status;
} GameState;

void game_init(GameState *game);
void game_update(GameState *game, MoveDirection direction, UINTN elapsed_ms);

#endif
