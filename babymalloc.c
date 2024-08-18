#include "babymalloc.h"

#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <string.h>

// #define DEBUG
#define WSIZE 8
#define ALIGN(size) (((size) + (WSIZE - 1)) & ~0x7)
#define GET_SIZE(p) (*(uint64_t *) (p) & ~0x7)
#define GET_USED(p) (*(uint64_t *) (p) & 0x1) // 0 for free, 1 for used
#define SET_USED(p) (*(uint64_t *) (p) |= 0x1)
#define SET_FREE(p) (*(uint64_t *) (p) &= ~0x1)

#ifdef DEBUG
#define debug_print(fmt, ...) printf(fmt, __VA_ARGS__)
#else
#define debug_print(fmt, ...)
#endif

void *heap_startp;
void *heap_endp;

static void *extend_heap(size_t size);
static void *find_fit(size_t size);
static void insert_block(void *blkp, size_t size);
static void coalesce(void *blkp);

void *babymalloc(size_t size) {
    size = ALIGN(size);
    if (!heap_startp) {
        heap_startp = extend_heap(size);
        heap_endp = heap_startp + size + 2 * WSIZE;
        debug_print("heap_startp: %p\n", heap_startp);
        debug_print("heap_startp value: %lu\n", *(uint64_t *) heap_startp);
        if (heap_startp == (void *) -1) {
            return NULL;
        }
    }

    void *blkp = find_fit(size);
    if (blkp) {
        insert_block(blkp, size);
        return blkp + WSIZE; // return the payload address (after the header)
    }

    blkp = extend_heap(size);
    if (!blkp) {
        return NULL;
    }

    heap_endp = blkp + size + 2 * WSIZE;
    insert_block(blkp, size);
    return blkp + WSIZE;
}

void babyfree(void *ptr) {
    void *blkp = ptr - WSIZE;
    SET_FREE(blkp);
    SET_FREE(blkp + GET_SIZE(blkp) + WSIZE);
    memset(ptr, 0, GET_SIZE(blkp));
    coalesce(blkp);
}

static void *extend_heap(size_t size) {
    void *blkp = sbrk(size + 2 * WSIZE);  // 2 words for header and footer
    if (blkp == (void *) -1) {
        return NULL;
    }

    *(uint64_t *) blkp = size;
    *(uint64_t *) (blkp + size + WSIZE) = size;
    SET_FREE(blkp);
    SET_FREE(blkp + size + WSIZE);
    return blkp;
}

// first-fit algorithm
static void *find_fit(size_t size) {
    void *blkp = heap_startp;

    debug_print("blkp: %p\n", blkp);
    debug_print("blkp value: %lu\n", *(uint64_t *) blkp);
    while (*(uint64_t *) blkp) {
        if (!GET_USED(blkp) && *(uint64_t *) blkp >= size) {
            return blkp;
        }
        blkp += (GET_SIZE(blkp) + 2 * WSIZE);
    }
    return NULL;
}

static void insert_block(void *blkp, size_t size) {
    SET_USED(blkp); // set header
    SET_USED(blkp + size + WSIZE); // set footer
}

static void coalesce(void *blkp) {
    uint64_t blk_size = GET_SIZE(blkp);
    void *prev_blkp = NULL;
    void *next_blkp = blkp + blk_size + 2 * WSIZE;;

    int coalesce_prev = 0;
    int coalesce_next = 0;

    // check if the block is the first block
    // if it is, we don't need to coalesce with the previous block
    if (blkp == heap_startp) {
        coalesce_prev = 0;
    } else {
        prev_blkp = blkp - (GET_SIZE(blkp - WSIZE) + 2 * WSIZE);
        if (!GET_USED(prev_blkp)) {
            coalesce_prev = 1;
        }
    }

    // check if the block is the last block
    // if it is, we don't need to coalesce with the next block
    if (next_blkp >= heap_endp) {
        coalesce_next = 0;
    } else if (!GET_USED(next_blkp)) {
        coalesce_next = 1;
    }

    if (coalesce_prev && coalesce_next) {
        blk_size += GET_SIZE(prev_blkp) + GET_SIZE(next_blkp) + 4 * WSIZE;
        void *next_blk_footer = next_blkp + GET_SIZE(next_blkp) + WSIZE;
        *(uint64_t *) prev_blkp = blk_size;
        *(uint64_t *) next_blk_footer = blk_size;
        SET_FREE(prev_blkp);
        SET_FREE(next_blk_footer);
    } else if (coalesce_prev) {
        blk_size += GET_SIZE(prev_blkp) + 2 * WSIZE;
        *(uint64_t *) prev_blkp = blk_size;
        void *blk_footer = blkp + blk_size + WSIZE;
        *(uint64_t *) blk_footer = blk_size;
        SET_FREE(prev_blkp);
        SET_FREE(blk_footer);
    } else if (coalesce_next) {
        blk_size += GET_SIZE(next_blkp) + 2 * WSIZE;
        void *next_blk_footer = next_blkp + GET_SIZE(next_blkp) + WSIZE;
        *(uint64_t *) blkp = blk_size;
        *(uint64_t *) next_blk_footer = blk_size;
        SET_FREE(blkp);
        SET_FREE(next_blk_footer);
    }

    if (prev_blkp == heap_startp) {
        *(uint64_t *) heap_startp = blk_size;
    }
}

void print_heap() {
    void *blkp = heap_startp;
    while (*(uint64_t *) blkp) {
        if (GET_USED(blkp)) {
            printf("Block at %p: size %lu %lu, used\n", blkp, GET_SIZE(blkp), GET_SIZE(blkp + GET_SIZE(blkp) + WSIZE));
        } else {
            printf("Block at %p: size %lu %lu, free\n", blkp, GET_SIZE(blkp), GET_SIZE(blkp + GET_SIZE(blkp) + WSIZE));
        }
        blkp += (GET_SIZE(blkp) + 2 * WSIZE);
    }
}
