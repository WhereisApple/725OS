#include <efi.h>
#include <efilib.h>
#include "framebuffer.h"
#include "assets.h"

typedef EFI_GRAPHICS_OUTPUT_BLT_PIXEL Pixel;

static EFI_GRAPHICS_OUTPUT_PROTOCOL *gop;
static Pixel *pixels;
static UINTN screen_width, screen_height, tile_size, frame_number;

static void put_pixel(INTN x, INTN y, UINT8 r, UINT8 g, UINT8 b)
{
    Pixel *p;
    if (!pixels || x < 0 || y < 0 || x >= (INTN)screen_width || y >= (INTN)screen_height) return;
    p = &pixels[(UINTN)y * screen_width + (UINTN)x];
    p->Red = r; p->Green = g; p->Blue = b; p->Reserved = 0;
}

static void fill_rect(INTN x, INTN y, UINTN w, UINTN h, UINT8 r, UINT8 g, UINT8 b)
{
    UINTN ix, iy;
    for (iy = 0; iy < h; ++iy)
        for (ix = 0; ix < w; ++ix)
            put_pixel(x + ix, y + iy, r, g, b);
}

static void draw_sheet(const SpriteSheet *sheet, UINTN frame, INTN dx, INTN dy,
                       UINTN dw, UINTN dh, MoveDirection rotation)
{
    UINTN sx0, sy0, x, y;
    if (!sheet || !sheet->pixels || !sheet->columns) return;
    frame %= sheet->columns * sheet->rows;
    sx0 = (frame % sheet->columns) * sheet->frame_width;
    sy0 = (frame / sheet->columns) * sheet->frame_height;
    for (y = 0; y < dh; ++y) {
        for (x = 0; x < dw; ++x) {
            UINTN u = x * sheet->frame_width / dw;
            UINTN v = y * sheet->frame_height / dh;
            UINTN sx = u, sy = v;
            const UINT8 *src;
            UINT8 alpha, r, g, b;
            Pixel *dst;
            if (rotation == MOVE_LEFT) { sx = sheet->frame_width - 1 - u; sy = sheet->frame_height - 1 - v; }
            else if (rotation == MOVE_UP) { sx = sheet->frame_width - 1 - v; sy = u; }
            else if (rotation == MOVE_DOWN) { sx = v; sy = sheet->frame_height - 1 - u; }
            if (sx >= sheet->frame_width || sy >= sheet->frame_height) continue;
            src = sheet->pixels + ((sy0 + sy) * sheet->width + sx0 + sx) * 4;
            alpha = src[3];
            if (!alpha) continue;
            r = src[0]; g = src[1]; b = src[2];
            if (alpha != 255 && dx + (INTN)x >= 0 && dy + (INTN)y >= 0 &&
                dx + (INTN)x < (INTN)screen_width && dy + (INTN)y < (INTN)screen_height) {
                dst = &pixels[(UINTN)(dy + y) * screen_width + (UINTN)(dx + x)];
                r = (UINT8)((r * alpha + dst->Red * (255 - alpha)) / 255);
                g = (UINT8)((g * alpha + dst->Green * (255 - alpha)) / 255);
                b = (UINT8)((b * alpha + dst->Blue * (255 - alpha)) / 255);
            }
            put_pixel(dx + x, dy + y, r, g, b);
        }
    }
}

