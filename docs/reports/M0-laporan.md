# Laporan Praktikum M0 – Baseline Requirements, Governance, dan Lingkungan Pengembangan

## 1. Sampul
- Judul praktikum: Praktikum M0 – Baseline Requirements, Governance, dan Lingkungan Pengembangan Reproducible MCSOS 260502
- Nama mahasiswa: 
- NIM: 
- Kelas: 
- Dosen: Muhaemin Sidiq, S.Pd., M.Pd.
- Program Studi: Pendidikan Teknologi Informasi, Institut Pendidikan Indonesia
- Tanggal: 06/05/2026

## 2. Tujuan
Menyiapkan lingkungan pengembangan yang reproducible dan dokumen governance untuk proyek MCSOS.

## 3. Dasar teori ringkas
Menjelaskan host vs target, WSL 2, cross-compilation, ELF object, QEMU, OVMF, Git, reproducibility, dan evidence-first engineering.

## 4. Lingkungan
| Komponen | Versi / output |
|---|---|
| Windows | [Cek di Settings About] |
| WSL distro | Ubuntu 24.04 |
| Git | `git --version` |
| Clang | `clang --version` |
| QEMU | `qemu-system-x86_64 --version` |

## 5. Desain baseline
Struktur repository dan dokumen baseline (Requirements, ADR, Threat Model) telah disusun sesuai standar.

## 6. Langkah kerja
Menjalankan setup toolchain, membuat struktur folder, dan menyusun dokumen teknis.

## 7. Hasil uji
| Pengujian | Command | Hasil | Pass/Fail |
|---|---|---|---|
| WSL version | `wsl --list --verbose` | | |
| Tool check | `bash tools/check_env.sh` | | |
| Metadata | `cat build/meta/toolchain-versions.txt` | | |
| Smoke object | `make smoke` | | |
| ELF header | `readelf -h build/smoke/freestanding.o` | | |
| Git status | `git status` | | |

## 8. Analisis
Menjelaskan kendala, error (seperti path OVMF), penyebab, dan perbaikannya.

## 9. Keamanan dan reliability
Menjelaskan risiko supply-chain dan mitigasi yang diterapkan di Threat Model.

## 10. Failure modes dan rollback
| Failure mode | Gejala | Diagnosis | Rollback/perbaikan |
|---|---|---|---|
| WSL bukan versi 2 | Speed lambat | `wsl -l -v` | `wsl --set-version` |
| Tool tidak ditemukan | Command fail | Check `$PATH` | `apt install` |

## 11. Kesimpulan
M0 siap uji lingkungan, belum siap boot. Syarat masuk M1 telah terpenuhi.

## 12. Lampiran
- Output `tools/check_env.sh`
- Isi `build/meta/toolchain-versions.txt`
- Output `readelf -h`
- Screenshot relevan
- Commit hash
