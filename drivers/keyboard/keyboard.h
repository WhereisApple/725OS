#ifndef PACMAN_KEYBOARD_H
#define PACMAN_KEYBOARD_H

#include <efi.h>
#include "game.h"

BOOLEAN keyboard_read(MoveDirection *direction, BOOLEAN *restart);
BOOLEAN keyboard_read_name_key(CHAR16 *character, BOOLEAN *enter, BOOLEAN *backspace);
void keyboard_wait_for_key(void);

#endif
