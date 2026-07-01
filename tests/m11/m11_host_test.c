#include "m11_elf_loader.h"
#include <stdio.h>
#include <string.h>

#define IMAGE_SIZE 12288u

static struct m11_user_region test_region(void) {
    struct m11_user_region r;
    r.base = 0x0000000000400000ull;
    r.limit = 0x0000008000000000ull;
    return r;
}

static void make_valid_image(unsigned char image[IMAGE_SIZE]) {
    memset(image, 0, IMAGE_SIZE);
    struct m11_elf64_ehdr *eh = (struct m11_elf64_ehdr *)(void *)image;
    eh->e_ident[0] = M11_ELFMAG0;
    eh->e_ident[1] = M11_ELFMAG1;
    eh->e_ident[2] = M11_ELFMAG2;
    eh->e_ident[3] = M11_ELFMAG3;
    eh->e_ident[4] = M11_ELFCLASS64;
    eh->e_ident[5] = M11_ELFDATA2LSB;
    eh->e_ident[6] = M11_EV_CURRENT;
    eh->e_type = M11_ET_EXEC;
    eh->e_machine = M11_EM_X86_64;
    eh->e_version = M11_EV_CURRENT;
    eh->e_entry = 0x0000000000401000ull;
    eh->e_phoff = sizeof(struct m11_elf64_ehdr);
    eh->e_ehsize = sizeof(struct m11_elf64_ehdr);
    eh->e_phentsize = sizeof(struct m11_elf64_phdr);
    eh->e_phnum = 2u;
    struct m11_elf64_phdr *ph = (struct m11_elf64_phdr *)(void *)(image + eh->e_phoff);
    ph[0].p_type = M11_PT_LOAD;
    ph[0].p_flags = M11_PF_R | M11_PF_X;
    ph[0].p_offset = 0x1000u;
    ph[0].p_vaddr = 0x0000000000400000ull;
    ph[0].p_filesz = 16u;
    ph[0].p_memsz = 4096u;
    ph[0].p_align = M11_PAGE_SIZE;
    ph[1].p_type = M11_PT_LOAD;
    ph[1].p_flags = M11_PF_R | M11_PF_W;
    ph[1].p_offset = 0x2000u;
    ph[1].p_vaddr = 0x0000000000401000ull;
    ph[1].p_filesz = 8u;
    ph[1].p_memsz = 4096u;
    ph[1].p_align = M11_PAGE_SIZE;
}

static int expect_code(const char *name, int got, int expected) {
    if (got != expected) {
        printf("FAIL %s: got=%s(%d) expected=%s(%d)\n", name, m11_error_name(got), got,
               m11_error_name(expected), expected);
        return 1;
    }
    printf("PASS %s: %s\n", name, m11_error_name(got));
    return 0;
}

int main(void) {
    unsigned failures = 0u;
    unsigned char image[IMAGE_SIZE];
    struct m11_process_image_plan plan;
    make_valid_image(image);
    int rc = m11_elf64_plan_load(image, IMAGE_SIZE, test_region(), &plan);
    failures += expect_code("valid ELF64 image", rc, M11_OK);
    if (rc == M11_OK && (plan.entry != 0x401000ull || plan.segment_count != 2u)) {
        printf("FAIL valid plan fields\n");
        failures++;
    } else if (rc == M11_OK) {
        printf("PASS valid plan fields: entry=0x%llx segments=%u\n",
               (unsigned long long)plan.entry, plan.segment_count);
    }

    make_valid_image(image);
    image[0] = 0u;
    failures += expect_code("bad magic", m11_elf64_plan_load(image, IMAGE_SIZE, test_region(), &plan), M11_ERR_MAGIC);

    make_valid_image(image);
    ((struct m11_elf64_ehdr *)(void *)image)->e_machine = 3u;
    failures += expect_code("bad machine", m11_elf64_plan_load(image, IMAGE_SIZE, test_region(), &plan), M11_ERR_MACHINE);

    make_valid_image(image);
    ((struct m11_elf64_ehdr *)(void *)image)->e_entry = 0x1000u;
    failures += expect_code("entry outside user range", m11_elf64_plan_load(image, IMAGE_SIZE, test_region(), &plan), M11_ERR_ENTRY);

    make_valid_image(image);
    struct m11_elf64_phdr *ph = (struct m11_elf64_phdr *)(void *)(image + sizeof(struct m11_elf64_ehdr));
    ph[0].p_memsz = 4u;
    ph[0].p_filesz = 16u;
    failures += expect_code("memsz below filesz", m11_elf64_plan_load(image, IMAGE_SIZE, test_region(), &plan), M11_ERR_SEGBOUNDS);

    make_valid_image(image);
    ph = (struct m11_elf64_phdr *)(void *)(image + sizeof(struct m11_elf64_ehdr));
    ph[0].p_offset = 0x3000u;
    ph[0].p_filesz = 1u;
    failures += expect_code("file range outside image", m11_elf64_plan_load(image, IMAGE_SIZE, test_region(), &plan), M11_ERR_SEGBOUNDS);

    make_valid_image(image);
    ph = (struct m11_elf64_phdr *)(void *)(image + sizeof(struct m11_elf64_ehdr));
    ph[0].p_align = 24u;
    failures += expect_code("bad alignment", m11_elf64_plan_load(image, IMAGE_SIZE, test_region(), &plan), M11_ERR_ALIGN);

    make_valid_image(image);
    ph = (struct m11_elf64_phdr *)(void *)(image + sizeof(struct m11_elf64_ehdr));
    ph[0].p_vaddr = 0x0000800000000000ull;
    failures += expect_code("segment outside user range", m11_elf64_plan_load(image, IMAGE_SIZE, test_region(), &plan), M11_ERR_SEGRANGE);

    if (failures != 0u) {
        printf("M11 host tests failed: %u\n", failures);
        return 1;
    }
    printf("M11 host tests passed.\n");
    return 0;
}

