#ifndef ARENA_H__
#define ARENA_H__

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <unistd.h>

#define ARN_ERRNO int

// ==== ARNEA POINTER ====

#define ARN_POINTER_SZ 16 // (Bytes)
typedef struct
{
    void *addr;
    // bit map: high [ 7 6 5 4 3 2 1 0 ] low
    // 0: escaped from current stack frame
    // 1: escaped from current stack
    // 7: escaped from current thread
    // others: reserved
    uint64_t escape_flags : 8;
    uint64_t size : 56;
} ArenaPointer;

#define ARN_WRAP_POINTER(raw, sz)                                              \
    ((ArenaPointer){                                                           \
        .addr = (void *)(raw),                                                 \
        .escape_flags = 0,                                                     \
        .size = sz,                                                            \
    })

#define ARN_UNWRAP_POINTER(wrapped, raw, sz)                                   \
    do {                                                                       \
        ArenaPointer ap = (wrapped);                                           \
        raw = ap.addr;                                                         \
        sz = ap.size;                                                          \
    } while (0)

typedef enum {
    ARN_ESC_FRM = ((uint64_t)(1 << 0)),
    ARN_ESC_STK = ((uint64_t)(1 << 1)),
    ARN_ESC_THD = ((uint64_t)(1 << 7)),
} ArenaEscapeFlag;

#define ARN_FLIP_ESCAPE_FLAGS(wrapped, flags)                                  \
    do {                                                                       \
        ArenaPointer *pwrapped = &(wrapped);                                   \
        uint64_t mask = (flags);                                               \
        pwrapped->escape_flags ^= mask;                                        \
    } while (0)

/* Check if all flags specified are set. */
static inline bool arena_escaped(ArenaPointer *wrapped, uint64_t flags);

// ==== ARNEA POINTER ====

// ==== ARENA LAYOUT ====

static inline ARN_ERRNO arena_name_memory_space(void *start, size_t length,
                                                const char *name);

#define ARN_MEMORY_SPACE_NAME_CAP 80
#define ARN_NAME_MEMORY_SPACE(start, length, arena_errno, format, ...)         \
    do {                                                                       \
        char name[ARN_MEMORY_SPACE_NAME_CAP];                                  \
        snprintf(name, ARN_MEMORY_SPACE_NAME_CAP, format, ##__VA_ARGS__);      \
        arena_errno = arena_name_memory_space(start, length, name);            \
    } while (0)

#define ARN_MAXIMUM_THD_NUM                                                    \
    ((size_t)1 << 12) // 4K (maximum number of arena threads)

#define ARN_MEMORY_SZ ((size_t)1 << 37) // 128G (Bytes)

#define ARN_STK_SZ ((size_t)1 << 34) // 16G (Bytes)

// Space for each thread to hold its arena pointer related data.
#define ARN_THD_STK_SZ (ARN_STK_SZ / ARN_MAXIMUM_THD_NUM) // 4M (Bytes)

#define ARN_THD_STK_HEADER_SZ ((size_t)64) // (Bytes)

#define ARN_THD_STK_CAP (ARN_THD_STK_SZ - ARN_THD_STK_HEADER_SZ)

// Space for each thread to hold its thread pointers.
#define ARN_THD_STK_MAXIMUM_THD_POINTER_SZ (ARN_THD_STK_CAP >> 2)
#define ARN_THD_STK_MAXIMUM_THD_POINTER_NUM                                    \
    (ARN_THD_STK_MAXIMUM_THD_POINTER_SZ / ARN_POINTER_SZ)

// Space for each thread to hold its local pointers.
#define ARN_THD_STK_MAXIMUM_LOC_POINTER_SZ                                     \
    (ARN_THD_STK_CAP - ARN_THD_STK_MAXIMUM_THD_POINTER_SZ)
#define ARN_THD_STK_MAXIMUM_LOC_POINTER_NUM                                    \
    (ARN_THD_STK_MAXIMUM_LOC_POINTER_SZ / ARN_POINTER_SZ)

// ==== ARENA LAYOUT ====

// ==== IMPLEMENTATION ====

static inline bool arena_escaped(ArenaPointer *wrapped, uint64_t flags)
{
    return !((wrapped->escape_flags & flags) ^ flags);
}

static inline ARN_ERRNO arena_name_memory_space(void *start, size_t length,
                                                const char *name)
{
    assert(start);

    size_t pgsz = sysconf(_SC_PAGESIZE);
    if (length % pgsz != 0) {
        return EPERM;
    }

    return prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, (unsigned long)start,
                 (unsigned long)length, name) >= 0
               ? 0
               : errno;
}

#endif // ARENA_H__
