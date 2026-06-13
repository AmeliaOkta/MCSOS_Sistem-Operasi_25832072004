#include "mcsos_thread.h"

static uintptr_t align_down_uintptr(uintptr_t value, uintptr_t alignment) {
    return value & ~(alignment - 1u);
}

static int valid_thread_object(const mcsos_thread_t *thread) {
    return thread != (const mcsos_thread_t *)0
        && thread->magic == MCSOS_THREAD_MAGIC;
}

static void zero_context(mcsos_context_t *ctx) {
    ctx->rsp = 0; ctx->rbp = 0; ctx->rbx = 0;
    ctx->r12 = 0; ctx->r13 = 0; ctx->r14 = 0;
    ctx->r15 = 0; ctx->rip = 0;
}

void mcsos_thread_trampoline(void) {
    for (;;) {
#if defined(__x86_64__)
        __asm__ volatile("hlt");
#else
        __builtin_trap();
#endif
    }
}

int mcsos_scheduler_init(mcsos_scheduler_t *sched,
                         mcsos_thread_t    *boot_thread) {
    if (!sched || !boot_thread) return MCSOS_SCHED_EINVAL;
    boot_thread->magic      = MCSOS_THREAD_MAGIC;
    boot_thread->id         = 0;
    boot_thread->name       = "boot";
    boot_thread->state      = MCSOS_THREAD_RUNNING;
    boot_thread->entry      = (mcsos_thread_entry_t)0;
    boot_thread->arg        = (void *)0;
    boot_thread->stack_base = (uint8_t *)0;
    boot_thread->stack_size = 0;
    boot_thread->next       = (mcsos_thread_t *)0;
    boot_thread->switches   = 0;
    boot_thread->ticks      = 0;
    boot_thread->exit_code  = 0;
    zero_context(&boot_thread->context);
    sched->current          = boot_thread;
    sched->idle             = boot_thread;
    sched->ready_head       = (mcsos_thread_t *)0;
    sched->ready_tail       = (mcsos_thread_t *)0;
    sched->next_id          = 1;
    sched->runnable_count   = 0;
    sched->context_switches = 0;
    sched->ticks            = 0;
    sched->initialized      = 1;
    return MCSOS_SCHED_OK;
}

int mcsos_thread_prepare(mcsos_thread_t      *thread,
                         const char          *name,
                         mcsos_thread_entry_t entry,
                         void                *arg,
                         void                *stack_base,
                         size_t               stack_size,
                         uint64_t             id) {
    if (!thread || !entry || !stack_base)    return MCSOS_SCHED_EINVAL;
    if (stack_size < MCSOS_MIN_KERNEL_STACK) return MCSOS_SCHED_ESTACK;
    uintptr_t low  = (uintptr_t)stack_base;
    uintptr_t high = low + (uintptr_t)stack_size;
    if (high <= low)                         return MCSOS_SCHED_ESTACK;
    uintptr_t top  = align_down_uintptr(high, MCSOS_STACK_ALIGN);
    if (top <= low + 128u)                   return MCSOS_SCHED_ESTACK;
    top -= sizeof(uint64_t);
    *((uint64_t *)top) = UINT64_C(0);
    thread->magic       = MCSOS_THREAD_MAGIC;
    thread->id          = id;
    thread->name        = name;
    thread->state       = MCSOS_THREAD_NEW;
    zero_context(&thread->context);
    thread->context.rsp = (uint64_t)top;
    thread->context.rip = (uint64_t)(uintptr_t)mcsos_thread_trampoline;
    thread->entry       = entry;
    thread->arg         = arg;
    thread->stack_base  = (uint8_t *)stack_base;
    thread->stack_size  = stack_size;
    thread->next        = (mcsos_thread_t *)0;
    thread->switches    = 0;
    thread->ticks       = 0;
    thread->exit_code   = 0;
    return MCSOS_SCHED_OK;
}

int mcsos_sched_enqueue(mcsos_scheduler_t *sched,
                        mcsos_thread_t    *thread) {
    if (!sched || !sched->initialized || !valid_thread_object(thread))
        return MCSOS_SCHED_EINVAL;
    if (thread->state != MCSOS_THREAD_NEW
     && thread->state != MCSOS_THREAD_READY
     && thread->state != MCSOS_THREAD_BLOCKED)
        return MCSOS_SCHED_ESTATE;
    thread->state = MCSOS_THREAD_READY;
    thread->next  = (mcsos_thread_t *)0;
    if (!sched->ready_tail) {
        sched->ready_head = thread;
        sched->ready_tail = thread;
    } else {
        sched->ready_tail->next = thread;
        sched->ready_tail       = thread;
    }
    sched->runnable_count++;
    return MCSOS_SCHED_OK;
}

