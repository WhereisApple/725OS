#ifndef PACMAN_MOUSE_H
#define PACMAN_MOUSE_H

#include <efi.h>

BOOLEAN mouse_init(INTN start_x, INTN start_y);
EFI_EVENT mouse_wait_event(void);
void mouse_get_position(INTN *x, INTN *y);
BOOLEAN mouse_poll_click(void);

#endif
