#include "m11_elf_loader.h"

static int m11_add_overflow_u64(uint64_t a, uint64_t b, uint64_t *out) {
    uint64_t r = a + b;
    if (r < a) {
        return 1;
    }
    *out = r;
    return 0;
}

static int m11_is_power_of_two_u64(uint64_t v) {
    return v != 0u && (v & (v - 1u)) == 0u;
}

static void m11_zero_plan(struct m11_process_image_plan *plan) {
    plan->entry = 0u;
    plan->segment_count = 0u;
    for (uint32_t i = 0u; i < M11_MAX_LOAD_SEGMENTS; ++i) {
        plan->segments[i].file_offset = 0u;
        plan->segments[i].vaddr = 0u;
        plan->segments[i].filesz = 0u;
        plan->segments[i].memsz = 0u;
        plan->segments[i].align = 0u;
        plan->segments[i].flags = 0u;
    }
}
int m11_validate_user_range(struct m11_user_region region, uint64_t base, uint64_t size) {
    uint64_t end = 0u;
    if (region.base >= region.limit) {
        return M11_ERR_SEGRANGE;
    }
    if (size == 0u) {
        return M11_ERR_SEGRANGE;
    }
    if (m11_add_overflow_u64(base, size, &end) != 0) {
        return M11_ERR_SEGRANGE;
    }
    if (base < region.base || end > region.limit || end <= base) {
        return M11_ERR_SEGRANGE;
    }
    return M11_OK;
}

static int m11_validate_ident(const struct m11_elf64_ehdr *eh) {
    if (eh->e_ident[0] != M11_ELFMAG0 || eh->e_ident[1] != M11_ELFMAG1 ||
        eh->e_ident[2] != M11_ELFMAG2 || eh->e_ident[3] != M11_ELFMAG3) {
        return M11_ERR_MAGIC;
    }
    if (eh->e_ident[4] != M11_ELFCLASS64) {
        return M11_ERR_CLASS;
    }
    if (eh->e_ident[5] != M11_ELFDATA2LSB) {
        return M11_ERR_ENDIAN;
    }
    if (eh->e_ident[6] != M11_EV_CURRENT || eh->e_version != M11_EV_CURRENT) {
        return M11_ERR_VERSION;
    }
    return M11_OK;
}

static int m11_validate_phdr_bounds(const struct m11_elf64_ehdr *eh, size_t image_size) {
    uint64_t ph_table_bytes = 0u;
    uint64_t ph_end = 0u;
    if (eh->e_phnum == 0u) {
        return M11_ERR_PHBOUNDS;
    }
    if (eh->e_phentsize != sizeof(struct m11_elf64_phdr)) {
        return M11_ERR_PHENTSIZE;
    }
    ph_table_bytes = (uint64_t)eh->e_phentsize * (uint64_t)eh->e_phnum;
    if (eh->e_phnum != 0u && ph_table_bytes / eh->e_phnum != eh->e_phentsize) {
        return M11_ERR_PHBOUNDS;
    }
    if (m11_add_overflow_u64(eh->e_phoff, ph_table_bytes, &ph_end) != 0) {
        return M11_ERR_PHBOUNDS;
    }
    if (ph_end > (uint64_t)image_size || eh->e_phoff > (uint64_t)image_size) {
        return M11_ERR_PHBOUNDS;
    }
    return M11_OK;
}
static int m11_validate_load_segment(const struct m11_elf64_phdr *ph, size_t image_size,
                                     struct m11_user_region region) {
    uint64_t file_end = 0u;
    if ((ph->p_flags & ~(M11_PF_R | M11_PF_W | M11_PF_X)) != 0u) {
        return M11_ERR_FLAGS;
    }
    if ((ph->p_flags & M11_PF_W) != 0u && (ph->p_flags & M11_PF_X) != 0u) {
        return M11_ERR_FLAGS;
    }
    if (ph->p_memsz < ph->p_filesz) {
        return M11_ERR_SEGBOUNDS;
    }
    if (ph->p_align != 0u && ph->p_align != 1u) {
        if (!m11_is_power_of_two_u64(ph->p_align)) {
            return M11_ERR_ALIGN;
        }
        if ((ph->p_vaddr % ph->p_align) != (ph->p_offset % ph->p_align)) {
            return M11_ERR_ALIGN;
        }
    }
    if (m11_add_overflow_u64(ph->p_offset, ph->p_filesz, &file_end) != 0) {
        return M11_ERR_SEGBOUNDS;
    }
    if (file_end > (uint64_t)image_size || ph->p_offset > (uint64_t)image_size) {
        return M11_ERR_SEGBOUNDS;
    }
    return m11_validate_user_range(region, ph->p_vaddr, ph->p_memsz);
}

