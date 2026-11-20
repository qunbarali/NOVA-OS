#ifndef NOVA_IPC_H
#define NOVA_IPC_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define NOVA_QUEUE_CAPACITY 8u
#define NOVA_QUEUE_MESSAGE_SIZE 32u

typedef struct {
    uint8_t data[NOVA_QUEUE_CAPACITY][NOVA_QUEUE_MESSAGE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} nova_queue_t;

typedef struct { bool locked; } nova_mutex_t;
typedef struct { uint32_t flags; } nova_event_t;

void nova_queue_init(nova_queue_t *queue);
bool nova_queue_send(nova_queue_t *queue, const void *message, size_t length);
bool nova_queue_receive(nova_queue_t *queue, void *message, size_t capacity, size_t *received);
size_t nova_queue_count(const nova_queue_t *queue);
void nova_mutex_init(nova_mutex_t *mutex);
bool nova_mutex_try_lock(nova_mutex_t *mutex);
bool nova_mutex_unlock(nova_mutex_t *mutex);
void nova_event_init(nova_event_t *event);
void nova_event_set(nova_event_t *event, uint32_t flags);
uint32_t nova_event_get(const nova_event_t *event);
uint32_t nova_event_clear(nova_event_t *event, uint32_t flags);
#endif
