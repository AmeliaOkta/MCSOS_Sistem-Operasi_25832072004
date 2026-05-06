#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
META_DIR="$ROOT_DIR/build/meta"
mkdir -p "$META_DIR"

fail=0

say() { printf '[M0] %s\n' "$*"; }
check_tool() {
    local tool="$1"
    if command -v "$tool" >/dev/null 2>&1; then
        printf '[OK]   %-24s %s\n' "$tool" "$(command -v "$tool")"
    else
        printf '[FAIL] %-24s not found\n' "$tool"
        fail=1
    fi
}

say "Repository root: $ROOT_DIR"
case "$ROOT_DIR" in
    /mnt/c/*|/mnt/d/*|/mnt/e/*)
        printf '[WARN] Repository appears to be on a Windows-mounted\nfilesystem. Move it to ~/src/mcsos for this practicum.\n'
        ;;
    *)
        printf '[OK] Repository is not under /mnt/<drive>.\n'
        ;;
esac

say "Checking required tools"
for tool in git make clang ld.lld llvm-readelf llvm-objdump readelf objdump nasm qemu-system-x86_64 gdb python3 shellcheck cppcheck; do
    check_tool "$tool"
done

say "Writing toolchain metadata"
{
    echo "date_utc=$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
    echo "root_dir=$ROOT_DIR"
    echo "uname=$(uname -a)"
    echo "wsl_distro=${WSL_DISTRO_NAME:-unknown}"
    echo "shell=$SHELL"
    echo ""
    echo "## Tool versions"
    git --version || true
    make --version | head -n 1 || true
    clang --version | head -n 1 || true
    ld.lld --version | head -n 1 || true
} > "$META_DIR/toolchain-versions.txt"

say "Metadata written to build/meta/toolchain-versions.txt"

if [ "$fail" -ne 0 ]; then
    say "Environment check failed. Install missing tools before continuing."
    exit 1
fi

say "Environment check completed. This means the M0 environment is checkable, not that the OS can boot."