int m11_elf64_plan_load(const void *image, size_t image_size,
                        struct m11_user_region region,
                        struct m11_process_image_plan *out_plan) {
    const struct m11_elf64_ehdr *eh = (const struct m11_elf64_ehdr *)image;
    int rc = M11_OK;
    if (image == 0 || out_plan == 0) {
        return M11_ERR_NULL;
    }
    m11_zero_plan(out_plan);
    if (image_size < sizeof(struct m11_elf64_ehdr)) {
        return M11_ERR_SIZE;
    }
    rc = m11_validate_ident(eh);
    if (rc != M11_OK) {
        return rc;
    }
    if (eh->e_type != M11_ET_EXEC && eh->e_type != M11_ET_DYN) {
        return M11_ERR_TYPE;
    }
    if (eh->e_machine != M11_EM_X86_64) {
        return M11_ERR_MACHINE;
    }
    if (eh->e_ehsize != sizeof(struct m11_elf64_ehdr)) {
        return M11_ERR_EHSIZE;
    }
    rc = m11_validate_phdr_bounds(eh, image_size);
    if (rc != M11_OK) {
        return rc;
    }
    rc = m11_validate_user_range(region, eh->e_entry, 1u);
    if (rc != M11_OK) {
        return M11_ERR_ENTRY;
    }
    const unsigned char *bytes = (const unsigned char *)image;
    const struct m11_elf64_phdr *ph = (const struct m11_elf64_phdr *)(const void *)(bytes + eh->e_phoff);
    out_plan->entry = eh->e_entry;
    for (uint16_t i = 0u; i < eh->e_phnum; ++i) {
        if (ph[i].p_type != M11_PT_LOAD) {
            continue;
        }
        if (out_plan->segment_count >= M11_MAX_LOAD_SEGMENTS) {
            m11_zero_plan(out_plan);
            return M11_ERR_SEGCOUNT;
        }
        rc = m11_validate_load_segment(&ph[i], image_size, region);
        if (rc != M11_OK) {
            m11_zero_plan(out_plan);
            return rc;
        }
        struct m11_segment_plan *seg = &out_plan->segments[out_plan->segment_count];
        seg->file_offset = ph[i].p_offset;
        seg->vaddr = ph[i].p_vaddr;
        seg->filesz = ph[i].p_filesz;
        seg->memsz = ph[i].p_memsz;
        seg->align = ph[i].p_align;
        seg->flags = ph[i].p_flags;
        out_plan->segment_count++;
    }
    if (out_plan->segment_count == 0u) {
        return M11_ERR_SEGCOUNT;
    }
    return M11_OK;
}

const char *m11_error_name(int code) {
    switch (code) {
        case M11_OK: return "M11_OK";
        case M11_ERR_NULL: return "M11_ERR_NULL";
        case M11_ERR_SIZE: return "M11_ERR_SIZE";
        case M11_ERR_MAGIC: return "M11_ERR_MAGIC";
        case M11_ERR_CLASS: return "M11_ERR_CLASS";
        case M11_ERR_ENDIAN: return "M11_ERR_ENDIAN";
        case M11_ERR_VERSION: return "M11_ERR_VERSION";
        case M11_ERR_TYPE: return "M11_ERR_TYPE";
        case M11_ERR_MACHINE: return "M11_ERR_MACHINE";
        case M11_ERR_EHSIZE: return "M11_ERR_EHSIZE";
        case M11_ERR_PHENTSIZE: return "M11_ERR_PHENTSIZE";
        case M11_ERR_PHBOUNDS: return "M11_ERR_PHBOUNDS";
        case M11_ERR_ALIGN: return "M11_ERR_ALIGN";
        case M11_ERR_SEGBOUNDS: return "M11_ERR_SEGBOUNDS";
        case M11_ERR_SEGRANGE: return "M11_ERR_SEGRANGE";
        case M11_ERR_SEGCOUNT: return "M11_ERR_SEGCOUNT";
        case M11_ERR_ENTRY: return "M11_ERR_ENTRY";
        case M11_ERR_FLAGS: return "M11_ERR_FLAGS";
        default: return "M11_ERR_UNKNOWN";
    }
}
