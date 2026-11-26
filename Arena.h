#ifndef ARENA_H__
#define ARENA_H__

#include <assert.h>
#include <bits/pthreadtypes.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>

#define ARN_ERRNO int

typedef atomic_uint_fast8_t arena_atomic_bitmap_t;
typedef uint_fast8_t arena_bitmap_t;
#define ARN_ATOMIC_BITMAP_SZ_IN_BYTE (sizeof(arena_atomic_bitmap_t))
#define ARN_ATOMIC_BITMAP_SZ_IN_BIT ((sizeof(arena_atomic_bitmap_t)) << 3)

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
        snprintf(name, ARN_MEMORY_SPACE_NAME_CAP,                              \
                 format __VA_OPT__(, ) __VA_ARGS__);                           \
        arena_errno = arena_name_memory_space(start, length, name);            \
    } while (0)

#define ARN_MAXIMUM_THD_NUM                                                    \
    (((size_t)1 << 12) -                                                       \
     1) // 4K - 1 (maximum number of arena threads, tid 0 reserved)

#define ARN_MEMORY_SZ ((size_t)1 << 37) // 128G (Bytes)

#define ARN_STK_SZ ((size_t)1 << 34) // 16G (Bytes)

// Space for each thread to hold its arena pointer related data.
#define ARN_THD_STK_SZ (ARN_STK_SZ / (ARN_MAXIMUM_THD_NUM + 1)) // 4M (Bytes)

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

extern void *arena_memory;

static inline void arena_format_memory(void);

#define ARN_LOCATE_STK(tid)                                                    \
    ((0 < tid && tid < (int)ARN_MAXIMUM_THD_NUM + 1)                           \
         ? &((ArenaStack *)arena_memory)[tid]                                  \
         : NULL)

// ==== ARENA LAYOUT ====

// ==== ARENA THREADS ====

#define ARN_TID_BITMAP_NUM                                                     \
    ((ARN_MAXIMUM_THD_NUM + 1) / ARN_ATOMIC_BITMAP_SZ_IN_BIT)

// A 1 at index 0, others are 0s.
extern arena_atomic_bitmap_t arena_tids[ARN_TID_BITMAP_NUM + 1];

/* Find an available thread id, if it does not exist, return -1. */
static inline int arena_next_tid(void);
/* Free an occupied thread id, ignore illegal specify. */
static inline void arena_free_tid(int tid);

extern pthread_once_t arena_thread_once;
extern pthread_key_t arena_thread_key;

/* Call arena to manage heap memory allocated in this posix threas, return -1
 * when initialization failed, stack frame on success.  */
static inline int arena_thread_init(void);

// ==== ARNEA THREADS ====

// ==== ARENA STACK ====

typedef struct
{
    size_t lp_offset;
    size_t tp_offset;
    int tid;
} ArenaStackHeader;

typedef struct
{
    ArenaStackHeader header;
    uint8_t reserved[ARN_THD_STK_HEADER_SZ - sizeof(ArenaStackHeader)];
    ArenaPointer local_pointers[ARN_THD_STK_MAXIMUM_LOC_POINTER_NUM];
    ArenaPointer thread_pointers[ARN_THD_STK_MAXIMUM_THD_POINTER_NUM];
} ArenaStack;

extern _Thread_local ArenaStack *p_arena_stack;

#define ARN_LP_OFFSET (p_arena_stack->header.lp_offset)
#define ARN_TP_OFFSET (p_arena_stack->header.tp_offset)
#define ARN_TID (p_arena_stack->header.tid)

#define ARN_LOC_POINTER(pos) (p_arena_stack->local_pointers[pos])
#define ARN_THR_POINTER(pos) (p_arena_stack->thread_pointers[pos])

#define ARN_STK_RESV (&p_arena_stack->local_pointers[ARN_LP_OFFSET++])

static inline void arena_format_stack(void);

static inline size_t arena_create_stack_frame(void);
static inline void arena_delete_stack_frame(size_t frame);

/* Register a raw pointer to stack of current thread, return EOVERFLOW when
 * stack is already full. */
static inline ARN_ERRNO arena_stack_accept(void *ptr, size_t size);

/* Find a registered pointer by raw pointer that is visiable for specified stack
 * frame, return NULL when pointer not found. */
static inline ArenaPointer *arena_stack_find(void *ptr, size_t frame);

// ==== ARENA STACK ====

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

static inline int arena_next_tid(void)
{
    const size_t MAXIMUM_TRY_TIMES = 3;

    for (size_t times_tried = 0; times_tried < MAXIMUM_TRY_TIMES;
         times_tried++) {
        for (int bi = 0; (size_t)bi < ARN_ATOMIC_BITMAP_SZ_IN_BIT; bi++) {
            for (int wi = 0; (size_t)wi < ARN_TID_BITMAP_NUM; wi++) {
                arena_bitmap_t bm = atomic_load(&arena_tids[wi]);
                assert(wi | bi | bm);

                while (!(bm & ((arena_bitmap_t)1 << bi))) {
                    if (atomic_compare_exchange_weak_explicit(
                            &arena_tids[wi], &bm,
                            bm | ((arena_bitmap_t)1 << bi),
                            memory_order_acq_rel, memory_order_acquire)) {
                        return wi * (int)ARN_ATOMIC_BITMAP_SZ_IN_BIT + bi;
                    }
                }
            }
        }
    }

    return -1;
}

