.PHONY: meta smoke clean check

meta:
	@bash tools/check_env.sh

smoke:
	@mkdir -p build/smoke
	clang --target=x86_64-unknown-none -ffreestanding -fno-stack-protector -fno-pic -mno-red-zone -c smoke/freestanding.c -o build/smoke/freestanding.o
	readelf -h build/smoke/freestanding.o

check:
	@bash tools/check_env.sh

clean:
	rm -rf build/
