# MCSOS 260502

MCSOS 260502 adalah proyek sistem operasi pendidikan bertahap untuk target x86_64 dengan host pengembangan Windows 11 x64 melalui WSL 2.

Status saat ini: M0 - baseline requirements, governance, dan lingkungan pengembangan reproducible.

Target awal:
- Arsitektur: x86_64
- Emulator: QEMU system x86_64
- Firmware emulator: OVMF / UEFI
- Bahasa kernel awal: freestanding C17 dan assembly x86_64 minimal
- Kernel model awal: monolithic educational kernel dengan boundary modular internal

Perintah awal:
```bash
make meta
make check
make smoke
