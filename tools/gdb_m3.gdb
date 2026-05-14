set pagination off
set confirm off
file build/kernel.elf
target remote localhost:1234
break kmain
break kernel_panic_at
continue
info registers
bt
disassemble /m kmain
