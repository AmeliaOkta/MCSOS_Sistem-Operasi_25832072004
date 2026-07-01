#include "mcs_sync.h"

static inline void mcs_cpu_relax(void) {
#if defined(__x86_64__) || defined(__i386__)
    __asm__ __volatile__("pause" ::: "memory");
#else
    __asm__ __volatile__("" ::: "memory");
#endif
}

void mcs_spin_init(mcs_spinlock_t *lock, uint32_t class_id, const char *name) {
    if (lock == 0) {
        return;
    }
    __atomic_store_n(&lock->locked, 0u, __ATOMIC_RELAXED);
    lock->class_id = class_id;
    lock->name = name;
}

bool mcs_spin_try_lock(mcs_spinlock_t *lock) {
    if (lock == 0) {
        return false;
    }
    uint32_t old = __atomic_exchange_n(&lock->locked, 1u, __ATOMIC_ACQUIRE);
    return old == 0u;
}

void mcs_spin_lock(mcs_spinlock_t *lock) {
    while (!mcs_spin_try_lock(lock)) {
        while (__atomic_load_n(&lock->locked, __ATOMIC_RELAXED) != 0u) {
            mcs_cpu_relax();
        }
    }
}

void mcs_spin_unlock(mcs_spinlock_t *lock) {
    if (lock == 0) {
        return;
    }
    __atomic_store_n(&lock->locked, 0u, __ATOMIC_RELEASE);
}

bool mcs_spin_is_locked(const mcs_spinlock_t *lock) {
    if (lock == 0) {
        return false;
    }
    return __atomic_load_n(&lock->locked, __ATOMIC_RELAXED) != 0u;
}
