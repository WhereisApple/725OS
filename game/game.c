#include "game.h"

#define GHOST_STEP_MS 400
#define PELLET_RESPAWN_MS 12000
#define POWER_TIME_MS 6000

static const CHAR8 starting_maze[GAME_HEIGHT][GAME_WIDTH + 1] = {
    "#########################",
    "#...........#...........#",
    "#.###.#####.#.#####.###.#",
    "#o.....................o#",
    "#.##.#.###.#.#.###.#.##.#",
    "#.....#.....#.....#.....#",
    "#####.#.###.#.###.#.#####",
    "#.....#.....#.....#.....#",
    "#.##.#.###...###.#.##.#",
    "#.....#.....#.....#.....#",
    "#####.#.###.#.###.#.#####",
    "#.....#.....#.....#.....#",
    "#.##.#.###...###.#.##.#",
    "#...........#...........#",
    "#########################"
};

static const UINT8 ghost_spawn_x[MAX_GHOSTS] = {23, 1, 23, 10, 14};
static const UINT8 ghost_spawn_y[MAX_GHOSTS] = {13, 13, 1, 7, 7};

static BOOLEAN can_enter(const GameState *game, INTN x, INTN y)
{
    return x >= 0 && x < GAME_WIDTH && y >= 0 && y < GAME_HEIGHT && game->maze[y][x] != '#';
}

static UINTN ghosts_for_score(UINTN score)
{
    if (score >= 7000) return 5;
    if (score >= 4500) return 4;
    if (score >= 2500) return 3;
    if (score >= 1000) return 2;
    return 1;
}

/* Breadth-first distances make ghosts route around corners and dead ends. */
static void move_ghosts(GameState *game)
{
    static const INTN step_x[4] = {0, 1, 0, -1};
    static const INTN step_y[4] = {-1, 0, 1, 0};
    INT16 distance[GAME_HEIGHT][GAME_WIDTH];
    UINT8 queue_x[GAME_WIDTH * GAME_HEIGHT];
    UINT8 queue_y[GAME_WIDTH * GAME_HEIGHT];
    UINTN head = 0, tail = 0, x, y, i;

    for (y = 0; y < GAME_HEIGHT; ++y)
        for (x = 0; x < GAME_WIDTH; ++x)
            distance[y][x] = -1;
    distance[game->player_y][game->player_x] = 0;
    queue_x[tail] = game->player_x;
    queue_y[tail++] = game->player_y;
    while (head < tail) {
        INTN cx = queue_x[head], cy = queue_y[head++];
        for (i = 0; i < 4; ++i) {
            INTN nx = cx + step_x[i], ny = cy + step_y[i];
            if (can_enter(game, nx, ny) && distance[ny][nx] < 0) {
                distance[ny][nx] = distance[cy][cx] + 1;
                queue_x[tail] = nx;
                queue_y[tail++] = ny;
            }
        }
    }

    for (i = 0; i < game->ghost_count; ++i) {
        INTN gx = game->ghost_x[i], gy = game->ghost_y[i];
        INT16 best = distance[gy][gx];
        UINTN d, start = (i + game->score / 10) % 4;
        for (d = 0; d < 4; ++d) {
            UINTN dir = (start + d) % 4;
            INTN nx = gx + step_x[dir], ny = gy + step_y[dir];
            if (can_enter(game, nx, ny) && distance[ny][nx] >= 0 && distance[ny][nx] < best) {
                best = distance[ny][nx];
                game->ghost_x[i] = nx;
                game->ghost_y[i] = ny;
            }
        }
    }
}

static BOOLEAN ghost_at(const GameState *game, UINTN x, UINTN y)
{
    UINTN i;
    for (i = 0; i < game->ghost_count; ++i)
        if (game->ghost_x[i] == x && game->ghost_y[i] == y) return TRUE;
    return FALSE;
}

