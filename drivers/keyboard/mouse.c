#include <efi.h>
#include <efilib.h>
#include <efipoint.h>
#include "mouse.h"

static EFI_SIMPLE_POINTER_PROTOCOL *pointer;
static INTN cursor_x, cursor_y;
static UINTN max_x, max_y;
static BOOLEAN previous_left;

BOOLEAN mouse_init(INTN start_x, INTN start_y)
{
    EFI_GUID guid = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
    EFI_STATUS status;
    EFI_GRAPHICS_OUTPUT_PROTOCOL *gop = NULL;
    EFI_GUID graphics_guid = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &guid, NULL, (void **)&pointer);
    if (EFI_ERROR(status) || !pointer || !pointer->WaitForInput) return FALSE;
    status = uefi_call_wrapper(BS->LocateProtocol, 3, &graphics_guid, NULL, (void **)&gop);
    if (EFI_ERROR(status) || !gop || !gop->Mode || !gop->Mode->Info) return FALSE;
    max_x = gop->Mode->Info->HorizontalResolution - 1;
    max_y = gop->Mode->Info->VerticalResolution - 1;
    cursor_x = start_x < 0 ? (INTN)(max_x / 2) : start_x;
    cursor_y = start_y < 0 ? (INTN)(max_y / 2) : start_y;
    previous_left = FALSE;
    return TRUE;
}

EFI_EVENT mouse_wait_event(void)
{
    return pointer ? pointer->WaitForInput : NULL;
}

void mouse_get_position(INTN *x, INTN *y)
{
    *x = cursor_x;
    *y = cursor_y;
}

BOOLEAN mouse_poll_click(void)
{
    EFI_SIMPLE_POINTER_STATE state;
    EFI_STATUS status;
    BOOLEAN clicked;
    if (!pointer) return FALSE;
    status = uefi_call_wrapper(pointer->GetState, 2, pointer, &state);
    if (EFI_ERROR(status)) return FALSE;
    cursor_x += state.RelativeMovementX * 2;
    cursor_y += state.RelativeMovementY * 2;
    if (cursor_x < 0) cursor_x = 0;
    if (cursor_y < 0) cursor_y = 0;
    if ((UINTN)cursor_x > max_x) cursor_x = max_x;
    if ((UINTN)cursor_y > max_y) cursor_y = max_y;
    clicked = state.LeftButton && !previous_left;
    previous_left = state.LeftButton;
    return clicked;
}
