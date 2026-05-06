# MCSOS Toolchain and Environment Invariants

## M1 invariants

1. Repository MCSOS berada di filesystem Linux WSL, bukan di `/mnt/c` atau mount Windows lain.
2. Semua generated artifact berada di `build/` dan tidak dikomit ke Git.
3. Semua build tool wajib tersedia melalui PATH WSL dan tercatat di `build/meta/toolchain-versions.txt`.
4. Proof object harus bertipe ELF64 x86_64 dan dihasilkan dengan mode freestanding.
5. Proof ELF tidak boleh memiliki undefined symbol.
6. Kompilasi kernel/proof tidak boleh bergantung pada hosted libc, startup object, dynamic linker, exception runtime, atau stack protector runtime host.
7. QEMU x86_64, machine q35, dan OVMF harus terdeteksi sebelum M2 dimulai.
8. Setiap perubahan toolchain atau versi distro harus dicatat dalam readiness review.
