#include <efi.h>
#include <efilib.h>
#include "console.h"

void display_game(const GameState *game, CHAR16 *message)
{
    UINTN x, y;
    uefi_call_wrapper(ST->ConOut->ClearScreen, 1, ST->ConOut);
    Print(L"                 725OS  |  PAC-MAN\r\n\r\n");
    for (y = 0; y < GAME_HEIGHT; ++y) {
        Print(L"       ");
        for (x = 0; x < GAME_WIDTH; ++x) {
            CHAR16 c = game->maze[y][x] == '#' ? L'#' : (game->maze[y][x] == '.' ? L'.' : L' ');
            if (x == game->player_x && y == game->player_y) c = L'C';
            if (x == game->ghost_x && y == game->ghost_y) c = L'G';
            Print(L"%c", c);
        }
        Print(L"\r\n");
    }
    Print(L"\r\n       Score: %u     Arrow keys / WASD to move     Esc to quit\r\n", game->score);
    if (message) Print(L"\r\n       %s\r\n", message);
}