/* Compact 5x7 font for the game's title, score, and status. */
static const UINT8 font[37][5] = {
    {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
    {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,0x09,0x09,0x09,0x01},
    {0x3e,0x41,0x49,0x49,0x7a},{0x7f,0x08,0x08,0x08,0x7f},{0x00,0x41,0x7f,0x41,0x00},
    {0x20,0x40,0x41,0x3f,0x01},{0x7f,0x08,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
    {0x7f,0x02,0x0c,0x02,0x7f},{0x7f,0x04,0x08,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
    {0x7f,0x09,0x09,0x09,0x06},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,0x09,0x19,0x29,0x46},
    {0x46,0x49,0x49,0x49,0x31},{0x01,0x01,0x7f,0x01,0x01},{0x3f,0x40,0x40,0x40,0x3f},
    {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,0x08,0x14,0x63},
    {0x07,0x08,0x70,0x08,0x07},{0x61,0x51,0x49,0x45,0x43},
    {0x3e,0x51,0x49,0x45,0x3e},{0x00,0x42,0x7f,0x40,0x00},{0x62,0x51,0x49,0x49,0x46},
    {0x22,0x41,0x49,0x49,0x36},{0x18,0x14,0x12,0x7f,0x10},{0x2f,0x49,0x49,0x49,0x31},
    {0x3e,0x49,0x49,0x49,0x32},{0x01,0x71,0x09,0x05,0x03},{0x36,0x49,0x49,0x49,0x36},
    {0x26,0x49,0x49,0x49,0x3e},{0x00,0x00,0x00,0x00,0x00}
};

static INTN glyph_index(char c)
{
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return 26 + c - '0';
    return 36;
}

static void draw_text(INTN x, INTN y, const char *text, UINT8 r, UINT8 g, UINT8 b, UINTN scale)
{
    UINTN i, col, row;
    for (i = 0; text[i]; ++i) {
        INTN index = glyph_index(text[i]);
        for (col = 0; col < 5; ++col)
            for (row = 0; row < 7; ++row)
                if (font[index][col] & (1u << row))
                    fill_rect(x + i * 6 * scale + col * scale, y + row * scale, scale, scale, r, g, b);
    }
}

static void draw_wall(INTN x, INTN y, const GameState *game, UINTN mx, UINTN my)
{
    /* Use the atlas' dark wall tile, then expose the silver-blue wall seams. */
    draw_sheet(&asset_tileset, 0, x, y, tile_size, tile_size, MOVE_NONE);
    if (my == 0 || game->maze[my - 1][mx] != '#') fill_rect(x, y, tile_size, 2, 139, 155, 180);
    if (my + 1 == GAME_HEIGHT || game->maze[my + 1][mx] != '#') fill_rect(x, y + tile_size - 2, tile_size, 2, 139, 155, 180);
    if (mx == 0 || game->maze[my][mx - 1] != '#') fill_rect(x, y, 2, tile_size, 139, 155, 180);
    if (mx + 1 == GAME_WIDTH || game->maze[my][mx + 1] != '#') fill_rect(x + tile_size - 2, y, 2, tile_size, 139, 155, 180);
}

BOOLEAN framebuffer_init(void)
{
    EFI_GUID guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    EFI_STATUS status;
    UINTN bytes;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &guid, NULL, (void **)&gop);
    if (EFI_ERROR(status) || !gop || !gop->Mode || !gop->Mode->Info) return FALSE;
    screen_width = gop->Mode->Info->HorizontalResolution;
    screen_height = gop->Mode->Info->VerticalResolution;
    tile_size = (screen_width >= 1000 && screen_height >= 700) ? 32 : 24;
    bytes = screen_width * screen_height * sizeof(Pixel);
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, bytes, (void **)&pixels);
    if (EFI_ERROR(status) || !pixels) return FALSE;
    return TRUE;
}

void framebuffer_draw_name_screen(const CHAR16 *name, const PersistentScore *record)
{
    UINTN i;
    INTN cx = screen_width / 2;
    INTN cy = screen_height / 2;
    char text[16];
    fill_rect(0, 0, screen_width, screen_height, 7, 8, 18);
    draw_text(cx - 72, cy - 100, "725OS PACMAN", 255, 220, 80, 2);
    draw_text(cx - 75, cy - 45, "HOME START GAME", 192, 203, 220, 1);
    fill_rect(cx - 120, cy - 20, 240, 36, 25, 24, 42);
    fill_rect(cx - 120, cy - 20, 240, 2, 120, 190, 255);
    fill_rect(cx - 120, cy + 14, 240, 2, 120, 190, 255);
    for (i = 0; i < 15 && name[i]; ++i) text[i] = name[i] < 128 ? (char)name[i] : '?';
    text[i] = 0;
    if (!i) draw_text(cx - 48, cy - 7, "PLAYER NAME", 100, 100, 120, 1);
    else draw_text(cx - (INTN)i * 3, cy - 7, text, 255, 255, 255, 1);
    draw_text(cx - 95, cy + 48, "EDIT NAME THEN ENTER TO START", 192, 203, 220, 1);
    draw_text(cx - 50, cy + 78, "BEST SCORE", 192, 203, 220, 1);
    {
        char digits[12];
        UINT32 value = record ? record->high_score : 0;
        UINTN n = 0, j;
        do { digits[n++] = '0' + value % 10; value /= 10; } while (value && n < 10);
        for (j = 0; j < n / 2; ++j) { char c = digits[j]; digits[j] = digits[n - 1 - j]; digits[n - 1 - j] = c; }
        digits[n] = 0;
        draw_text(cx + 58, cy + 78, digits, 255, 255, 255, 1);
    }
    if (record && record->high_score_name[0]) {
        for (i = 0; i < 15 && record->high_score_name[i]; ++i) text[i] = record->high_score_name[i] < 128 ? (char)record->high_score_name[i] : '?';
        text[i] = 0;
        draw_text(cx - (INTN)i * 3, cy + 105, text, 255, 220, 80, 1);
    }
    uefi_call_wrapper(gop->Blt, 10, gop, pixels, EfiBltBufferToVideo,
                      0, 0, 0, 0, screen_width, screen_height, screen_width * sizeof(Pixel));
}

