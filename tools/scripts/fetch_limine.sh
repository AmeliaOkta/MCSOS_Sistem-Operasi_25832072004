#!/usr/bin/env bash
set -euo pipefail

LIMINE_DIR="third_party/limine"
LIMINE_BRANCH="${LIMINE_BRANCH:-v11.x-binary}"
LIMINE_URL="${LIMINE_URL:-https://github.com/Limine-Bootloader/Limine.git}"

mkdir -p third_party build/meta

if [ -d "$LIMINE_DIR/.git" ]; then
  git -C "$LIMINE_DIR" fetch --depth=1 origin "$LIMINE_BRANCH"
  git -C "$LIMINE_DIR" checkout "$LIMINE_BRANCH"
else
  rm -rf "$LIMINE_DIR"
  git clone "$LIMINE_URL" --branch="$LIMINE_BRANCH" --depth=1 "$LIMINE_DIR"
fi

make -C "$LIMINE_DIR"

git -C "$LIMINE_DIR" rev-parse HEAD | tee build/meta/limine-revision.txt
printf 'branch=%s\nurl=%s\n' "$LIMINE_BRANCH" "$LIMINE_URL" | tee -a build/meta/limine-revision.txt

test -f "$LIMINE_DIR/limine-bios.sys"
test -f "$LIMINE_DIR/limine-bios-cd.bin"
test -f "$LIMINE_DIR/limine-uefi-cd.bin"
test -f "$LIMINE_DIR/BOOTX64.EFI"
test -x "$LIMINE_DIR/limine" || test -f "$LIMINE_DIR/limine"

echo "OK: Limine ready in $LIMINE_DIR"
