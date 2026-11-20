#include "nova/ipc.h"
#include <string.h>

void nova_queue_init(nova_queue_t *queue) {
    if (queue != NULL) memset(queue, 0, sizeof(*queue));
}

bool nova_queue_send(nova_queue_t *queue, const void *message, size_t length) {
    if (queue == NULL || message == NULL || length == 0u ||
        length > NOVA_QUEUE_MESSAGE_SIZE || queue->count >= NOVA_QUEUE_CAPACITY) return false;
    memset(queue->data[queue->tail], 0, NOVA_QUEUE_MESSAGE_SIZE);
    memcpy(queue->data[queue->tail], message, length);
    queue->tail = (uint8_t)((queue->tail + 1u) % NOVA_QUEUE_CAPACITY);
    ++queue->count;
    return true;
}

bool nova_queue_receive(nova_queue_t *queue, void *message, size_t capacity, size_t *received) {
    if (queue == NULL || message == NULL || queue->count == 0u ||
        capacity < NOVA_QUEUE_MESSAGE_SIZE) return false;
    memcpy(message, queue->data[queue->head], NOVA_QUEUE_MESSAGE_SIZE);
    if (received != NULL) *received = NOVA_QUEUE_MESSAGE_SIZE;
    memset(queue->data[queue->head], 0, NOVA_QUEUE_MESSAGE_SIZE);
    queue->head = (uint8_t)((queue->head + 1u) % NOVA_QUEUE_CAPACITY);
    --queue->count;
    return true;
}

size_t nova_queue_count(const nova_queue_t *queue) { return queue == NULL ? 0u : queue->count; }

void nova_mutex_init(nova_mutex_t *mutex) { if (mutex != NULL) mutex->locked = false; }
bool nova_mutex_try_lock(nova_mutex_t *mutex) {
    if (mutex == NULL || mutex->locked) return false;
    mutex->locked = true;
    return true;
}
bool nova_mutex_unlock(nova_mutex_t *mutex) {
    if (mutex == NULL || !mutex->locked) return false;
    mutex->locked = false;
    return true;
}
void nova_event_init(nova_event_t *event) { if (event != NULL) event->flags = 0u; }
void nova_event_set(nova_event_t *event, uint32_t flags) { if (event != NULL) event->flags |= flags; }
uint32_t nova_event_get(const nova_event_t *event) { return event == NULL ? 0u : event->flags; }
uint32_t nova_event_clear(nova_event_t *event, uint32_t flags) {
    if (event == NULL) return 0u;
    uint32_t previous = event->flags;
    event->flags &= ~flags;
    return previous;
}
