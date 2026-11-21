#include <stdatomic.h>
#include "nova/app_abi.h"
#include "nova/kernel.h"
#include "nova/task.h"
#include "nova/display.h"
#include "nova/font.h"
#include "nova/fs.h"
static atomic_bool stop_flag;
static int app_display_text(uint16_t x,uint16_t y,const char *text,uint16_t color){
    if(!text||!nova_display_is_ready())return -1;
    nova_font_draw_text(x,y,text,color,0x0000u);return 0;
}
static void app_yield(void){nova_task_yield();}
static const nova_app_api_v1_t api={
    .abi_version=NOVA_APP_ABI_VERSION,
    .struct_size=sizeof(nova_app_api_v1_t),
    .uptime_ms=nova_uptime_ms,
    .display_text=app_display_text,
    .storage_mount=nova_fs_mount,
    .storage_unmount=nova_fs_unmount,
    .storage_read=nova_fs_read_file,
    .storage_write_atomic=nova_fs_write_atomic,
    .yield=app_yield,
    .stop_requested=nova_app_stop_requested
};
const nova_app_api_v1_t *nova_app_api_v1(void){return &api;}
void nova_app_request_stop(void){atomic_store_explicit(&stop_flag,true,memory_order_release);}
bool nova_app_stop_requested(void){return atomic_load_explicit(&stop_flag,memory_order_acquire);}
