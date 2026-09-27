#include <efi.h>
#include <efilib.h>
#include "keyboard.h"

BOOLEAN keyboard_read(MoveDirection *direction, BOOLEAN *restart)
{
    EFI_INPUT_KEY key;
    EFI_STATUS status = uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key);
    *direction = MOVE_NONE;
    *restart = FALSE;
    if (status != EFI_SUCCESS) return FALSE;

    if (key.ScanCode == SCAN_ESC) return TRUE;
    if (key.UnicodeChar == L'r' || key.UnicodeChar == L'R') { *restart = TRUE; return FALSE; }
    if (key.ScanCode == SCAN_UP || key.UnicodeChar == L'w' || key.UnicodeChar == L'W') *direction = MOVE_UP;
    if (key.ScanCode == SCAN_DOWN || key.UnicodeChar == L's' || key.UnicodeChar == L'S') *direction = MOVE_DOWN;
    if (key.ScanCode == SCAN_LEFT || key.UnicodeChar == L'a' || key.UnicodeChar == L'A') *direction = MOVE_LEFT;
    if (key.ScanCode == SCAN_RIGHT || key.UnicodeChar == L'd' || key.UnicodeChar == L'D') *direction = MOVE_RIGHT;
    return FALSE;
}

BOOLEAN keyboard_read_name_key(CHAR16 *character, BOOLEAN *enter, BOOLEAN *backspace)
{
    EFI_INPUT_KEY key;
    if (uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key) != EFI_SUCCESS) return FALSE;
    *character = key.UnicodeChar;
    *enter = key.UnicodeChar == L'\r';
    *backspace = key.UnicodeChar == L'\b';
    return TRUE;
}

void keyboard_wait_for_key(void)
{
    EFI_INPUT_KEY key;
    while (uefi_call_wrapper(ST->ConIn->ReadKeyStroke, 2, ST->ConIn, &key) != EFI_SUCCESS)
        uefi_call_wrapper(BS->Stall, 1, 10000);
}
