# System Requirements MCSOS 260502 - Baseline M0

## Scope

Dokumen ini menetapkan requirement awal untuk proyek MCSOS 260502. Requirement pada M0 berfokus pada lingkungan, governance, dan evidence. Requirement runtime kernel akan diperinci pada milestone berikutnya.

| ID | Requirement | Rationale | Verification evidence |
|---|---|---|---|
| REQ-M0-001 | Repository MCSOS harus berada di filesystem Linux WSL, bukan `/mnt/c`. | Mengurangi risiko permission, case, dan IO mismatch. | Output `pwd` dan `tools/check_env.sh`. |
| REQ-M0-002 | Semua tool wajib harus terdeteksi oleh script validasi. | Build lanjutan tidak boleh bergantung pada tool manual tak tercatat. | Output `bash tools/check_env.sh`. |
| REQ-M0-003 | Versi toolchain harus dicatat pada `build/meta/toolchain-versions.txt`. | Traceability dan reproducibility. | Isi file metadata. |
| REQ-M0-004 | Repository harus memiliki struktur `docs`, `tools`, `smoke`, dan `build`. | Menyeragamkan artefak praktikum. | Output `tree -a -L 3`. |
| REQ-M0-005 | Smoke test harus menghasilkan object ELF64 x86-64 relocatable. | Validasi awal target toolchain. | Output `readelf -h`. |
| REQ-M0-006 | Proyek harus memiliki assumptions dan non-goals. | Mencegah scope creep dan klaim readiness berlebih. | `docs/requirements/assumptions_and_nongoals.md`. |
| REQ-M0-007 | Proyek harus memiliki ADR awal untuk toolchain dan boot baseline. | Keputusan teknis harus dapat dilacak. | `docs/adr/ADR-0001-toolchain-and-boot-baseline.md`. |
| REQ-M0-008 | Proyek harus memiliki threat model awal. | Security from phase 0. | `docs/security/threat_model.md`. |
| REQ-M0-009 | Proyek harus memiliki risk register. | Risiko teknis dan operasional harus dikelola. | `docs/governance/risk_register.md`. |
| REQ-M0-010 | Proyek harus memiliki verification matrix. | Setiap requirement harus punya bukti validasi. | `docs/testing/verification_matrix.md`. |
| REQ-M0-011 | Semua perubahan M0 harus dikomit ke Git. | Traceability penilaian. | `git log --oneline`. |
| REQ-M0-012 | Laporan M0 harus memuat log, command, screenshot seperlunya, commit hash, dan analisis failure mode. | Evidence-first assessment. | `docs/reports/M0-laporan.md`. |
