# Threat Model Ringkas M1 - Toolchain dan Lingkungan

## Assets

1. Source code MCSOS.
2. Script build dan test.
3. Toolchain compiler, linker, assembler, emulator, debugger.
4. Artefak generated: object, ELF, log, metadata.
5. Bukti praktikum dan laporan.

## Trust assumptions

1. Paket Ubuntu/Debian berasal dari repository resmi atau mirror kampus yang disetujui.
2. Mahasiswa tidak mengubah binary compiler/linker secara manual.
3. Repository berada pada filesystem Linux WSL agar permission dan executable bit stabil.
4. Build M1 belum mengeksekusi kode guest MCSOS; risiko utama adalah salah konfigurasi dan supply-chain.

## Threats

| Threat | Dampak | Mitigasi M1 |
|---|---|---|
| Compiler host salah target | Object tidak cocok untuk kernel | Inspect `readelf` dan target triple |
| Linker memakai libc/startup host | Kernel bergantung pada runtime tidak tersedia | Gunakan `-nostdlib`, cek `nm -u` |
| Repository di `/mnt/c` | permission, symlink, case sensitivity, I/O tidak stabil | Check path pada `check_toolchain.sh` |
| Generated artifact dikomit | repository kotor dan sulit direproduksi | `.gitignore`, `make distclean` |
| OVMF tidak tersedia | M2 gagal boot UEFI | `qemu_probe.sh` |
| Versi tool tidak dicatat | build tidak dapat diaudit | `collect_meta.sh` |
