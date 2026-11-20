#include "nova/ipc.h"
#include "nova/smp_lock.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    nova_smp_lock_init();
    nova_queue_t queue;
    nova_queue_init(&queue);
    const char payload[] = "NOVA IPC";
    char received[16] = {0};
    size_t length = 0u;
    assert(nova_queue_count(&queue) == 0u);
    assert(nova_queue_send(&queue, payload, sizeof(payload)));
    assert(nova_queue_count(&queue) == 1u);
    assert(!nova_queue_receive(&queue, received, 2u, &length));
    assert(nova_queue_receive(&queue, received, sizeof(received), &length));
    assert(length == sizeof(payload));
    assert(strcmp(received, payload) == 0);
    assert(nova_queue_count(&queue) == 0u);

    for (unsigned i = 0u; i < NOVA_QUEUE_CAPACITY; ++i)
        assert(nova_queue_send(&queue, payload, sizeof(payload)));
    assert(!nova_queue_send(&queue, payload, sizeof(payload)));
    assert(nova_queue_count(&queue) == NOVA_QUEUE_CAPACITY);
    for (unsigned i = 0u; i < NOVA_QUEUE_CAPACITY; ++i)
        assert(nova_queue_receive(&queue, received, sizeof(received), NULL));

    nova_event_t event;
    nova_event_init(&event);
    nova_event_set(&event, 0x05u);
    assert(nova_event_get(&event) == 0x05u);
    assert(nova_event_clear(&event, 0x01u) == 0x05u);
    assert(nova_event_get(&event) == 0x04u);

    puts("NOVA host IPC tests passed.");
    return 0;
}