void framebuffer_draw_game(const GameState *game, const CHAR16 *player_name,
                           const PersistentScore *record)
{
    UINTN x, y;
    UINTN board_width = GAME_WIDTH * tile_size;
    UINTN board_height = GAME_HEIGHT * tile_size;
    INTN left = (screen_width - board_width) / 2;
    INTN top = (screen_height - board_height) / 2;
    UINTN frame = (frame_number++ / 3) % 8;
    EFI_STATUS status;

    if (!pixels || !gop) return;
    fill_rect(0, 0, screen_width, screen_height, 7, 8, 18);
    draw_text((screen_width - 12 * 6 * 2) / 2, top - 52, "725OS PACMAN", 255, 220, 80, 2);
    draw_text(left, top - 32, "SCORE", 192, 203, 220, 1);
    if (game->frightened_ms) draw_text(left + 360, top - 32, "POWER", 80, 220, 255, 1);
    draw_text(left + 140, top - 32, "BEST", 192, 203, 220, 1);
    draw_text(left + 270, top - 20, "GHOSTS", 192, 203, 220, 1);
    {
        char text_name[16];
        UINTN n;
        for (n = 0; n < 15 && player_name[n]; ++n) text_name[n] = player_name[n] < 128 ? (char)player_name[n] : '?';
        text_name[n] = 0;
        draw_text(left, top - 20, text_name, 255, 220, 80, 1);
    }
    {
        char score[12];
        UINTN value = game->score, n = 0, i;
        do { score[n++] = '0' + value % 10; value /= 10; } while (value && n < 10);
        for (i = 0; i < n / 2; ++i) { CHAR8 c = score[i]; score[i] = score[n - 1 - i]; score[n - 1 - i] = c; }
        score[n] = 0;
        draw_text(left + 42, top - 32, score, 255, 255, 255, 1);
    }
    {
        char best[12];
        UINT32 value = record ? record->high_score : 0;
        UINTN n = 0, i;
        do { best[n++] = '0' + value % 10; value /= 10; } while (value && n < 10);
        for (i = 0; i < n / 2; ++i) { char c = best[i]; best[i] = best[n - 1 - i]; best[n - 1 - i] = c; }
        best[n] = 0;
        draw_text(left + 170, top - 32, best, 255, 255, 255, 1);
        { char ghosts[2] = { (char)('0' + game->ghost_count), 0 };
          draw_text(left + 315, top - 20, ghosts, 255, 255, 255, 1); }
    }

    for (y = 0; y < GAME_HEIGHT; ++y) {
        for (x = 0; x < GAME_WIDTH; ++x) {
            INTN px = left + x * tile_size, py = top + y * tile_size;
            CHAR8 cell = game->maze[y][x];
            fill_rect(px, py, tile_size, tile_size, 10, 9, 23);
            if (cell == '#') draw_wall(px, py, game, x, y);
            else if (cell == '.') draw_sheet(&asset_coin_transparent, frame, px + (tile_size - 16) / 2, py + (tile_size - 16) / 2, 16, 16, MOVE_NONE);
            else if (cell == 'o') draw_sheet(&asset_big_coin_transparent, frame, px + (tile_size - 16) / 2, py + (tile_size - 16) / 2, 16, 16, MOVE_NONE);
        }
    }
    {
        const SpriteSheet *ghost_art[MAX_GHOSTS] = {
            &asset_ghost_red, &asset_ghost_blue, &asset_ghost_green,
            &asset_ghost_orange, &asset_ghost_yellow
        };
        for (x = 0; x < game->ghost_count; ++x) {
            const SpriteSheet *art = game->frightened_ms ? &asset_ghost_blue : ghost_art[x];
            draw_sheet(art, frame, left + game->ghost_x[x] * tile_size,
                       top + game->ghost_y[x] * tile_size, tile_size, tile_size, MOVE_NONE);
        }
    }
    draw_sheet(&asset_pacman, frame, left + game->player_x * tile_size, top + game->player_y * tile_size,
               tile_size, tile_size, game->facing);

    {
        INTN button_y = top + board_height + 8;
        INTN restart_x = left + board_width - 224;
        INTN home_x = left + board_width - 112;
        fill_rect(restart_x, button_y, 104, 22, 40, 65, 100);
        fill_rect(restart_x, button_y, 104, 2, 120, 190, 255);
        fill_rect(restart_x, button_y + 20, 104, 2, 120, 190, 255);
        draw_text(restart_x + 8, button_y + 7, "RESTART R", 255, 255, 255, 1);
        fill_rect(home_x, button_y, 104, 22, 40, 65, 100);
        fill_rect(home_x, button_y, 104, 2, 120, 190, 255);
        fill_rect(home_x, button_y + 20, 104, 2, 120, 190, 255);
        draw_text(home_x + 8, button_y + 7, "HOME ESC", 255, 255, 255, 1);
    }
    if (game->status != GAME_PLAYING) {
        fill_rect(left + board_width / 4, top + board_height / 2 - 28, board_width / 2, 58, 7, 8, 18);
        draw_text(left + board_width / 4 + 28, top + board_height / 2 - 19,
                  "GAME OVER", 255, 100, 100, 2);
        draw_text(left + board_width / 4 + 20, top + board_height / 2 + 1,
                  "PRESS R TO RESTART", 255, 255, 255, 1);
    }
    status = uefi_call_wrapper(gop->Blt, 10, gop, pixels, EfiBltBufferToVideo,
                               0, 0, 0, 0, screen_width, screen_height, screen_width * sizeof(Pixel));
    (void)status;
}

