#include <string.h>
#include "pico.h"
#include "nova/app_package.h"
#include "nova/app_abi.h"
#include "nova/fs.h"
#include "nova/sha256.h"
static uint16_t read_le16(const uint8_t *p){return (uint16_t)((uint16_t)p[0]|((uint16_t)p[1]<<8));}
static uint32_t read_le32(const uint8_t *p){return (uint32_t)p[0]|((uint32_t)p[1]<<8)|((uint32_t)p[2]<<16)|((uint32_t)p[3]<<24);}
int nova_app_package_validate(const char *path,nova_app_package_info_t *info){
    if(!path||!info)return -1;
    uint32_t file_size=0u;
    if(nova_fs_file_size(path,&file_size)!=0||file_size<NOVA_APP_PACKAGE_HEADER_SIZE||file_size>NOVA_APP_PACKAGE_MAX_SIZE)return -2;
    uint8_t header[NOVA_APP_PACKAGE_HEADER_SIZE];
    if(nova_fs_read_range(path,0u,header,sizeof(header))!=0)return -3;
    static const uint8_t magic[8]={'N','O','V','A','A','P','P','1'};
    if(memcmp(header,magic,sizeof(magic))!=0||read_le16(header+8u)!=1u)return -4;
    uint8_t target=header[10u],kind=header[11u];
#if PICO_RP2350
    if(target!=2u)return -5;
#elif PICO_RP2040
    if(target!=1u)return -5;
#else
    return -5;
#endif
    uint32_t abi=read_le32(header+12u),payload=read_le32(header+16u),load=read_le32(header+20u);
    if(abi!=NOVA_APP_ABI_VERSION||!payload||payload!=file_size-NOVA_APP_PACKAGE_HEADER_SIZE||load==0u)return -6;
    if(kind!=NOVA_APP_IMAGE_BIN&&kind!=NOVA_APP_IMAGE_ELF32)return -7;
    for(size_t i=120u;i<sizeof(header);i++)if(header[i]!=0u)return -8;
    memcpy(info->name,header+24u,48u);info->name[48]='\0';
    memcpy(info->version,header+72u,16u);info->version[16]='\0';
    if(info->name[0]=='\0'||info->version[0]=='\0')return -9;
    if(kind==NOVA_APP_IMAGE_ELF32){
        uint8_t elf[52];
        if(payload<sizeof(elf)||nova_fs_read_range(path,NOVA_APP_PACKAGE_HEADER_SIZE,elf,sizeof(elf))!=0)return -10;
        if(elf[0]!=0x7Fu||elf[1]!='E'||elf[2]!='L'||elf[3]!='F'||elf[4]!=1u||elf[5]!=1u||
           read_le16(elf+16u)!=2u||read_le16(elf+18u)!=40u||read_le32(elf+24u)!=load||(load&1u)==0u)return -11;
    }
    uint8_t digest[32];
    if(nova_fs_sha256_range(path,NOVA_APP_PACKAGE_HEADER_SIZE,payload,digest)!=0)return -12;
    if(memcmp(digest,header+88u,sizeof(digest))!=0)return -13;
    info->target=target;info->image_kind=kind;info->abi_version=abi;info->load_address=load;info->payload_length=payload;
    memcpy(info->sha256,digest,sizeof(digest));return 0;
}
