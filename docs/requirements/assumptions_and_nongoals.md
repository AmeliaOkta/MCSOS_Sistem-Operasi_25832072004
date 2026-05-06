# Assumptions and Non-Goals MCSOS 260502 - M0

## Assumptions

1. Target arsitektur awal adalah x86_64 long mode.
2. Host pengembangan adalah Windows 11 x64.
3. Build dilakukan di WSL 2 Linux environment.
4. Repository utama berada di filesystem Linux WSL.
5. Emulator utama untuk milestone awal adalah QEMU system x86_64.
6. Firmware emulator untuk jalur boot awal adalah OVMF/UEFI.
7. Bootloader awal yang direkomendasikan untuk milestone boot adalah Limine atau bootloader setara yang memiliki handoff terdokumentasi.
8. Bahasa kernel awal adalah freestanding C17 dengan assembly minimal.
9. Compatibility target awal adalah POSIX-like subset, bukan Linux ABI penuh.
10. Setiap milestone harus menghasilkan bukti: log, command output, image, checksum, map file, disassembly, trace, atau laporan.

## Non-goals M0

1. M0 tidak membuat kernel bootable.
2. M0 tidak mengimplementasikan bootloader.
3. M0 tidak membuat linker script final.
4. M0 tidak mengimplementasikan interrupt, paging, scheduler, syscall, VFS, driver, networking, graphics, atau security enforcement.
5. M0 tidak mengklaim MCSOS siap produksi.
6. M0 tidak mengklaim semua mesin x86_64 akan kompatibel.
7. M0 tidak mengharuskan hardware bring-up.
8. M0 tidak mengharuskan byte-for-byte reproducible build; nondeterminism cukup dicatat.
