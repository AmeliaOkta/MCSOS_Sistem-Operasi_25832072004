#ifndef MCSOS_M11_ELF_LOADER_H
#define MCSOS_M11_ELF_LOADER_H

#include <stddef.h>
#include <stdint.h>

#define M11_EI_NIDENT 16u
#define M11_ELFMAG0 0x7fu
#define M11_ELFMAG1 'E'
#define M11_ELFMAG2 'L'
#define M11_ELFMAG3 'F'
#define M11_ELFCLASS64 2u
#define M11_ELFDATA2LSB 1u
#define M11_EV_CURRENT 1u
#define M11_ET_EXEC 2u
#define M11_ET_DYN 3u
#define M11_EM_X86_64 62u
#define M11_PT_LOAD 1u
#define M11_PF_X 1u
#define M11_PF_W 2u
#define M11_PF_R 4u
#define M11_MAX_LOAD_SEGMENTS 8u
#define M11_PAGE_SIZE 4096ull

#define M11_OK 0
#define M11_ERR_NULL -1
#define M11_ERR_SIZE -2
#define M11_ERR_MAGIC -3
#define M11_ERR_CLASS -4
#define M11_ERR_ENDIAN -5
#define M11_ERR_VERSION -6
#define M11_ERR_TYPE -7
#define M11_ERR_MACHINE -8
#define M11_ERR_EHSIZE -9
#define M11_ERR_PHENTSIZE -10
#define M11_ERR_PHBOUNDS -11
#define M11_ERR_ALIGN -12
#define M11_ERR_SEGBOUNDS -13
#define M11_ERR_SEGRANGE -14
#define M11_ERR_SEGCOUNT -15
#define M11_ERR_ENTRY -16
#define M11_ERR_FLAGS -17

struct m11_elf64_ehdr {
    unsigned char e_ident[M11_EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint64_t e_entry;
    uint64_t e_phoff;
    uint64_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};

struct m11_elf64_phdr {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
};

struct m11_user_region {
    uint64_t base;
    uint64_t limit;
};

struct m11_segment_plan {
    uint64_t file_offset;
    uint64_t vaddr;
    uint64_t filesz;
    uint64_t memsz;
    uint64_t align;
    uint32_t flags;
};

struct m11_process_image_plan {
    uint64_t entry;
    uint32_t segment_count;
    struct m11_segment_plan segments[M11_MAX_LOAD_SEGMENTS];
};

int m11_validate_user_range(struct m11_user_region region, uint64_t base, uint64_t size);
int m11_elf64_plan_load(const void *image, size_t image_size,
                        struct m11_user_region region,
                        struct m11_process_image_plan *out_plan);
const char *m11_error_name(int code);

#endif

