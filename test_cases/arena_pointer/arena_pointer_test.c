#include <assert.h>
#include <bits/types/stack_t.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "Arena.h"

void test_arena_wrap_pointer(void);
void test_arena_unwrap_pointer(void);
void test_arena_flip_escape_flags(void);
void test_arena_escaped(void);

int main(void)
{
    test_arena_wrap_pointer();
    test_arena_unwrap_pointer();
    test_arena_flip_escape_flags();
    test_arena_escaped();
    exit(EXIT_SUCCESS);
}

void test_arena_wrap_pointer(void)
{
    void *addr = NULL;
    size_t size = (size_t)sysconf(_SC_PAGESIZE);

    ArenaPointer wrapped = ARN_WRAP_POINTER(addr, size);

    assert(wrapped.addr == addr);
    assert(wrapped.size == size);
}

void test_arena_unwrap_pointer(void)
{
    const void *addr_exp = NULL;
    const size_t size_exp = (size_t)sysconf(_SC_PAGESIZE);

    ArenaPointer wrapped = ARN_WRAP_POINTER(addr_exp, size_exp);

    void *addr_got;
    size_t size_got;

    ARN_UNWRAP_POINTER(wrapped, addr_got, size_got);

    assert(addr_exp == addr_got);
    assert(size_exp == size_got);
}

void test_arena_flip_escape_flags(void)
{
    ArenaPointer frame_escaper = ARN_WRAP_POINTER(NULL, sysconf(_SC_PAGESIZE));
    ARN_FLIP_ESCAPE_FLAGS(frame_escaper, ARN_ESC_FRM);
    assert(frame_escaper.escape_flags & ARN_ESC_FRM);

    ArenaPointer stack_escaper = ARN_WRAP_POINTER(NULL, sysconf(_SC_PAGESIZE));
    ARN_FLIP_ESCAPE_FLAGS(stack_escaper, ARN_ESC_STK);
    assert(stack_escaper.escape_flags & ARN_ESC_STK);

    ArenaPointer frame_thread_escaper =
        ARN_WRAP_POINTER(NULL, sysconf(_SC_PAGESIZE));
    ARN_FLIP_ESCAPE_FLAGS(frame_thread_escaper, ARN_ESC_FRM | ARN_ESC_THD);
    assert(frame_thread_escaper.escape_flags & (ARN_ESC_FRM | ARN_ESC_THD));

    ArenaPointer stack_thread_escaper =
        ARN_WRAP_POINTER(NULL, sysconf(_SC_PAGESIZE));
    ARN_FLIP_ESCAPE_FLAGS(stack_thread_escaper, ARN_ESC_STK | ARN_ESC_THD);
    assert(stack_thread_escaper.escape_flags & (ARN_ESC_STK | ARN_ESC_THD));
}

void test_arena_escaped(void)
{
    ArenaPointer frame_thread_escaper =
        ARN_WRAP_POINTER(NULL, sysconf(_SC_PAGESIZE));
    ARN_FLIP_ESCAPE_FLAGS(frame_thread_escaper, ARN_ESC_FRM | ARN_ESC_THD);

    assert(arena_escaped(&frame_thread_escaper, ARN_ESC_FRM | ARN_ESC_THD));

    ARN_FLIP_ESCAPE_FLAGS(frame_thread_escaper, ARN_ESC_FRM);
    assert(!arena_escaped(&frame_thread_escaper, ARN_ESC_FRM | ARN_ESC_THD) &&
           arena_escaped(&frame_thread_escaper, ARN_ESC_THD));
}
