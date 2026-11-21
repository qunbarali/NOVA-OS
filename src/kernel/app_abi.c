#include "nova/app_abi.h"
#include "nova/kernel.h"
#include "nova/task.h"
#include "nova/display.h"
#include "nova/font.h"
#include "nova/fs.h"
static volatile unsigned char stop_flag;
static int app_display_text(uint16_t x,uint16_t y,const char *text,uint16_t color){
    if(!text||!nova_display_is_ready())return -1;
    nova_font_draw_text(x,y,text,color,0x0000u);return 0;
}
static int app_storage_read(const char *path,void *buffer,size_t capacity,size_t *read_bytes){
    return nova_fs_read_file(path,(uint8_t *)buffer,capacity,read_bytes);
}
static void app_yield(void){nova_task_yield();}
static const nova_app_api_v1_t api={
    .abi_version=NOVA_APP_ABI_VERSION,
    .struct_size=(uint32_t)sizeof(nova_app_api_v1_t),
    .uptime_ms=nova_uptime_ms,
    .display_text=app_display_text,
    .storage_mount=nova_fs_mount,
    .storage_unmount=nova_fs_unmount,
    .storage_read=app_storage_read,
    .storage_write_atomic=nova_fs_write_atomic,
    .yield=app_yield,
    .stop_requested=nova_app_stop_requested
};
const nova_app_api_v1_t *nova_app_api_v1(void){return &api;}
void nova_app_request_stop(void){__atomic_store_n(&stop_flag,1u,__ATOMIC_RELEASE);}
void nova_app_reset_stop(void){__atomic_store_n(&stop_flag,0u,__ATOMIC_RELEASE);}
bool nova_app_stop_requested(void){return __atomic_load_n(&stop_flag,__ATOMIC_ACQUIRE)!=0u;}
