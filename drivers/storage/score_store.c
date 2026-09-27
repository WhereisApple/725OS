#include <efi.h>
#include <efilib.h>
#include "score_store.h"

#define SCORE_RECORD_VERSION 1
#define SCORE_VARIABLE_ATTRIBUTES (EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS)

static EFI_GUID score_vendor_guid = {
    0x8b9c4f21, 0x59d1, 0x4ee2, {0x9a, 0x34, 0x72, 0x35, 0x4f, 0x53, 0x01, 0x01}
};
static CHAR16 score_variable_name[] = L"725OSPacmanRecord";

void score_store_load(PersistentScore *record)
{
    UINTN size = sizeof(*record);
    UINT32 attributes = 0;
    EFI_STATUS status;
    UINTN i;
    for (i = 0; i < sizeof(*record); ++i) ((UINT8 *)record)[i] = 0;
    status = uefi_call_wrapper(RT->GetVariable, 5, score_variable_name, &score_vendor_guid,
                               &attributes, &size, record);
    if (EFI_ERROR(status) || size != sizeof(*record) || record->version != SCORE_RECORD_VERSION) {
        for (i = 0; i < sizeof(*record); ++i) ((UINT8 *)record)[i] = 0;
        record->version = SCORE_RECORD_VERSION;
    }
}

EFI_STATUS score_store_save(const PersistentScore *record)
{
    return uefi_call_wrapper(RT->SetVariable, 5, score_variable_name, &score_vendor_guid,
                             SCORE_VARIABLE_ATTRIBUTES, sizeof(*record), (void *)record);
}

void score_store_set_name(PersistentScore *record, const CHAR16 *name)
{
    UINTN i;
    for (i = 0; i + 1 < PACMAN_NAME_LENGTH && name[i]; ++i)
        record->last_player_name[i] = name[i];
    record->last_player_name[i] = 0;
    while (++i < PACMAN_NAME_LENGTH) record->last_player_name[i] = 0;
}
