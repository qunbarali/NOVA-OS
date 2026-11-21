#include <string.h>
#include "ff.h"
#include "diskio.h"
#include "nova/sd_spi.h"
#include "nova/storage_owner.h"
static nova_sd_card_t *bound_card;
void nova_diskio_bind(nova_sd_card_t *card){bound_card=card;}
DSTATUS disk_initialize(BYTE pdrv){return (pdrv==0u&&bound_card&&bound_card->initialized&&nova_storage_current_owner()==NOVA_STORAGE_OWNER_KERNEL)?0u:STA_NOINIT;}
DSTATUS disk_status(BYTE pdrv){return disk_initialize(pdrv);}
DRESULT disk_read(BYTE pdrv,BYTE *buff,LBA_t sector,UINT count){
    if(pdrv!=0u||!buff||!count||disk_status(pdrv)&STA_NOINIT)return RES_NOTRDY;
    if(nova_sd_read(bound_card,(uint32_t)sector,buff,(uint32_t)count)!=0)return RES_ERROR;
    return RES_OK;
}
DRESULT disk_write(BYTE pdrv,const BYTE *buff,LBA_t sector,UINT count){
    if(pdrv!=0u||!buff||!count||disk_status(pdrv)&STA_NOINIT)return RES_NOTRDY;
    if(nova_sd_write(bound_card,(uint32_t)sector,buff,(uint32_t)count)!=0)return RES_ERROR;
    return RES_OK;
}
DRESULT disk_ioctl(BYTE pdrv,BYTE cmd,void *buff){
    if(pdrv!=0u||disk_status(pdrv)&STA_NOINIT)return RES_NOTRDY;
    if(cmd==CTRL_SYNC)return RES_OK;
    if(!buff)return RES_PARERR;
    if(cmd==GET_SECTOR_COUNT){*(LBA_t *)buff=(LBA_t)bound_card->sector_count;return RES_OK;}
    if(cmd==GET_SECTOR_SIZE){*(WORD *)buff=512u;return RES_OK;}
    if(cmd==GET_BLOCK_SIZE){*(DWORD *)buff=1u;return RES_OK;}
    return RES_PARERR;
}
