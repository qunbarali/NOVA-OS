#include "nova/memory.h"
#include "nova/smp_lock.h"
#include <stdint.h>
#include <string.h>

typedef union {
    max_align_t alignment;
    uint8_t bytes[NOVA_POOL_BLOCK_SIZE];
} pool_block_t;

static pool_block_t blocks[NOVA_POOL_BLOCK_COUNT];
static uint8_t allocated[NOVA_POOL_BLOCK_COUNT];

void nova_memory_init(void) {
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    memset(allocated, 0, sizeof(allocated));
    memset(blocks, 0, sizeof(blocks));
    nova_smp_lock_release(lock);
}

void *nova_pool_alloc(void) {
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    void *result = NULL;
    for (size_t i = 0u; i < NOVA_POOL_BLOCK_COUNT; ++i) {
        if (allocated[i] == 0u) {
            allocated[i] = 1u;
            memset(blocks[i].bytes, 0, sizeof(blocks[i].bytes));
            result = blocks[i].bytes;
            break;
        }
    }
    nova_smp_lock_release(lock);
    return result;
}

bool nova_pool_free(void *block) {
    if (block == NULL) return false;
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    bool freed = false;
    for (size_t i = 0u; i < NOVA_POOL_BLOCK_COUNT; ++i) {
        if (block == (void *)blocks[i].bytes && allocated[i] != 0u) {
            memset(blocks[i].bytes, 0, sizeof(blocks[i].bytes));
            allocated[i] = 0u;
            freed = true;
            break;
        }
    }
    nova_smp_lock_release(lock);
    return freed;
}

size_t nova_pool_free_blocks(void) {
    nova_smp_lock_state_t lock = nova_smp_lock_acquire();
    size_t count = 0u;
    for (size_t i = 0u; i < NOVA_POOL_BLOCK_COUNT; ++i)
        if (allocated[i] == 0u) ++count;
    nova_smp_lock_release(lock);
    return count;
}

size_t nova_pool_used_blocks(void) { return NOVA_POOL_BLOCK_COUNT - nova_pool_free_blocks(); }
