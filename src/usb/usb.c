#include <string.h>
#include "tusb.h"
#include "pico/stdio.h"
#include "pico/stdio/driver.h"
#include "nova/usb.h"
#include "nova/storage_owner.h"
#include "nova/sd_spi.h"
static nova_sd_card_t *storage_card;
static bool usb_storage_owned;
static bool usb_ejected=true;
static void cdc_out(const char *buffer,int length){
    if(length<=0||!tud_cdc_connected())return;
    uint32_t n=tud_cdc_write(buffer,(uint32_t)length);(void)n;tud_cdc_write_flush();
}
static int cdc_in(char *buffer,int length){
    if(length<=0||!tud_cdc_available())return 0;
    return (int)tud_cdc_read(buffer,(uint32_t)length);
}
static stdio_driver_t nova_cdc_stdio={.out_chars=cdc_out,.out_flush=tud_cdc_write_flush,.in_chars=cdc_in};
void nova_usb_bind_storage(nova_sd_card_t *card){storage_card=card;}
void nova_usb_init(void){
    tusb_rhport_init_t init={.role=TUSB_ROLE_DEVICE,.speed=TUSB_SPEED_AUTO};
    (void)tusb_init(0u,&init);
    stdio_set_driver_enabled(&nova_cdc_stdio,true);
}
void nova_usb_task(void){tud_task();}
void tud_mount_cb(void){
    usb_storage_owned=storage_card&&storage_card->initialized&&nova_storage_claim(NOVA_STORAGE_OWNER_USB);
    usb_ejected=false;
}
void tud_umount_cb(void){
    if(usb_storage_owned){(void)nova_storage_release(NOVA_STORAGE_OWNER_USB);usb_storage_owned=false;}
    usb_ejected=true;
}
void tud_msc_inquiry_cb(uint8_t lun,uint8_t vendor[8],uint8_t product[16],uint8_t revision[4]){
    (void)lun;memcpy(vendor,"NOVA    ",8u);memcpy(product,"SD CARD         ",16u);memcpy(revision,"1.0 ",4u);
}
bool tud_msc_test_unit_ready_cb(uint8_t lun){
    (void)lun;
    if(!usb_storage_owned||!storage_card||!storage_card->initialized||usb_ejected){
        tud_msc_set_sense(0u,0x02u,0x3Au,0x00u);return false;
    }
    return true;
}
void tud_msc_capacity_cb(uint8_t lun,uint32_t *block_count,uint16_t *block_size){
    (void)lun;*block_count=storage_card?storage_card->sector_count:0u;*block_size=NOVA_SD_SECTOR_SIZE;
}
bool tud_msc_start_stop_cb(uint8_t lun,uint8_t power_condition,bool start,bool load_eject){
    (void)lun;(void)power_condition;
    if(load_eject)usb_ejected=!start;
    return true;
}
bool tud_msc_is_writable_cb(uint8_t lun){(void)lun;return usb_storage_owned&&!usb_ejected;}
int32_t tud_msc_read10_cb(uint8_t lun,uint32_t lba,uint32_t offset,void *buffer,uint32_t size){
    (void)lun;
    if(!tud_msc_test_unit_ready_cb(0u)||!buffer||offset!=0u||size==0u||size%NOVA_SD_SECTOR_SIZE)return -1;
    uint32_t count=size/NOVA_SD_SECTOR_SIZE;
    return nova_sd_read(storage_card,lba,(uint8_t *)buffer,count)==0?(int32_t)size:-1;
}
int32_t tud_msc_write10_cb(uint8_t lun,uint32_t lba,uint32_t offset,uint8_t *buffer,uint32_t size){
    (void)lun;
    if(!tud_msc_test_unit_ready_cb(0u)||!buffer||offset!=0u||size==0u||size%NOVA_SD_SECTOR_SIZE)return -1;
    uint32_t count=size/NOVA_SD_SECTOR_SIZE;
    return nova_sd_write(storage_card,lba,buffer,count)==0?(int32_t)size:-1;
}
void tud_msc_write10_complete_cb(uint8_t lun){(void)lun;/* SD driver waits for card-ready after every block. */}
