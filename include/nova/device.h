#ifndef NOVA_DEVICE_H
#define NOVA_DEVICE_H
#include <stddef.h>
#include <stdint.h>
#define NOVA_DEVICE_MAX 8u
#define NOVA_DEVICE_NAME_MAX 12u
#define NOVA_DEVICE_CAP_READ  0x01u
#define NOVA_DEVICE_CAP_WRITE 0x02u
#define NOVA_DEVICE_CAP_CTRL  0x04u

typedef enum {
    NOVA_DEVICE_GPIO=1,
    NOVA_DEVICE_CONSOLE,
    NOVA_DEVICE_STORAGE,
    NOVA_DEVICE_DISPLAY,
    NOVA_DEVICE_USB,
    NOVA_DEVICE_SENSOR
} nova_device_class_t;

typedef struct {
    int (*read)(void *context, void *buffer, size_t capacity);
    int (*write)(void *context, const void *buffer, size_t length);
    int (*control)(void *context, uint32_t command, void *argument);
} nova_device_ops_t;

typedef struct {
    uint8_t id;
    char name[NOVA_DEVICE_NAME_MAX + 1u];
    nova_device_class_t device_class;
    uint32_t capabilities;
    const nova_device_ops_t *ops;
    void *context;
} nova_device_t;

void nova_device_manager_init(void);
int nova_device_register(const nova_device_t *device);
size_t nova_device_count(void);
size_t nova_device_snapshot(nova_device_t *out, size_t capacity);
int nova_device_read(const char *name, void *buffer, size_t capacity);
int nova_device_write(const char *name, const void *buffer, size_t length);
int nova_device_control(const char *name, uint32_t command, void *argument);
const char *nova_device_class_name(nova_device_class_t device_class);
#endif
