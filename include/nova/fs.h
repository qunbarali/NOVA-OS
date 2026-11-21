#ifndef NOVA_FS_H
#define NOVA_FS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "nova/sd_spi.h"
int nova_fs_bind(nova_sd_card_t *card);
int nova_fs_mount(void);
int nova_fs_unmount(void);
int nova_fs_list_root(char *output,size_t capacity);
int nova_fs_read_file(const char *path,uint8_t *buffer,size_t capacity,size_t *read_bytes);
int nova_fs_write_atomic(const char *path,const void *data,size_t length);
int nova_fs_sync(void);
bool nova_fs_is_mounted(void);
#endif
