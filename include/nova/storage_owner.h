#ifndef NOVA_STORAGE_OWNER_H
#define NOVA_STORAGE_OWNER_H
#include <stdbool.h>
typedef enum { NOVA_STORAGE_OWNER_NONE=0, NOVA_STORAGE_OWNER_KERNEL=1, NOVA_STORAGE_OWNER_USB=2 } nova_storage_owner_t;
/* Single-writer ownership gate shared by filesystem and USB MSC. */
bool nova_storage_claim(nova_storage_owner_t owner);
bool nova_storage_release(nova_storage_owner_t owner);
nova_storage_owner_t nova_storage_current_owner(void);
bool nova_storage_usb_ejected(void);
void nova_storage_set_usb_ejected(bool ejected);
#endif
