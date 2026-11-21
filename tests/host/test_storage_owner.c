#include <assert.h>
#include "nova/storage_owner.h"
int main(void){
    assert(nova_storage_current_owner()==NOVA_STORAGE_OWNER_NONE);
    assert(!nova_storage_claim(NOVA_STORAGE_OWNER_NONE));
    assert(nova_storage_claim(NOVA_STORAGE_OWNER_KERNEL));
    assert(nova_storage_current_owner()==NOVA_STORAGE_OWNER_KERNEL);
    assert(!nova_storage_claim(NOVA_STORAGE_OWNER_USB));
    assert(!nova_storage_release(NOVA_STORAGE_OWNER_USB));
    assert(nova_storage_release(NOVA_STORAGE_OWNER_KERNEL));
    assert(nova_storage_claim(NOVA_STORAGE_OWNER_USB));
    assert(!nova_storage_usb_ejected());
    assert(!nova_storage_claim(NOVA_STORAGE_OWNER_KERNEL));
    nova_storage_set_usb_ejected(true);
    assert(nova_storage_usb_ejected());
    assert(nova_storage_release(NOVA_STORAGE_OWNER_USB));
    assert(nova_storage_current_owner()==NOVA_STORAGE_OWNER_NONE);
    return 0;
}
