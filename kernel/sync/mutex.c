#include "mcs_sync.h"

void mcs_mutex_init(mcs_mutex_t *mutex, uint32_t class_id, const char *name) {
    if (mutex == 0) {
        return;
    }
    __atomic_store_n(&mutex->locked, 0u, __ATOMIC_RELAXED);
    __atomic_store_n(&mutex->owner, 0u, __ATOMIC_RELAXED);
    mutex->class_id = class_id;
    mutex->name = name;
}

int mcs_mutex_try_lock(mcs_mutex_t *mutex, uint64_t owner_id) {
    if (mutex == 0 || owner_id == 0u) {
        return MCS_SYNC_EINVAL;
    }
    uint32_t expected = 0u;
    if (!__atomic_compare_exchange_n(&mutex->locked, &expected, 1u, false, __ATOMIC_ACQUIRE, __ATOMIC_RELAXED)) {
        if (__atomic_load_n(&mutex->owner, __ATOMIC_RELAXED) == owner_id) {
            return MCS_SYNC_EDEADLK;
        }
        return MCS_SYNC_EBUSY;
    }
    __atomic_store_n(&mutex->owner, owner_id, __ATOMIC_RELEASE);
    return MCS_SYNC_OK;
}

int mcs_mutex_unlock(mcs_mutex_t *mutex, uint64_t owner_id) {
    if (mutex == 0 || owner_id == 0u) {
        return MCS_SYNC_EINVAL;
    }
    uint64_t owner = __atomic_load_n(&mutex->owner, __ATOMIC_ACQUIRE);
    if (owner != owner_id) {
        return MCS_SYNC_EPERM;
    }
    __atomic_store_n(&mutex->owner, 0u, __ATOMIC_RELEASE);
    __atomic_store_n(&mutex->locked, 0u, __ATOMIC_RELEASE);
    return MCS_SYNC_OK;
}

bool mcs_mutex_is_locked(const mcs_mutex_t *mutex) {
    if (mutex == 0) {
        return false;
    }
    return __atomic_load_n(&mutex->locked, __ATOMIC_RELAXED) != 0u;
}

uint64_t mcs_mutex_owner(const mcs_mutex_t *mutex) {
    if (mutex == 0) {
        return 0u;
    }
    return __atomic_load_n(&mutex->owner, __ATOMIC_RELAXED);
}
