#include "tusb.h"
#define NOVA_USB_VID 0xCafeu
#define NOVA_USB_PID 0x4011u
enum { ITF_CDC=0, ITF_CDC_DATA, ITF_MSC, ITF_TOTAL };
#define EP_CDC_NOTIF 0x81u
#define EP_CDC_OUT 0x02u
#define EP_CDC_IN 0x82u
#define EP_MSC_OUT 0x03u
#define EP_MSC_IN 0x83u
#define CONFIG_LENGTH (TUD_CONFIG_DESC_LEN+TUD_CDC_DESC_LEN+TUD_MSC_DESC_LEN)
static const tusb_desc_device_t device_descriptor={
    .bLength=sizeof(tusb_desc_device_t),.bDescriptorType=TUSB_DESC_DEVICE,.bcdUSB=0x0200,
    .bDeviceClass=TUSB_CLASS_MISC,.bDeviceSubClass=MISC_SUBCLASS_COMMON,.bDeviceProtocol=MISC_PROTOCOL_IAD,
    .bMaxPacketSize0=CFG_TUD_ENDPOINT0_SIZE,.idVendor=NOVA_USB_VID,.idProduct=NOVA_USB_PID,
    .bcdDevice=0x0100,.iManufacturer=1,.iProduct=2,.iSerialNumber=3,.bNumConfigurations=1
};
uint8_t const *tud_descriptor_device_cb(void){return (uint8_t const *)&device_descriptor;}
static const uint8_t configuration_descriptor[]={
    TUD_CONFIG_DESCRIPTOR(1,ITF_TOTAL,0,CONFIG_LENGTH,0x00,100),
    TUD_CDC_DESCRIPTOR(ITF_CDC,4,EP_CDC_NOTIF,8,EP_CDC_OUT,EP_CDC_IN,64),
    TUD_MSC_DESCRIPTOR(ITF_MSC,5,EP_MSC_OUT,EP_MSC_IN,64)
};
uint8_t const *tud_descriptor_configuration_cb(uint8_t index){(void)index;return configuration_descriptor;}
static const char *const strings[]={"","NOVA","NOVA OS USB","NOVA-ALPHA","CDC SERIAL","SD STORAGE"};
static uint16_t string_descriptor[32];
uint16_t const *tud_descriptor_string_cb(uint8_t index,uint16_t langid){
    (void)langid;
    if(index==0u){string_descriptor[0]=(uint16_t)((TUSB_DESC_STRING<<8)|4u);string_descriptor[1]=0x0409u;return string_descriptor;}
    if(index>=sizeof(strings)/sizeof(strings[0]))return NULL;
    const char *s=strings[index];uint8_t n=0u;while(s[n]&&n<31u){string_descriptor[1u+n]=(uint8_t)s[n];n++;}
    string_descriptor[0]=(uint16_t)((TUSB_DESC_STRING<<8)|(2u*n+2u));return string_descriptor;
}
