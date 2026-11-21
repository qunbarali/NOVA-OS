#ifndef NOVA_APP_MANAGER_H
#define NOVA_APP_MANAGER_H
#include <stdbool.h>
#include "nova/app_abi.h"
#include "nova/task.h"
typedef struct {
    nova_app_state_t state;
    nova_task_id_t task_id;
    int exit_code;
    char name[48];
} nova_app_manager_info_t;
/* Starts a trusted, statically linked ABI-v1 entry point as one scheduler task. */
int nova_app_launch(const char *name,nova_app_entry_v1_t entry);
bool nova_app_manager_request_stop(void);
void nova_app_manager_poll(void);
void nova_app_manager_snapshot(nova_app_manager_info_t *info);
#endif
