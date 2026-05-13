void serial_init(void);
void serial_write(const char *s);

__attribute__((noreturn)) static void halt_forever(void) {
    for (;;) {
        __asm__ volatile ("cli; hlt" : : : "memory");
    }
}

void kmain(void) {
    serial_init();
    serial_write("MCSOS 260502 M2 boot path entered\n");
    serial_write("[M2] early serial online\n");
    serial_write("[M2] kernel reached controlled halt loop\n");
    halt_forever();
}