mcsos_thread_t *mcsos_sched_pick_next(mcsos_scheduler_t *sched) {
    if (!sched || !sched->initialized) return (mcsos_thread_t *)0;
    mcsos_thread_t *t = sched->ready_head;
    if (!t) return sched->idle;
    sched->ready_head = t->next;
    if (!sched->ready_head) sched->ready_tail = (mcsos_thread_t *)0;
    t->next = (mcsos_thread_t *)0;
    if (sched->runnable_count > 0u) sched->runnable_count--;
    return t;
}

int mcsos_sched_yield(mcsos_scheduler_t *sched) {
    if (!sched || !sched->initialized || !valid_thread_object(sched->current))
        return MCSOS_SCHED_EINVAL;
    mcsos_thread_t *old  = sched->current;
    mcsos_thread_t *next = mcsos_sched_pick_next(sched);
    if (!valid_thread_object(next)) return MCSOS_SCHED_ECORRUPT;
    if (next == old) { old->state = MCSOS_THREAD_RUNNING; return MCSOS_SCHED_OK; }
    if (old->state == MCSOS_THREAD_RUNNING && old != sched->idle) {
        old->state = MCSOS_THREAD_READY;
        int rc = mcsos_sched_enqueue(sched, old);
        if (rc != MCSOS_SCHED_OK) return rc;
    }
    next->state = MCSOS_THREAD_RUNNING;
    sched->current = next;
    old->switches++;
    next->switches++;
    sched->context_switches++;
#if !defined(MCSOS_HOST_TEST)
    mcsos_context_switch(&old->context, &next->context);
#endif
    return MCSOS_SCHED_OK;
}

int mcsos_sched_tick(mcsos_scheduler_t *sched) {
    if (!sched || !sched->initialized || !valid_thread_object(sched->current))
        return MCSOS_SCHED_EINVAL;
    sched->ticks++;
    sched->current->ticks++;
    return MCSOS_SCHED_OK;
}

int mcsos_thread_block_current(mcsos_scheduler_t *sched) {
    if (!sched || !sched->initialized || !valid_thread_object(sched->current))
        return MCSOS_SCHED_EINVAL;
    if (sched->current == sched->idle) return MCSOS_SCHED_ESTATE;
    sched->current->state = MCSOS_THREAD_BLOCKED;
    return mcsos_sched_yield(sched);
}

int mcsos_thread_mark_ready(mcsos_scheduler_t *sched,
                            mcsos_thread_t    *thread) {
    if (!valid_thread_object(thread))          return MCSOS_SCHED_EINVAL;
    if (thread->state != MCSOS_THREAD_BLOCKED) return MCSOS_SCHED_ESTATE;
    return mcsos_sched_enqueue(sched, thread);
}

size_t mcsos_sched_ready_count(const mcsos_scheduler_t *sched) {
    if (!sched || !sched->initialized) return 0u;
    size_t n = 0u;
    const mcsos_thread_t *c = sched->ready_head;
    while (c) { n++; c = c->next; }
    return n;
}

int mcsos_sched_validate(const mcsos_scheduler_t *sched) {
    if (!sched || !sched->initialized || !valid_thread_object(sched->current))
        return MCSOS_SCHED_EINVAL;
    size_t count = 0u;
    const mcsos_thread_t *cursor = sched->ready_head;
    const mcsos_thread_t *last   = (const mcsos_thread_t *)0;
    while (cursor) {
        if (!valid_thread_object(cursor) || cursor->state != MCSOS_THREAD_READY)
            return MCSOS_SCHED_ECORRUPT;
        if (cursor == sched->current) return MCSOS_SCHED_ECORRUPT;
        last   = cursor;
        cursor = cursor->next;
        count++;
        if (count > sched->runnable_count + 1u) return MCSOS_SCHED_ECORRUPT;
    }
    if (last != sched->ready_tail)              return MCSOS_SCHED_ECORRUPT;
    if (count != (size_t)sched->runnable_count) return MCSOS_SCHED_ECORRUPT;
    return MCSOS_SCHED_OK;
}
