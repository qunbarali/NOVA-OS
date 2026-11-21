#ifndef NOVA_SHA256_H
#define NOVA_SHA256_H
#include <stddef.h>
#include <stdint.h>
typedef struct { uint32_t state[8]; uint64_t bit_count; uint8_t block[64]; size_t used; } nova_sha256_t;
void nova_sha256_init(nova_sha256_t *ctx);
void nova_sha256_update(nova_sha256_t *ctx,const void *data,size_t length);
void nova_sha256_final(nova_sha256_t *ctx,uint8_t digest[32]);
#endif
