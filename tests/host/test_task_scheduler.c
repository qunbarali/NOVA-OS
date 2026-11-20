#include "nova/task.h"

#include <assert.h>
#include <stdio.h>

static unsigned task_a_runs;
static unsigned task_b_runs;
static unsigned task_exit_runs;

static nova_task_result_t task_a(void *context) {
    (void)context;
    ++task_a_runs;
    if (task_a_runs == 1u) nova_task_sleep_current(10u);
    return NOVA_TASK_AGAIN;
}

static nova_task_result_t task_b(void *context) {
    (void)context;
    ++task_b_runs;
    return NOVA_TASK_AGAIN;
}

static nova_task_result_t task_exit(void *context) {
    (void)context;
    ++task_exit_runs;
    return NOVA_TASK_EXIT;
}

int main(void) {
    nova_task_init();

    nova_task_id_t a = nova_task_create("task-a", task_a, NULL);
    nova_task_id_t b = nova_task_create("task-b", task_b, NULL);
    nova_task_id_t exiting = nova_task_create("task-exit", task_exit, NULL);
    assert(a != NOVA_TASK_ID_INVALID);
    assert(b != NOVA_TASK_ID_INVALID);
    assert(exiting != NOVA_TASK_ID_INVALID);
    assert(nova_task_count() == 3u);

    nova_task_scheduler_poll(100u);
    assert(task_a_runs == 1u);
    assert(task_b_runs == 1u);
    assert(task_exit_runs == 1u);
    assert(nova_task_count() == 2u);
    assert(nova_task_dispatch_count() == 3u);

    /* Sleeping task must not run before its deadline. */
    nova_task_scheduler_poll(109u);
    assert(task_a_runs == 1u);
    assert(task_b_runs == 2u);

    /* It becomes runnable exactly at the wrap-safe deadline. */
    nova_task_scheduler_poll(110u);
    assert(task_a_runs == 2u);
    assert(task_b_runs == 3u);

    assert(nova_task_suspend(b));
    nova_task_scheduler_poll(111u);
    assert(task_b_runs == 3u);
    assert(nova_task_resume(b));
    nova_task_scheduler_poll(112u);
    assert(task_b_runs == 4u);

    assert(nova_task_terminate(a));
    assert(nova_task_terminate(b));
    assert(nova_task_count() == 0u);

    /* Deadline arithmetic must remain correct across uint32_t wraparound. */
    nova_task_init();
    task_a_runs = 0u;
    a = nova_task_create("wrap", task_a, NULL);
    assert(a != NOVA_TASK_ID_INVALID);
    nova_task_scheduler_poll(UINT32_MAX - 4u);
    assert(task_a_runs == 1u);
    nova_task_scheduler_poll(4u);
    assert(task_a_runs == 1u);
    nova_task_scheduler_poll(5u);
    assert(task_a_runs == 2u);

    puts("NOVA host scheduler tests passed.");
    return 0;
}