static inline void arena_free_tid(int tid)
{
    if (tid <= 0 || ARN_MAXIMUM_THD_NUM <= (size_t)tid)
        return;

    int wi = tid / ARN_ATOMIC_BITMAP_SZ_IN_BIT,
        bi = tid % ARN_ATOMIC_BITMAP_SZ_IN_BIT;

    atomic_fetch_and(&arena_tids[wi], ~((arena_bitmap_t)1 << bi));
}

static inline ARN_ERRNO arena_stack_accept(void *ptr, size_t size)
{
    if (ARN_LP_OFFSET >= ARN_THD_STK_MAXIMUM_LOC_POINTER_NUM)
        return EOVERFLOW;

    ArenaPointer *wrapped = ARN_STK_RESV;

    wrapped->addr = ptr;
    wrapped->size = size;
    wrapped->escape_flags = 0;

    return 0;
}

static inline ArenaPointer *arena_stack_find(void *ptr, size_t frame)
{
    for (size_t pos = frame; pos < ARN_LP_OFFSET; pos++)
        if (ARN_LOC_POINTER(pos).addr == ptr)
            return &ARN_LOC_POINTER(pos);

    for (size_t pos = 0; pos < ARN_TP_OFFSET; pos++)
        if (ARN_THR_POINTER(pos).addr == ptr)
            return &ARN_LOC_POINTER(pos);

    return NULL;
}

static inline void arena_format_memory(void)
{
    arena_memory = mmap(NULL, ARN_MEMORY_SZ, PROT_READ | PROT_WRITE,
                        MAP_ANON | MAP_PRIVATE | MAP_NORESERVE, -1, 0);
    if (arena_memory == MAP_FAILED)
        abort(); // the program cannot run at all

    arena_format_stack();
}

static inline void arena_format_stack(void)
{
    static_assert(ARN_STK_SZ == sizeof(ArenaStack) * (ARN_MAXIMUM_THD_NUM + 1),
                  "bad aligned stack");
    size_t start = (size_t)arena_memory;

    int e = 0;
    for (size_t i = 0; i < ARN_MAXIMUM_THD_NUM; i++) {
        ARN_NAME_MEMORY_SPACE((void *)(start + i * ARN_THD_STK_SZ),
                              ARN_THD_STK_SZ, e, "__arena_thr%zu_stk__", i);
        if (e)
            abort(); // the program cannot run at all
    }
}

static inline void arena_thread_on_exit(void *unused)
{
    (void)unused;

    arena_free_tid(ARN_TID);

    // TODO: free thread pointers
    madvise(ARN_LOCATE_STK(ARN_TID), ARN_THD_STK_SZ, MADV_DONTNEED);
    p_arena_stack = NULL;
}

static inline void arena_init_thread_once(void)
{
    arena_format_memory();
    pthread_key_create(&arena_thread_key, arena_thread_on_exit);
}

static inline int arena_thread_init(void)
{
    if (!p_arena_stack) {
        pthread_once(&arena_thread_once, arena_init_thread_once);

        int tid = arena_next_tid();
        if (tid < 0)
            return -1;

        p_arena_stack = ARN_LOCATE_STK(tid);
        ARN_TID = tid;
        ARN_LP_OFFSET = 0;
        ARN_TP_OFFSET = 0;

        if (pthread_setspecific(arena_thread_key, p_arena_stack) != 0) {
            p_arena_stack = NULL;
            return -1;
        }
    }

    return ARN_LP_OFFSET;
}

static inline size_t arena_create_stack_frame(void) { return ARN_LP_OFFSET; }

static inline void arena_delete_stack_frame(size_t frame)
{
    if (frame >= ARN_THD_STK_MAXIMUM_LOC_POINTER_NUM)
        return;

    size_t prev_frame_end = frame;

    while (frame < ARN_THD_STK_MAXIMUM_LOC_POINTER_NUM) {
        ArenaPointer ptr = ARN_LOC_POINTER(frame);

        if (ptr.escape_flags & ARN_ESC_FRM) {
            ARN_LOC_POINTER(prev_frame_end++) = ptr;
            ARN_FLIP_ESCAPE_FLAGS(ARN_LOC_POINTER(prev_frame_end - 1),
                                  ARN_ESC_FRM);
        } else {
            // TODO: free memory on heap
        }

        frame++;
    }

    ARN_LP_OFFSET = prev_frame_end;
}

// ==== IMPLEMENTATION ====

#ifdef ARENA_IMPL__
arena_atomic_bitmap_t arena_tids[ARN_TID_BITMAP_NUM + 1] = {1};

pthread_once_t arena_thread_once = PTHREAD_ONCE_INIT;
pthread_key_t arena_thread_key;

_Thread_local ArenaStack *p_arena_stack = NULL;

void *arena_memory = NULL;
#endif

#define WITH_ARENA                                                             \
    for (int _current_frame = arena_thread_init();                             \
         0 <= _current_frame &&                                                \
         (size_t)_current_frame < ARN_THD_STK_MAXIMUM_LOC_POINTER_NUM;         \
         _current_frame = (arena_delete_stack_frame(_current_frame), -1))

#endif // ARENA_H__
