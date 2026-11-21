#ifndef NOVA_APP_PACKAGE_H
#define NOVA_APP_PACKAGE_H
#include <stdint.h>
#define NOVA_APP_PACKAGE_HEADER_SIZE 128u
#define NOVA_APP_PACKAGE_MAX_SIZE (2u*1024u*1024u)
#define NOVA_APP_IMAGE_BIN 1u
#define NOVA_APP_IMAGE_ELF32 2u
typedef struct {
    uint8_t target;
    uint8_t image_kind;
    uint32_t abi_version;
    uint32_t load_address;
    uint32_t payload_length;
    char name[49];
    char version[17];
    uint8_t sha256[32];
} nova_app_package_info_t;
/* Requires a mounted NOVA FAT volume. Verifies header, target, size and payload digest. */
int nova_app_package_validate(const char *path,nova_app_package_info_t *info);
#endif
