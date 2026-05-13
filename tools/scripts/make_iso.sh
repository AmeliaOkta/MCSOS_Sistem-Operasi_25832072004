#!/usr/bin/env bash
set -euo pipefail

KERNEL="build/kernel.elf"
ISO="build/mcsos.iso"
ISO_ROOT="iso_root"
LIMINE_DIR="third_party/limine"

if [ ! -f "$KERNEL" ]; then
  echo "ERROR: $KERNEL tidak ditemukan. Jalankan make build." >&2
  exit 1
fi

if [ ! -d "$LIMINE_DIR" ]; then
  ./tools/scripts/fetch_limine.sh
fi

mkdir -p "$ISO_ROOT/boot/limine" "$ISO_ROOT/EFI/BOOT" build
cp -v "$KERNEL" "$ISO_ROOT/boot/kernel.elf"
cp -v configs/limine/limine.conf "$ISO_ROOT/boot/limine/limine.conf"
cp -v "$LIMINE_DIR/limine-bios.sys" "$ISO_ROOT/boot/limine/"
cp -v "$LIMINE_DIR/limine-bios-cd.bin" "$ISO_ROOT/boot/limine/"
cp -v "$LIMINE_DIR/limine-uefi-cd.bin" "$ISO_ROOT/boot/limine/"
cp -v "$LIMINE_DIR/BOOTX64.EFI" "$ISO_ROOT/EFI/BOOT/BOOTX64.EFI"
if [ -f "$LIMINE_DIR/BOOTIA32.EFI" ]; then
  cp -v "$LIMINE_DIR/BOOTIA32.EFI" "$ISO_ROOT/EFI/BOOT/BOOTIA32.EFI"
fi

xorriso -as mkisofs \
  -R -r -J \
  -b boot/limine/limine-bios-cd.bin \
  -no-emul-boot -boot-load-size 4 -boot-info-table \
  --efi-boot boot/limine/limine-uefi-cd.bin \
  -efi-boot-part --efi-boot-image --protective-msdos-label \
  "$ISO_ROOT" -o "$ISO"

"$LIMINE_DIR/limine" bios-install "$ISO"
sha256sum "$ISO" | tee build/mcsos.iso.sha256

echo "OK: ISO dibuat pada $ISO"
