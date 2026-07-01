#include "mcs_sync.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define THREADS 4
#define ITERS 25000

static mcs_spinlock_t g_counter_lock;
static unsigned long g_counter;

static void require_true(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "[FAIL] %s\n", message);
        exit(1);
    }
}

static void *worker(void *arg) {
    (void)arg;
    for (int i = 0; i < ITERS; i++) {
        mcs_spin_lock(&g_counter_lock);
        g_counter++;
        mcs_spin_unlock(&g_counter_lock);
    }
    return 0;
}

static void test_lockdep_order(void) {
    mcs_lockdep_state_t st;
    mcs_lockdep_init(&st);
    require_true(mcs_lockdep_before_acquire(&st, 10u, "pmm") == MCS_SYNC_OK, "acquire rank 10");
    require_true(mcs_lockdep_before_acquire(&st, 20u, "vmm") == MCS_SYNC_OK, "acquire rank 20");
    require_true(st.depth == 2u, "depth after two locks");
    require_true(mcs_lockdep_after_release(&st, 20u, "vmm") == MCS_SYNC_OK, "release rank 20");
    require_true(mcs_lockdep_after_release(&st, 10u, "pmm") == MCS_SYNC_OK, "release rank 10");
    require_true(st.depth == 0u, "depth zero after releases");
}

static void test_lockdep_negative(void) {
    mcs_lockdep_state_t st;
    mcs_lockdep_init(&st);
    require_true(mcs_lockdep_before_acquire(&st, 20u, "vmm") == MCS_SYNC_OK, "acquire rank 20 first");
    require_true(mcs_lockdep_before_acquire(&st, 10u, "pmm") == MCS_SYNC_EDEADLK, "reject descending rank");
    require_true(mcs_lockdep_before_acquire(&st, 20u, "vmm") == MCS_SYNC_EDEADLK, "reject recursion");
    require_true(st.violation_count == 2u, "two lockdep violations counted");
    require_true(mcs_lockdep_after_release(&st, 20u, "vmm") == MCS_SYNC_OK, "release rank 20 after negatives");
}

static void test_spinlock_threads(void) {
    pthread_t thread[THREADS];
    mcs_spin_init(&g_counter_lock, 100u, "counter");
    g_counter = 0;
    for (int i = 0; i < THREADS; i++) {
        require_true(pthread_create(&thread[i], 0, worker, 0) == 0, "pthread_create");
    }
    for (int i = 0; i < THREADS; i++) {
        require_true(pthread_join(thread[i], 0) == 0, "pthread_join");
    }
    require_true(g_counter == (unsigned long)THREADS * (unsigned long)ITERS, "spinlock-protected counter exact");
    require_true(!mcs_spin_is_locked(&g_counter_lock), "spinlock unlocked after test");
}

static void test_mutex_owner(void) {
    mcs_mutex_t mutex;
    mcs_mutex_init(&mutex, 200u, "proc_table");
    require_true(mcs_mutex_try_lock(&mutex, 1u) == MCS_SYNC_OK, "owner 1 lock");
    require_true(mcs_mutex_owner(&mutex) == 1u, "owner recorded");
    require_true(mcs_mutex_try_lock(&mutex, 1u) == MCS_SYNC_EDEADLK, "recursive mutex rejected");
    require_true(mcs_mutex_try_lock(&mutex, 2u) == MCS_SYNC_EBUSY, "other owner sees busy");
    require_true(mcs_mutex_unlock(&mutex, 2u) == MCS_SYNC_EPERM, "non-owner unlock rejected");
    require_true(mcs_mutex_unlock(&mutex, 1u) == MCS_SYNC_OK, "owner unlock");
    require_true(!mcs_mutex_is_locked(&mutex), "mutex unlocked");
}

int main(void) {
    test_lockdep_order();
    test_lockdep_negative();
    test_spinlock_threads();
    test_mutex_owner();
    puts("[PASS] M12 synchronization host tests passed");
    return 0;
}
