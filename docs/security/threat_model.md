# Threat Model Awal MCSOS 260502 - M0

## Assets

| Asset | Alasan dilindungi |
|---|---|
| Source code repository | Menentukan perilaku kernel dan tools. |
| Toolchain | Compiler/linker yang salah dapat menghasilkan artefak salah. |
| Build scripts | Script dapat menyisipkan flag berbahaya atau target salah. |
| Documentation baseline | Menjadi sumber requirement dan acceptance criteria. |
| Generated artifacts | Image/log/map dapat menjadi bukti penilaian. |
| Signing keys masa depan | Belum dibuat pada M0, tetapi harus direncanakan. |

## Actors

| Actor | Capability |
|---|---|
| Mahasiswa | Mengubah repository dan menjalankan build. |
| Anggota kelompok | Mengubah branch dan dokumen. |
| Dosen/asisten | Melakukan review dan penilaian. |
| Dependency eksternal | Menyediakan paket, source, dan tools. |
| Malicious local process | Dapat memodifikasi file jika permission buruk. |

## Trust boundaries

1. Windows host ↔ WSL Linux environment.
2. Repository source ↔ generated build output.
3. Package manager ↔ toolchain lokal.
4. Script praktikum ↔ shell pengguna.
5. QEMU guest masa depan ↔ host environment.

## Initial threats and mitigations

| Threat | Dampak | Mitigasi M0 |
|---|---|---|
| Repository ditempatkan di `/mnt/c` | Permission/line ending berubah | Check script memberi warning; repository dipindah ke `~/src/mcsos`. |
| Build tidak reproducible | Sulit diaudit | Tool versi dicatat di `build/meta/toolchain-versions.txt`. |
| Compiler host dipakai tanpa target eksplisit | Object salah ABI | Smoke test memakai `--target` dan `readelf`. |
| Script dari internet dieksekusi tanpa review | Supply-chain compromise | Gunakan package manager resmi atau source resmi; catat URL dan checksum untuk source manual. |
| Klaim readiness berlebihan | Penilaian tidak valid | Gunakan readiness label berbasis bukti. |
| Anggota kelompok tidak memahami baseline | Integrasi gagal | Laporan mencantumkan peran dan review lintas anggota. |

## Out of scope M0

1. Enforcement MAC/RBAC/capability.
2. Secure Boot penuh.
3. TPM measured boot.
4. Kernel exploit mitigation.
5. Syscall fuzzing.

Semua item out-of-scope akan masuk milestone berikutnya setelah boot, memory, syscall, dan userspace baseline tersedia.
