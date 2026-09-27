BUILD := build
EFI_CC := x86_64-linux-gnu-gcc
EFI_LD := ld
EFI_OBJCOPY := objcopy
PYTHON := python3
EFI_CFLAGS := -Iinclude -Idrivers/keyboard -Idrivers/display -I/usr/include/efi -I/usr/include/efi/x86_64 -fpic -fshort-wchar -mno-red-zone -fno-stack-protector -fno-strict-aliasing -O2 -DEFI_FUNCTION_WRAPPER -Wall -Wextra
SOURCES := boot/boot.c game/game.c drivers/keyboard/keyboard.c drivers/keyboard/mouse.c drivers/storage/score_store.c drivers/display/framebuffer.c
OBJECTS := $(patsubst %.c,$(BUILD)/%.o,$(SOURCES)) $(BUILD)/assets_table.o $(BUILD)/assets.o
ASSET_PNGS := $(wildcard assets/sprites/*.png)
HEADERS := $(wildcard include/*.h drivers/keyboard/*.h drivers/display/*.h drivers/storage/*.h)

.PHONY: all clean run
all: $(BUILD)/esp/EFI/BOOT/BOOTX64.EFI

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c $(HEADERS)
	mkdir -p $(dir $@)
	$(EFI_CC) $(EFI_CFLAGS) -c $< -o $@

$(BUILD)/assets.stamp: scripts/pack_assets.py $(ASSET_PNGS) | $(BUILD)
	$(PYTHON) scripts/pack_assets.py --assets assets/sprites --binary $(BUILD)/assets.bin --source $(BUILD)/assets_table.c
	touch $@

$(BUILD)/assets_table.o: $(BUILD)/assets.stamp
	$(EFI_CC) $(EFI_CFLAGS) -c $(BUILD)/assets_table.c -o $@

$(BUILD)/assets.o: $(BUILD)/assets.stamp
	$(EFI_OBJCOPY) --input-target=binary --output-target=elf64-x86-64 --binary-architecture=i386:x86-64 $(BUILD)/assets.bin $@

$(BUILD)/boot.so: $(OBJECTS)
	$(EFI_LD) -nostdlib -znocombreloc -T /usr/lib/elf_x86_64_efi.lds -shared -Bsymbolic /usr/lib/crt0-efi-x86_64.o $(OBJECTS) -L/usr/lib -lefi -lgnuefi -o $@

$(BUILD)/BOOTX64.EFI: $(BUILD)/boot.so
	$(EFI_OBJCOPY) -j .text -j .sdata -j .data -j .dynamic -j .dynsym -j .rel -j .rela -j .reloc --target=efi-app-x86_64 $< $@

$(BUILD)/esp/EFI/BOOT/BOOTX64.EFI: $(BUILD)/BOOTX64.EFI
	mkdir -p $(BUILD)/esp/EFI/BOOT
	cp $(BUILD)/BOOTX64.EFI $@

$(BUILD)/OVMF_VARS.fd: | $(BUILD)
	cp /usr/share/OVMF/OVMF_VARS_4M.fd $@

run: all $(BUILD)/OVMF_VARS.fd
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
		-drive if=pflash,format=raw,file=$(BUILD)/OVMF_VARS.fd \
		-drive format=raw,file=fat:rw:$(BUILD)/esp -m 256

clean:
	rm -f $(BUILD)/boot.o $(BUILD)/boot.so $(BUILD)/BOOTX64.EFI $(BUILD)/esp.img $(BUILD)/assets.bin $(BUILD)/assets_table.c $(BUILD)/assets_table.o $(BUILD)/assets.o $(BUILD)/assets.stamp
	rm -rf $(BUILD)/esp $(BUILD)/boot $(BUILD)/game $(BUILD)/drivers $(BUILD)/storage
