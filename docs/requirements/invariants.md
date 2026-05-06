# Invariants MCSOS 260502 - M0

Dokumen ini mencatat kondisi yang harus selalu benar (invariant) dalam desain dan implementasi MCSOS.

## System-Wide Invariants

1. **Long Mode**: Setelah transisi dari bootloader, kernel harus selalu berjalan dalam x86_64 Long Mode (64-bit).
2. **Memory Safety (M0-M1)**: Kernel tidak boleh menulis ke area memori yang ditandai sebagai Reserved oleh tabel memori UEFI.
3. **Stack Integrity**: Setiap fungsi C harus memiliki stack yang valid dan cukup untuk eksekusi sesuai ABI x86-64.
4. **No External Library**: Kernel tidak boleh melakukan link ke library standar host (seperti glibc). Semua fungsi harus bersifat internal (freestanding).

## Project Invariants

1. **Traceability**: Setiap file binary yang dihasilkan harus dapat dilacak kembali ke source code melalui Git commit hash.
2. **Build Success**: `make check` harus selalu lulus sebelum melakukan commit ke branch utama.
3. **Documentation Sync**: Setiap fitur baru harus memiliki dokumentasi yang sesuai di folder `docs/`.
