#ifndef NOVA_MEMORY_H
#define NOVA_MEMORY_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define NOVA_POOL_BLOCK_SIZE 32u
#define NOVA_POOL_BLOCK_COUNT 8u
void nova_memory_init(void);
void *nova_pool_alloc(void);
bool nova_pool_free(void *block);
size_t nova_pool_free_blocks(void);
size_t nova_pool_used_blocks(void);
#endif