void framebuffer_draw_cursor(INTN x, INTN y)
{
    fill_rect(x - 5, y - 1, 11, 3, 255, 255, 255);
    fill_rect(x - 1, y - 5, 3, 11, 255, 255, 255);
    uefi_call_wrapper(gop->Blt, 10, gop, pixels, EfiBltBufferToVideo,
                      0, 0, 0, 0, screen_width, screen_height, screen_width * sizeof(Pixel));
}

BOOLEAN framebuffer_restart_hit(INTN x, INTN y)
{
    UINTN board_width = GAME_WIDTH * tile_size;
    UINTN board_height = GAME_HEIGHT * tile_size;
    INTN left = (screen_width - board_width) / 2;
    INTN top = (screen_height - board_height) / 2;
    INTN button_x = left + board_width - 224;
    INTN button_y = top + board_height + 8;
    return x >= button_x && x < button_x + 104 && y >= button_y && y < button_y + 22;
}

BOOLEAN framebuffer_home_hit(INTN x, INTN y)
{
    UINTN board_width = GAME_WIDTH * tile_size;
    UINTN board_height = GAME_HEIGHT * tile_size;
    INTN left = (screen_width - board_width) / 2;
    INTN top = (screen_height - board_height) / 2;
    INTN button_x = left + board_width - 112;
    INTN button_y = top + board_height + 8;
    return x >= button_x && x < button_x + 104 && y >= button_y && y < button_y + 22;
}

void framebuffer_shutdown(void)
{
    if (pixels) uefi_call_wrapper(BS->FreePool, 1, pixels);
    pixels = NULL;
}
