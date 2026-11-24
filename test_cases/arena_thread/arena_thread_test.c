#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define ARENA_IMPL__
#include "Arena.h"
#include "unit_test/unit_test_assert.h"

void test_arena_next_tid(void);
void test_arena_next_tid_multi_threads(void);

int main(void)
{
    test_arena_next_tid_multi_threads();
    test_arena_next_tid();
    exit(EXIT_SUCCESS);
}

void test_arena_next_tid(void)
{
    int tids[ARN_MAXIMUM_THD_NUM + 1] = {0};
    for (size_t i = 0; i < ARN_MAXIMUM_THD_NUM; i++) {
        int tid = arena_next_tid();
        ASSERT(0 < tid && (size_t)tid < ARN_MAXIMUM_THD_NUM + 1,
               "unexpected thread-id allocation failure under %zuth test, got "
               "tid %d",
               i, tid);

        ASSERT(!tids[tid], "duplicated thread-id %d under %zuth test", tid, i);
        tids[tid] = tid;
    }

    int tid = arena_next_tid();
    ASSERT(tid == -1, "unexpected thread-id allocation success, got tid %d",
           tid);

    arena_tids[0] = (arena_bitmap_t)1;
    for (size_t i = 1; i < ARN_TID_BITMAP_NUM; i++)
        arena_tids[i] = (arena_bitmap_t)0;
}

/* Test arena_next_tid, return 0 when success, -1 when thread-id allocation
 * failure, -2 when duplicated thread-id got, respectively. */
void *arena_routine(void *tids)
{
    int tid = arena_next_tid();
    ASSERT_ThrSafe(0 < tid && (size_t)tid < ARN_MAXIMUM_THD_NUM + 1, -1,
                   "unexpected thread-id allocation failure, got tid %d", tid);

    atomic_bool *ptid = (atomic_bool *)tids + tid;
    bool tid_occupied = atomic_load(ptid);

    ASSERT_ThrSafe(!tid_occupied, -2, "duplicated thread-id %d", tid);

    return 0;
}

void test_arena_next_tid_multi_threads(void)
{
    atomic_bool tids[ARN_MAXIMUM_THD_NUM + 1] = {false};
    pthread_t arena_threads[ARN_MAXIMUM_THD_NUM + 1];

    for (size_t i = 0; i < ARN_MAXIMUM_THD_NUM; i++) {
        pthread_create(&arena_threads[i], NULL, arena_routine, tids);
    }

    void *arena_routine_returned;
    for (size_t i = 0; i < ARN_MAXIMUM_THD_NUM; i++) {
        pthread_join(arena_threads[i], &arena_routine_returned);
        ASSERT(!arena_routine_returned,
               "thread-id allocation failed during %zuth thread", i);
    }

    arena_tids[0] = (arena_bitmap_t)1;
    for (size_t i = 1; i < ARN_TID_BITMAP_NUM; i++)
        arena_tids[i] = (arena_bitmap_t)0;
}
