#include "mcs_sync.h"

void mcs_lockdep_init(mcs_lockdep_state_t *state) {
    if (state == 0) {
        return;
    }
    for (uint32_t i = 0; i < MCS_LOCKDEP_MAX_HELD; i++) {
        state->held_class[i] = 0;
        state->held_name[i] = 0;
    }
    state->depth = 0;
    state->violation_count = 0;
}

bool mcs_lockdep_is_held(const mcs_lockdep_state_t *state, uint32_t class_id) {
    if (state == 0 || class_id == 0) {
        return false;
    }
    for (uint32_t i = 0; i < state->depth && i < MCS_LOCKDEP_MAX_HELD; i++) {
        if (state->held_class[i] == class_id) {
            return true;
        }
    }
    return false;
}

int mcs_lockdep_before_acquire(mcs_lockdep_state_t *state, uint32_t class_id, const char *name) {
    if (state == 0 || class_id == 0) {
        return MCS_SYNC_EINVAL;
    }
    if (state->depth >= MCS_LOCKDEP_MAX_HELD) {
        state->violation_count++;
        return MCS_SYNC_EOVERFLOW;
    }
    for (uint32_t i = 0; i < state->depth; i++) {
        if (state->held_class[i] == class_id) {
            state->violation_count++;
            return MCS_SYNC_EDEADLK;
        }
    }
    if (state->depth > 0) {
        uint32_t top = state->held_class[state->depth - 1u];
        if (class_id < top) {
            state->violation_count++;
            return MCS_SYNC_EDEADLK;
        }
    }
    state->held_class[state->depth] = class_id;
    state->held_name[state->depth] = name;
    state->depth++;
    return MCS_SYNC_OK;
}

int mcs_lockdep_after_release(mcs_lockdep_state_t *state, uint32_t class_id, const char *name) {
    (void)name;
    if (state == 0 || class_id == 0) {
        return MCS_SYNC_EINVAL;
    }
    if (state->depth == 0) {
        state->violation_count++;
        return MCS_SYNC_EPERM;
    }
    uint32_t index = state->depth - 1u;
    if (state->held_class[index] != class_id) {
        state->violation_count++;
        return MCS_SYNC_EDEADLK;
    }
    state->held_class[index] = 0;
    state->held_name[index] = 0;
    state->depth--;
    return MCS_SYNC_OK;
}
