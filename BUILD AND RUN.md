## Build and run
On Debian or Ubuntu, I need GNU EFI, Python Pillow, QEMU, and OVMF:
sudo apt install gnu-efi python3-pil qemu-system-x86 ovmf
make
make run
The build puts the UEFI app at `build/BOOTX64.EFI` and copies it to `build/esp/EFI/BOOT/BOOTX64.EFI`. The `build/esp` directory can be copied to a FAT32 EFI System Partition to boot on UEFI hardware. I haven’t tested it on a physical PC yet. Persistent scores on hardware depend on the firmware supporting UEFI variables.
In QEMU, saved names and scores live in `build/OVMF_VARS.fd`. `make clean` leaves that file in place so the record survives rebuilds.
