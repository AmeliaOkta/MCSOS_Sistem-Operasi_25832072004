# Readiness Review M1 - Toolchain Reproducible

## Identitas

- Nama mahasiswa/kelompok: 
- NIM anggota: 
- Kelas: 
- Dosen: Muhaemin Sidiq, S.Pd., M.Pd.
- Program Studi: Pendidikan Teknologi Informasi, Institut Pendidikan Indonesia
- Tanggal: 
- Commit hash: 

## Ringkasan hasil

Tuliskan ringkasan singkat hasil M1. Gunakan istilah terukur: 'siap untuk M2' hanya bila semua acceptance criteria M1 terpenuhi.

## Evidence checklist

| Evidence | Path | Status | Catatan |
|---|---|---|---|
| Toolchain versions | `build/meta/toolchain-versions.txt` | | |
| Host readiness | `build/meta/host-readiness.txt` | | |
| QEMU capabilities | `build/meta/qemu-capabilities.txt` | | |
| Freestanding object | `build/proof/freestanding_probe.o` | | |
| Freestanding ELF | `build/proof/freestanding_probe.elf` | | |
| ELF header | `build/proof/readelf-header.txt` | | |
| ELF sections | `build/proof/readelf-sections.txt` | | |
| Disassembly | `build/proof/objdump-disassembly.txt` | | |
| Undefined symbol report | `build/proof/nm-undefined.txt` | | |
| Reproducibility hash | `build/repro/sha256-run1.txt`, `build/repro/sha256-run2.txt` | | |

## Acceptance criteria M1

| Kriteria | Lulus/Gagal | Bukti |
|---|---|---|
| Repository berada di filesystem Linux WSL | | |
| Semua tool wajib tersedia | | |
| `make meta` berhasil | | |
| `make check` berhasil | | |
| `make proof` berhasil | | |
| `make qemu-probe` berhasil | | |
| `make repro` berhasil | | |
| `make test` berhasil dari clean checkout | | |
| `nm-undefined.txt` kosong | | |
| Hasil `readelf` menunjukkan ELF64 x86_64 | | |

## Known limitations

Tuliskan keterbatasan yang masih ada. Contoh: belum ada cross GCC 'x86_64-elf-gcc', belum ada CI, belum ada hardware test, belum ada boot image.

## Risiko dan mitigasi

Tuliskan minimal tiga risiko teknis M1 dan mitigasinya.

## Readiness decision

Pilih salah satu:

- [ ] Belum siap lanjut M2.
- [ ] Siap lanjut M2 dengan catatan.
- [ ] Siap lanjut M2.

Alasan keputusan:

