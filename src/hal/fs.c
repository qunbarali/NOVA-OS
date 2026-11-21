#include <limits.h>
#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "pico/mutex.h"
#include "nova/fs.h"
#include "nova/storage_owner.h"
#include "nova/sha256.h"

void nova_diskio_bind(nova_sd_card_t *card);
static FATFS filesystem;
static mutex_t fs_mutex;
static bool mutex_ready;
static bool mounted;
static nova_sd_card_t *card;

static void fs_lock(void){if(mutex_ready)mutex_enter_blocking(&fs_mutex);}
static void fs_unlock(void){if(mutex_ready)mutex_exit(&fs_mutex);}
static int sync_locked(void){return disk_ioctl(0u,CTRL_SYNC,NULL)==RES_OK?0:-1;}

int nova_fs_bind(nova_sd_card_t *device){
    if(!device||!device->initialized||mounted)return -1;
    if(!mutex_ready){mutex_init(&fs_mutex);mutex_ready=true;}
    card=device;nova_diskio_bind(device);return 0;
}
int nova_fs_mount(void){
    fs_lock();
    if(mounted){fs_unlock();return 0;}
    if(!card||!card->initialized||!nova_storage_claim(NOVA_STORAGE_OWNER_KERNEL)){fs_unlock();return -1;}
    FRESULT result=f_mount(&filesystem,"0:",1u);
    if(result!=FR_OK){(void)nova_storage_release(NOVA_STORAGE_OWNER_KERNEL);fs_unlock();return -(int)result-10;}
    mounted=true;fs_unlock();return 0;
}
int nova_fs_unmount(void){
    fs_lock();
    if(!mounted){fs_unlock();return 0;}
    if(sync_locked()!=0){fs_unlock();return -3;}
    FRESULT result=f_mount(NULL,"0:",0u);
    if(result!=FR_OK){fs_unlock();return -(int)result-10;}
    mounted=false;
    bool released=nova_storage_release(NOVA_STORAGE_OWNER_KERNEL);
    fs_unlock();return released?0:-2;
}
bool nova_fs_is_mounted(void){return mounted;}
int nova_fs_sync(void){
    fs_lock();int result=mounted?sync_locked():-1;fs_unlock();return result;
}
int nova_fs_list_directory(const char *path,char *output,size_t capacity){
    if(!path||!output||capacity==0u)return -1;
    fs_lock();
    if(!mounted){fs_unlock();return -1;}
    DIR dir;FILINFO info;FRESULT r=f_opendir(&dir,path);
    if(r!=FR_OK){fs_unlock();return -(int)r-10;}
    size_t used=0u;output[0]='\0';
    while((r=f_readdir(&dir,&info))==FR_OK&&info.fname[0]){
        if(strcmp(info.fname,".")==0||strcmp(info.fname,"..")==0)continue;
        int n=snprintf(output+used,capacity-used,"%.255s%s\n",info.fname,(info.fattrib&AM_DIR)?"/":"");
        if(n<0){(void)f_closedir(&dir);fs_unlock();return -2;}
        if((size_t)n>=capacity-used){used=capacity-1u;output[used]='\0';break;}
        used+=(size_t)n;
    }
    (void)f_closedir(&dir);fs_unlock();return r==FR_OK?(int)used:-(int)r-10;
}
int nova_fs_list_root(char *output,size_t capacity){return nova_fs_list_directory("0:/",output,capacity);}
int nova_fs_read_file(const char *path,uint8_t *buffer,size_t capacity,size_t *read_bytes){
    if(!path||!buffer||!read_bytes)return -1;
    fs_lock();
    if(!mounted){fs_unlock();return -1;}
    FIL file;FRESULT r=f_open(&file,path,FA_READ);
    if(r!=FR_OK){fs_unlock();return -(int)r-10;}
    UINT amount=0u;r=f_read(&file,buffer,(UINT)(capacity>UINT_MAX?UINT_MAX:capacity),&amount);
    FRESULT close_result=f_close(&file);*read_bytes=(size_t)amount;
    fs_unlock();return r==FR_OK&&close_result==FR_OK?0:-2;
}
int nova_fs_read_range(const char *path,uint32_t offset,void *buffer,uint32_t length){
    if(!path||(!buffer&&length)||length>UINT_MAX)return -1;
    fs_lock();if(!mounted){fs_unlock();return -1;}
    FIL file;FRESULT r=f_open(&file,path,FA_READ);
    if(r!=FR_OK){fs_unlock();return -(int)r-10;}
    r=f_lseek(&file,(FSIZE_t)offset);UINT amount=0u;
    if(r==FR_OK)r=f_read(&file,buffer,(UINT)length,&amount);
    FRESULT close_result=f_close(&file);fs_unlock();
    return r==FR_OK&&close_result==FR_OK&&amount==length?0:-2;
}
int nova_fs_file_size(const char *path,uint32_t *size){
    if(!path||!size)return -1;
    fs_lock();if(!mounted){fs_unlock();return -1;}
    FILINFO info;FRESULT r=f_stat(path,&info);if(r==FR_OK)*size=(uint32_t)info.fsize;
    fs_unlock();return r==FR_OK?0:-(int)r-10;
}
int nova_fs_sha256_range(const char *path,uint32_t offset,uint32_t length,uint8_t digest[32]){
    if(!path||!digest)return -1;
    fs_lock();if(!mounted){fs_unlock();return -1;}
    FIL file;FRESULT r=f_open(&file,path,FA_READ);
    if(r!=FR_OK){fs_unlock();return -(int)r-10;}
    r=f_lseek(&file,(FSIZE_t)offset);
    nova_sha256_t hash; nova_sha256_init(&hash);
    uint8_t chunk[1024];uint32_t remaining=length;
    while(r==FR_OK&&remaining){
        UINT request=(UINT)(remaining>sizeof(chunk)?sizeof(chunk):remaining),amount=0u;
        r=f_read(&file,chunk,request,&amount);
        if(r!=FR_OK||amount!=request){r=FR_DISK_ERR;break;}
        nova_sha256_update(&hash,chunk,amount);remaining-=amount;
    }
    FRESULT close_result=f_close(&file);
    if(r==FR_OK&&close_result==FR_OK)nova_sha256_final(&hash,digest);
    fs_unlock();return r==FR_OK&&close_result==FR_OK?0:-2;
}
int nova_fs_write_atomic(const char *path,const void *data,size_t length){
    if(!path||!data||length>UINT32_MAX)return -1;
    fs_lock();
    int result=-1;
    if(!mounted)goto done;
    char temp[256],backup[256];
    int a=snprintf(temp,sizeof(temp),"%s.NVT",path),b=snprintf(backup,sizeof(backup),"%s.NVB",path);
    if(a<0||b<0||(size_t)a>=sizeof(temp)||(size_t)b>=sizeof(backup)){result=-2;goto done;}
    FILINFO recovery_info;
    if(f_stat(path,&recovery_info)!=FR_OK&&f_stat(backup,&recovery_info)==FR_OK)(void)f_rename(backup,path);
    (void)f_unlink(temp);
    FIL file;FRESULT r=f_open(&file,temp,FA_CREATE_ALWAYS|FA_WRITE);
    if(r!=FR_OK){result=-(int)r-10;goto done;}
    size_t offset=0u;
    while(offset<length){
        UINT chunk=(UINT)((length-offset)>4096u?4096u:(length-offset)),written=0u;
        r=f_write(&file,(const uint8_t *)data+offset,chunk,&written);
        if(r!=FR_OK||written!=chunk){(void)f_close(&file);(void)f_unlink(temp);result=-3;goto done;}
        offset+=written;
    }
    r=f_sync(&file);FRESULT close_result=f_close(&file);
    if(r!=FR_OK||close_result!=FR_OK){(void)f_unlink(temp);result=-4;goto done;}
    FILINFO info;bool had_old=f_stat(path,&info)==FR_OK;
    (void)f_unlink(backup);
    if(had_old&&f_rename(path,backup)!=FR_OK){(void)f_unlink(temp);result=-5;goto done;}
    if(f_rename(temp,path)!=FR_OK){if(had_old)(void)f_rename(backup,path);(void)f_unlink(temp);result=-6;goto done;}
    if(had_old)(void)f_unlink(backup);
    result=sync_locked();
done:
    fs_unlock();return result;
}
