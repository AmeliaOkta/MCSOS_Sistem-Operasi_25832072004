#include <stdio.h>
#include <stdint.h>
#include "mcsos_thread.h"

static void noop(void *arg) { (void)arg; }

#define REQUIRE(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #expr); \
        return 1; \
    } \
} while (0)

int main(void) {
    mcsos_scheduler_t sched;
    mcsos_thread_t    boot, a, b;
    unsigned char     stack_a[8192];
    unsigned char     stack_b[8192];

    REQUIRE(mcsos_scheduler_init(&sched, &boot) == MCSOS_SCHED_OK);
    REQUIRE(mcsos_sched_validate(&sched)         == MCSOS_SCHED_OK);

    REQUIRE(mcsos_thread_prepare(&a, "a", noop, (void*)0,
            stack_a, sizeof(stack_a), sched.next_id++) == MCSOS_SCHED_OK);
    REQUIRE(mcsos_thread_prepare(&b, "b", noop, (void*)0,
            stack_b, sizeof(stack_b), sched.next_id++) == MCSOS_SCHED_OK);

    REQUIRE((a.context.rsp & 0xfu) == 8u);

    REQUIRE(mcsos_sched_enqueue(&sched, &a) == MCSOS_SCHED_OK);
    REQUIRE(mcsos_sched_enqueue(&sched, &b) == MCSOS_SCHED_OK);
    REQUIRE(mcsos_sched_ready_count(&sched) == 2u);
    REQUIRE(mcsos_sched_validate(&sched)    == MCSOS_SCHED_OK);

    REQUIRE(mcsos_sched_yield(&sched)       == MCSOS_SCHED_OK);
    REQUIRE(sched.current                   == &a);
    REQUIRE(a.state                         == MCSOS_THREAD_RUNNING);
    REQUIRE(mcsos_sched_ready_count(&sched) == 1u);

    REQUIRE(mcsos_sched_tick(&sched) == MCSOS_SCHED_OK);
    REQUIRE(a.ticks                  == 1u);

    REQUIRE(mcsos_sched_yield(&sched) == MCSOS_SCHED_OK);
    REQUIRE(sched.current             == &b);

    REQUIRE(mcsos_sched_yield(&sched) == MCSOS_SCHED_OK);
    REQUIRE(sched.current             == &a);

    REQUIRE(sched.context_switches == 3u);
    REQUIRE(mcsos_sched_validate(&sched) == MCSOS_SCHED_OK);

    puts("M9 scheduler host unit test PASS");
    return 0;
}
