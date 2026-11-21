#include <string.h>
#include "nova/app_manager.h"
static struct {
    nova_app_state_t state;
    nova_task_id_t task_id;
    int exit_code;
    char name[48];
    nova_app_entry_v1_t entry;
} manager={.state=NOVA_APP_STOPPED,.task_id=NOVA_TASK_ID_INVALID,.exit_code=0,.entry=NULL};
static nova_task_result_t app_task(void *context){
    (void)context;
    if(!manager.entry){manager.state=NOVA_APP_FAILED;manager.exit_code=-1;manager.task_id=NOVA_TASK_ID_INVALID;return NOVA_TASK_EXIT;}
    if(nova_app_stop_requested()){manager.entry=NULL;manager.task_id=NOVA_TASK_ID_INVALID;manager.state=NOVA_APP_STOPPED;return NOVA_TASK_EXIT;}
    manager.state=NOVA_APP_RUNNING;
    int result=manager.entry(nova_app_api_v1());
    manager.state=NOVA_APP_STOPPING;
    manager.exit_code=result;
    manager.entry=NULL;
    manager.task_id=NOVA_TASK_ID_INVALID;
    manager.state=result<0?NOVA_APP_FAILED:NOVA_APP_STOPPED;
    return NOVA_TASK_EXIT;
}
int nova_app_launch(const char *name,nova_app_entry_v1_t entry){
    if(!name||!entry||(manager.state!=NOVA_APP_STOPPED&&manager.state!=NOVA_APP_FAILED))return -1;
    const nova_app_api_v1_t *api=nova_app_api_v1();
    if(!api||api->abi_version!=NOVA_APP_ABI_VERSION||api->struct_size<sizeof(nova_app_api_v1_t))return -2;
    size_t length=strlen(name);if(length==0u||length>=sizeof(manager.name))return -3;
    memcpy(manager.name,name,length+1u);
    char task_name[NOVA_TASK_NAME_MAX+1u];size_t task_length=length<NOVA_TASK_NAME_MAX?length:NOVA_TASK_NAME_MAX;
    memcpy(task_name,name,task_length);task_name[task_length]='\0';
    manager.state=NOVA_APP_VALIDATING;manager.exit_code=0;manager.entry=entry;nova_app_reset_stop();
    manager.state=NOVA_APP_READY;
    manager.task_id=nova_task_create(task_name,app_task,NULL);
    if(manager.task_id==NOVA_TASK_ID_INVALID){manager.entry=NULL;manager.state=NOVA_APP_FAILED;manager.exit_code=-4;return -4;}
    return 0;
}
bool nova_app_manager_request_stop(void){
    if(manager.state!=NOVA_APP_RUNNING&&manager.state!=NOVA_APP_READY)return false;
    manager.state=NOVA_APP_STOPPING;nova_app_request_stop();return true;
}
void nova_app_manager_poll(void){
    if(manager.task_id==NOVA_TASK_ID_INVALID)return;
    nova_task_info_t tasks[NOVA_TASK_MAX];size_t count=nova_task_snapshot(tasks,NOVA_TASK_MAX);
    for(size_t i=0u;i<count;i++){
        if(tasks[i].id==manager.task_id){
            if(tasks[i].state==NOVA_TASK_FAULTED){manager.state=NOVA_APP_FAILED;manager.exit_code=-5;manager.entry=NULL;}
            return;
        }
    }
    if(manager.state==NOVA_APP_RUNNING||manager.state==NOVA_APP_STOPPING){manager.state=NOVA_APP_FAILED;manager.exit_code=-6;manager.entry=NULL;manager.task_id=NOVA_TASK_ID_INVALID;}
}
void nova_app_manager_snapshot(nova_app_manager_info_t *info){
    if(!info)return;
    info->state=manager.state;info->task_id=manager.task_id;info->exit_code=manager.exit_code;
    memcpy(info->name,manager.name,sizeof(info->name));
}
