#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "nova/fs.h"
#include "nova/storage_owner.h"
void nova_diskio_bind(nova_sd_card_t *card);
static FATFS filesystem;
static bool mounted;
static nova_sd_card_t *card;
int nova_fs_bind(nova_sd_card_t *device){if(!device||!device->initialized||mounted)return -1;card=device;nova_diskio_bind(device);return 0;}
int nova_fs_mount(void){
    if(mounted)return 0;
    if(!card||!card->initialized||!nova_storage_claim(NOVA_STORAGE_OWNER_KERNEL))return -1;
    FRESULT result=f_mount(&filesystem,"0:",1u);
    if(result!=FR_OK){(void)nova_storage_release(NOVA_STORAGE_OWNER_KERNEL);return -(int)result-10;}
    mounted=true;return 0;
}
int nova_fs_unmount(void){
    if(!mounted)return 0;
    FRESULT result=f_mount(NULL,"0:",0u);
    if(result!=FR_OK)return -(int)result-10;
    mounted=false;return nova_storage_release(NOVA_STORAGE_OWNER_KERNEL)?0:-2;
}
bool nova_fs_is_mounted(void){return mounted;}
int nova_fs_sync(void){if(!mounted)return -1;return disk_ioctl(0u,CTRL_SYNC,NULL)==RES_OK?0:-1;}
int nova_fs_list_root(char *output,size_t capacity){
    if(!mounted||!output||capacity==0u)return -1;
    DIR dir;FILINFO info;FRESULT r=f_opendir(&dir,"0:/");if(r!=FR_OK)return -(int)r-10;
    size_t used=0u;output[0]='\0';
    while((r=f_readdir(&dir,&info))==FR_OK&&info.fname[0]){
        int n=snprintf(output+used,capacity-used,"%s%s%s\n",(info.fattrib&AM_DIR)?"[":"",info.fname,(info.fattrib&AM_DIR)?"]":"");
        if(n<0||(size_t)n>=capacity-used){(void)f_closedir(&dir);return -2;}
        used+=(size_t)n;
    }
    (void)f_closedir(&dir);return r==FR_OK?(int)used:-(int)r-10;
}
int nova_fs_read_file(const char *path,uint8_t *buffer,size_t capacity,size_t *read_bytes){
    if(!mounted||!path||!buffer||!read_bytes)return -1;
    FIL file;FRESULT r=f_open(&file,path,FA_READ);if(r!=FR_OK)return -(int)r-10;
    UINT amount=0u;r=f_read(&file,buffer,(UINT)(capacity>UINT_MAX?UINT_MAX:capacity),&amount);
    FRESULT close_result=f_close(&file);*read_bytes=(size_t)amount;
    return r==FR_OK&&close_result==FR_OK?0:-2;
}
int nova_fs_write_atomic(const char *path,const void *data,size_t length){
    if(!mounted||!path||!data||length>UINT32_MAX)return -1;
    char temp[256],backup[256];
    int a=snprintf(temp,sizeof(temp),"%s.NVT",path),b=snprintf(backup,sizeof(backup),"%s.NVB",path);
    if(a<0||b<0||(size_t)a>=sizeof(temp)||(size_t)b>=sizeof(backup))return -2;
    FIL file;FRESULT r=f_open(&file,temp,FA_CREATE_ALWAYS|FA_WRITE);if(r!=FR_OK)return -(int)r-10;
    size_t offset=0u;
    while(offset<length){UINT chunk=(UINT)((length-offset)>4096u?4096u:(length-offset)),written=0u;r=f_write(&file,(const uint8_t *)data+offset,chunk,&written);if(r!=FR_OK||written!=chunk){(void)f_close(&file);(void)f_unlink(temp);return -3;}offset+=written;}
    r=f_sync(&file);FRESULT close_result=f_close(&file);if(r!=FR_OK||close_result!=FR_OK){(void)f_unlink(temp);return -4;}
    FILINFO info;bool had_old=f_stat(path,&info)==FR_OK;
    (void)f_unlink(backup);
    if(had_old&&f_rename(path,backup)!=FR_OK){(void)f_unlink(temp);return -5;}
    if(f_rename(temp,path)!=FR_OK){if(had_old)(void)f_rename(backup,path);(void)f_unlink(temp);return -6;}
    if(had_old)(void)f_unlink(backup);
    return nova_fs_sync();
}
