#include "nova/storage_owner.h"
static volatile unsigned char owner;
static volatile unsigned char usb_ejected=1u;
bool nova_storage_claim(nova_storage_owner_t requested){
    if(requested==NOVA_STORAGE_OWNER_NONE)return false;
    unsigned char expected=(unsigned char)NOVA_STORAGE_OWNER_NONE;
    if(!__atomic_compare_exchange_n(&owner,&expected,(unsigned char)requested,false,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return false;
    if(requested==NOVA_STORAGE_OWNER_USB)__atomic_store_n(&usb_ejected,0u,__ATOMIC_RELEASE);
    return true;
}
bool nova_storage_release(nova_storage_owner_t current){
    if(current==NOVA_STORAGE_OWNER_NONE)return false;
    unsigned char expected=(unsigned char)current;
    if(!__atomic_compare_exchange_n(&owner,&expected,(unsigned char)NOVA_STORAGE_OWNER_NONE,false,__ATOMIC_ACQ_REL,__ATOMIC_ACQUIRE))return false;
    return true;
}
nova_storage_owner_t nova_storage_current_owner(void){
    return (nova_storage_owner_t)__atomic_load_n(&owner,__ATOMIC_ACQUIRE);
}
bool nova_storage_usb_ejected(void){return __atomic_load_n(&usb_ejected,__ATOMIC_ACQUIRE)!=0u;}
void nova_storage_set_usb_ejected(bool ejected){__atomic_store_n(&usb_ejected,ejected?1u:0u,__ATOMIC_RELEASE);}