static void respawn_pellets(GameState *game, UINTN elapsed_ms)
{
    UINTN x, y;
    if (!elapsed_ms) return;
    for (y = 0; y < GAME_HEIGHT; ++y) {
        for (x = 0; x < GAME_WIDTH; ++x) {
            UINT16 *timer = &game->respawn_ms[y][x];
            if (!*timer) continue;
            if (*timer > elapsed_ms) {
                *timer -= elapsed_ms;
                continue;
            }
            if ((x == game->player_x && y == game->player_y) || ghost_at(game, x, y)) {
                *timer = 1000;
                continue;
            }
            game->maze[y][x] = game->respawn_kind[y][x];
            *timer = 0;
            ++game->dots_left;
        }
    }
}

void game_init(GameState *game)
{
    UINTN y, x, i;
    for (y = 0; y < GAME_HEIGHT; ++y) {
        for (x = 0; x <= GAME_WIDTH; ++x) game->maze[y][x] = starting_maze[y][x];
        for (x = 0; x < GAME_WIDTH; ++x) {
            game->respawn_ms[y][x] = 0;
            game->respawn_kind[y][x] = 0;
        }
    }
    game->player_x = 1;
    game->player_y = 1;
    game->score = 0;
    game->dots_left = 0;
    game->ghost_count = 1;
    game->ghost_elapsed_ms = 0;
    game->frightened_ms = 0;
    game->facing = MOVE_RIGHT;
    game->status = GAME_PLAYING;
    for (i = 0; i < MAX_GHOSTS; ++i) {
        game->ghost_x[i] = ghost_spawn_x[i];
        game->ghost_y[i] = ghost_spawn_y[i];
    }
    for (y = 0; y < GAME_HEIGHT; ++y)
        for (x = 0; x < GAME_WIDTH; ++x)
            if (game->maze[y][x] == '.' || game->maze[y][x] == 'o') ++game->dots_left;
}

void game_update(GameState *game, MoveDirection direction, UINTN elapsed_ms)
{
    INTN x = game->player_x, y = game->player_y;
    UINTN i;
    if (game->status != GAME_PLAYING) return;

    respawn_pellets(game, elapsed_ms);
    if (game->frightened_ms > elapsed_ms) game->frightened_ms -= elapsed_ms;
    else game->frightened_ms = 0;
    game->ghost_elapsed_ms += elapsed_ms;
    while (game->ghost_elapsed_ms >= (game->frightened_ms ? 700 : GHOST_STEP_MS)) {
        game->ghost_elapsed_ms -= game->frightened_ms ? 700 : GHOST_STEP_MS;
        move_ghosts(game);
    }

    if (direction == MOVE_UP) --y;
    if (direction == MOVE_DOWN) ++y;
    if (direction == MOVE_LEFT) --x;
    if (direction == MOVE_RIGHT) ++x;
    if (direction != MOVE_NONE && can_enter(game, x, y)) {
        game->player_x = x;
        game->player_y = y;
        game->facing = direction;
        if (game->maze[y][x] == '.' || game->maze[y][x] == 'o') {
            CHAR8 kind = game->maze[y][x];
            game->respawn_kind[y][x] = kind;
            game->respawn_ms[y][x] = PELLET_RESPAWN_MS;
            game->score += kind == 'o' ? 50 : 10;
            game->maze[y][x] = ' ';
            --game->dots_left;
            if (kind == 'o') game->frightened_ms = POWER_TIME_MS;
            game->ghost_count = ghosts_for_score(game->score);
        }
    }

    for (i = 0; i < game->ghost_count; ++i) {
        if (game->player_x == game->ghost_x[i] && game->player_y == game->ghost_y[i]) {
            if (game->frightened_ms) {
                game->score += 200;
                game->ghost_x[i] = ghost_spawn_x[i];
                game->ghost_y[i] = ghost_spawn_y[i];
                game->ghost_count = ghosts_for_score(game->score);
            } else {
                game->status = GAME_LOST;
                break;
            }
        }
    }
}
