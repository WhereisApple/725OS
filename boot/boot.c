#include <efi.h>
#include <efilib.h>
#include "game.h"
#include "../drivers/keyboard/keyboard.h"
#include "../drivers/keyboard/mouse.h"
#include "../drivers/display/framebuffer.h"
#include "../drivers/storage/score_store.h"
static void copy_name(CHAR16 *destination, const CHAR16 *source)
{
    UINTN i;
    for (i = 0; i + 1 < PACMAN_NAME_LENGTH && source[i]; ++i) destination[i] = source[i];
    destination[i] = 0;
    while (++i < PACMAN_NAME_LENGTH) destination[i] = 0;
}
static void save_record_if_needed(PersistentScore *record, const GameState *game,
                                  const CHAR16 *player_name, BOOLEAN force)
{
    if (game->score <= record->high_score) return;
    if (!force && game->score - record->high_score < 100) return;
    record->high_score = (UINT32)game->score;
    copy_name(record->high_score_name, player_name);
    score_store_save(record);
}
static BOOLEAN enter_player_name(CHAR16 *name, PersistentScore *record)
{
    EFI_EVENT event = ST->ConIn->WaitForKey;
    UINTN event_index;
    BOOLEAN enter, backspace;
    CHAR16 character;
    for (;;) {
        framebuffer_draw_name_screen(name, record);
        if (EFI_ERROR(uefi_call_wrapper(BS->WaitForEvent, 3, 1, &event, &event_index))) return FALSE;
        if (!keyboard_read_name_key(&character, &enter, &backspace)) continue;
        if (enter && name[0]) return TRUE;
        if (backspace) {
            UINTN length = 0;
            while (name[length]) ++length;
            if (length) name[length - 1] = 0;
        } else if (character >= 32 && character <= 126) {
            UINTN length = 0;
            while (length + 1 < PACMAN_NAME_LENGTH && name[length]) ++length;
            if (length + 1 < PACMAN_NAME_LENGTH) {
                name[length] = character;
                name[length + 1] = 0;
            }
        }
    }
}
EFI_STATUS efi_main(EFI_HANDLE image, EFI_SYSTEM_TABLE *system_table)
{
    PersistentScore record;
    CHAR16 player_name[PACMAN_NAME_LENGTH];
    EFI_EVENT timer_event, events[3];
    UINTN event_count, event_index;
    EFI_STATUS status;
    BOOLEAN mouse_available, return_home = FALSE;
    INTN cursor_x = 0, cursor_y = 0;
    InitializeLib(image, system_table);
    if (!framebuffer_init()) {
        Print(L"Unable to initialize the UEFI graphics framebuffer.\r\n");
        keyboard_wait_for_key();
        return EFI_UNSUPPORTED;
    }
    score_store_load(&record);
    copy_name(player_name, record.last_player_name);
    mouse_available = mouse_init(-1, -1);
    if (mouse_available) mouse_get_position(&cursor_x, &cursor_y);

    status = uefi_call_wrapper(BS->CreateEvent, 5, EVT_TIMER, TPL_APPLICATION,
                               NULL, NULL, &timer_event);
    if (EFI_ERROR(status)) { framebuffer_shutdown(); return status; }
    for (;;) {
        GameState game;
        BOOLEAN restart_session = FALSE;
        if (!enter_player_name(player_name, &record)) break;
        score_store_set_name(&record, player_name);
        score_store_save(&record);
        status = uefi_call_wrapper(BS->SetTimer, 3, timer_event, TimerPeriodic, 500000);
        if (EFI_ERROR(status)) break;
        events[0] = ST->ConIn->WaitForKey;
        events[1] = timer_event;
        event_count = 2;
        if (mouse_available) events[event_count++] = mouse_wait_event();
        game_init(&game);
        return_home = FALSE;
        while (!return_home && !restart_session) {
            while (game.status == GAME_PLAYING && !return_home && !restart_session) {
                MoveDirection direction = MOVE_NONE;
                BOOLEAN keyboard_restart = FALSE;
                status = uefi_call_wrapper(BS->WaitForEvent, 3, event_count, events, &event_index);
                if (EFI_ERROR(status)) { return_home = TRUE; break; }
                if (event_index == 0) {
                    if (keyboard_read(&direction, &keyboard_restart)) {
                        save_record_if_needed(&record, &game, player_name, TRUE);
                        return_home = TRUE;
                    } else if (keyboard_restart) {
                        save_record_if_needed(&record, &game, player_name, TRUE);
                        game_init(&game);
                    } else {
                        game_update(&game, direction, 0);
                    }
                } else if (event_index == 1) {
                    game_update(&game, MOVE_NONE, 50);
                } else {
                    BOOLEAN clicked = mouse_poll_click();
                    mouse_get_position(&cursor_x, &cursor_y);
                    if (clicked && framebuffer_restart_hit(cursor_x, cursor_y)) {
                        save_record_if_needed(&record, &game, player_name, TRUE);
                        game_init(&game);
                    } else if (clicked && framebuffer_home_hit(cursor_x, cursor_y)) {
                        save_record_if_needed(&record, &game, player_name, TRUE);
                        return_home = TRUE;
                    }
                }
                save_record_if_needed(&record, &game, player_name, FALSE);
                framebuffer_draw_game(&game, player_name, &record);
                if (mouse_available) framebuffer_draw_cursor(cursor_x, cursor_y);
            }
            if (game.status != GAME_LOST || return_home || restart_session) break;
            save_record_if_needed(&record, &game, player_name, TRUE);
            uefi_call_wrapper(BS->SetTimer, 3, timer_event, TimerCancel, 0);
            framebuffer_draw_game(&game, player_name, &record);
            if (mouse_available) framebuffer_draw_cursor(cursor_x, cursor_y);
            while (!return_home && !restart_session) {
                BOOLEAN keyboard_restart = FALSE;
                event_count = mouse_available ? 2 : 1;
                events[0] = ST->ConIn->WaitForKey;
                if (mouse_available) events[1] = mouse_wait_event();
                status = uefi_call_wrapper(BS->WaitForEvent, 3, event_count, events, &event_index);
                if (EFI_ERROR(status)) { return_home = TRUE; break; }
                if (event_index == 0) {
                    MoveDirection ignored;
                    if (keyboard_read(&ignored, &keyboard_restart)) return_home = TRUE;
                    else if (keyboard_restart) restart_session = TRUE;
                } else {
                    BOOLEAN clicked = mouse_poll_click();
                    mouse_get_position(&cursor_x, &cursor_y);
                    if (clicked && framebuffer_restart_hit(cursor_x, cursor_y)) restart_session = TRUE;
                    else if (clicked && framebuffer_home_hit(cursor_x, cursor_y)) return_home = TRUE;
                }
                if (!return_home && !restart_session) {
                    framebuffer_draw_game(&game, player_name, &record);
                    if (mouse_available) framebuffer_draw_cursor(cursor_x, cursor_y);
                }
            }
            if (restart_session) {
                game_init(&game);
                events[0] = ST->ConIn->WaitForKey;
                events[1] = timer_event;
                event_count = 2;
                if (mouse_available) events[event_count++] = mouse_wait_event();
                status = uefi_call_wrapper(BS->SetTimer, 3, timer_event, TimerPeriodic, 500000);
                if (EFI_ERROR(status)) return_home = TRUE;
                else restart_session = FALSE;
            }
        }
        uefi_call_wrapper(BS->SetTimer, 3, timer_event, TimerCancel, 0);
        if (!return_home) break;
    }
    uefi_call_wrapper(BS->SetTimer, 3, timer_event, TimerCancel, 0);
    uefi_call_wrapper(BS->CloseEvent, 1, timer_event);
    framebuffer_shutdown();
    uefi_call_wrapper(BS->Exit, 4, image, EFI_SUCCESS, 0, NULL);
    while (1) uefi_call_wrapper(BS->Stall, 1, 1000000);
    return EFI_SUCCESS;
}
