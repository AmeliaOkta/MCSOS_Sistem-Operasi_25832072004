#ifndef MCS_SYNC_H
#define MCS_SYNC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MCS_LOCKDEP_MAX_HELD 16u
#define MCS_LOCK_NAME_MAX 32u

#define MCS_SYNC_OK 0
#define MCS_SYNC_EINVAL (-22)
#define MCS_SYNC_EBUSY (-16)
#define MCS_SYNC_EPERM (-1)
#define MCS_SYNC_EDEADLK (-35)
#define MCS_SYNC_EOVERFLOW (-75)

typedef struct mcs_lockdep_state {
    uint32_t held_class[MCS_LOCKDEP_MAX_HELD];
    const char *held_name[MCS_LOCKDEP_MAX_HELD];
    uint32_t depth;
    uint32_t violation_count;
} mcs_lockdep_state_t;

typedef struct mcs_spinlock {
    volatile uint32_t locked;
    uint32_t class_id;
    const char *name;
} mcs_spinlock_t;

typedef struct mcs_mutex {
    volatile uint32_t locked;
    uint64_t owner;
    uint32_t class_id;
    const char *name;
} mcs_mutex_t;

void mcs_lockdep_init(mcs_lockdep_state_t *state);
int mcs_lockdep_before_acquire(mcs_lockdep_state_t *state, uint32_t class_id, const char *name);
int mcs_lockdep_after_release(mcs_lockdep_state_t *state, uint32_t class_id, const char *name);
bool mcs_lockdep_is_held(const mcs_lockdep_state_t *state, uint32_t class_id);

void mcs_spin_init(mcs_spinlock_t *lock, uint32_t class_id, const char *name);
bool mcs_spin_try_lock(mcs_spinlock_t *lock);
void mcs_spin_lock(mcs_spinlock_t *lock);
void mcs_spin_unlock(mcs_spinlock_t *lock);
bool mcs_spin_is_locked(const mcs_spinlock_t *lock);

void mcs_mutex_init(mcs_mutex_t *mutex, uint32_t class_id, const char *name);
int mcs_mutex_try_lock(mcs_mutex_t *mutex, uint64_t owner_id);
int mcs_mutex_unlock(mcs_mutex_t *mutex, uint64_t owner_id);
bool mcs_mutex_is_locked(const mcs_mutex_t *mutex);
uint64_t mcs_mutex_owner(const mcs_mutex_t *mutex);

#endif
