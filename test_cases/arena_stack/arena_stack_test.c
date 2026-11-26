#include <asm-generic/errno.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ARENA_IMPL__
#include "Arena.h"
#include "unit_test/unit_test_assert.h"

void test_arena_create_stack_frame_no_escape(int lp);
void test_arena_thread_on_exit(void);

void test_arena_create_stack_frame_escape_frame(int lp);

int main(void)
{
    test_arena_create_stack_frame_no_escape(0);
    test_arena_thread_on_exit();

    test_arena_create_stack_frame_escape_frame(0);
    test_arena_thread_on_exit();

    exit(EXIT_SUCCESS);
}

void test_arena_create_stack_frame_no_escape(int lp)
{
    if (!lp)
        ASSERT(!p_arena_stack, "unexpected stack address %p",
               (void *)p_arena_stack);

    WITH_ARENA
    {
        ASSERT(_current_frame >= 0 &&
                   (size_t)_current_frame < ARN_THD_STK_MAXIMUM_LOC_POINTER_NUM,
               "failed to create new frame, local pointer: %d", lp);

        ASSERT(_current_frame == lp, "unexpected frame %d, expected %d",
               _current_frame, lp);

        ASSERT(ARN_LP_OFFSET == (size_t)lp,
               "unexpected local pointer offset %zu, expected %d",
               ARN_LP_OFFSET, lp);

        const int DELTA = 7;
        int i;

        for (i = 0; i < DELTA; i++) {
            if (arena_stack_accept(NULL, 0) == EOVERFLOW) {
                break;
            }
        }
        if (i == DELTA)
            test_arena_create_stack_frame_no_escape(lp + DELTA);
        else
            return;
    }

    ASSERT(ARN_LP_OFFSET == (size_t)lp, "local pointer not freed");
}

void test_arena_thread_on_exit(void)
{
    arena_thread_on_exit(NULL);
    ASSERT(!p_arena_stack, "failed to clear thread resource");
}

void test_arena_create_stack_frame_escape_frame(int lp)
{
    const int DELTA = 7;
    int i;
    WITH_ARENA
    {
        for (i = 0; i < DELTA; i++) {
            if (arena_stack_accept(NULL, 0) == EOVERFLOW) {
                break;
            }
            ARN_FLIP_ESCAPE_FLAGS(ARN_LOC_POINTER(lp + i), ARN_ESC_FRM);
        }
        if (i == DELTA)
            test_arena_create_stack_frame_escape_frame(lp + DELTA);
    }

    ASSERT(ARN_LP_OFFSET == (size_t)lp + i, "escaped freed by mistake: %zu:%zu",
           ARN_LP_OFFSET + 1, (size_t)lp + i);
}
