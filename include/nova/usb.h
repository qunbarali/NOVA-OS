#ifndef NOVA_USB_H
#define NOVA_USB_H
#include "nova/sd_spi.h"
void nova_usb_bind_storage(nova_sd_card_t *card);
void nova_usb_init(void);
void nova_usb_task(void);
#endif
