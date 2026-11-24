#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "Arena.h"
#include "unit_test/unit_test_assert.h"

void test_arena_name_memory_space(void);

int main(void)
{
    test_arena_name_memory_space();
    exit(EXIT_SUCCESS);
}

void test_arena_name_memory_space(void)
{
    void *ptr = mmap(NULL, sysconf(_SC_PAGESIZE), PROT_READ | PROT_WRITE,
                     MAP_ANON | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
    assert(ptr);

    int e = 0;
    ARN_NAME_MEMORY_SPACE(ptr, sysconf(_SC_PAGESIZE), e, "name_mem_test");
    ASSERT(!e, "failed to name memory space: %s", strerror(e));

    ARN_NAME_MEMORY_SPACE(ptr, 1, e, "test");
    assert(e == EPERM);
}
