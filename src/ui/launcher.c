#include <stddef.h>
#include <string.h>
#include "pico/stdlib.h"
#include "nova/ui.h"
#include "nova/display.h"
#include "nova/font.h"
#include "nova/input.h"
#include "nova/kernel.h"
#include "nova/fs.h"
#include "nova/storage_owner.h"
#include "nova/battery.h"
#include <stdio.h>

#define COLOR_BG 0x0841u
#define COLOR_PANEL 0x18E3u
#define COLOR_TEXT 0xFFFFu
#define COLOR_ACCENT 0x07FFu
#define COLOR_MUTED 0xAD55u
static const char *const menu_items[]={"APPLICATIONS","FILES","USB STATUS","SYSTEM INFO"};
static uint8_t selected;
static char status_text[24]="SYSTEM READY";
static bool initialized;
static bool files_open;
static char root_listing[256];
static void render(void){
    uint16_t w=nova_display_width(),h=nova_display_height();
    if(!nova_display_is_ready())return;
    nova_display_fill(COLOR_BG);
    nova_display_fill_rect(0u,0u,w,18u,0x001Fu);
    nova_font_draw_text(4u,5u,"NOVA OS",COLOR_TEXT,0x001Fu);
    nova_battery_info_t battery;char battery_label[12]="BAT --";
    if(nova_battery_read(&battery)) (void)snprintf(battery_label,sizeof(battery_label),"BAT %u%%",(unsigned)battery.percent);
    nova_font_draw_text((uint16_t)(w>54u?w-54u:4u),5u,battery_label,COLOR_ACCENT,0x001Fu);
    nova_font_draw_text(5u,25u,"LAUNCHER",COLOR_ACCENT,COLOR_BG);
    uint16_t row_h=16u;
    for(uint8_t i=0u;i<4u;i++){
        uint16_t y=(uint16_t)(40u+(uint16_t)i*row_h);
        if(y+14u>=h-24u)break;
        nova_display_fill_rect(3u,y,(uint16_t)(w-6u),14u,i==selected?COLOR_ACCENT:COLOR_PANEL);
        nova_font_draw_text(7u,(uint16_t)(y+3u),menu_items[i],i==selected?COLOR_BG:COLOR_TEXT,i==selected?COLOR_ACCENT:COLOR_PANEL);
    }
    nova_display_fill_rect(0u,(uint16_t)(h-18u),w,18u,COLOR_PANEL);
    nova_font_draw_text(4u,(uint16_t)(h-13u),status_text,COLOR_MUTED,COLOR_PANEL);
}
void nova_ui_init(void){selected=0u;initialized=true;render();}
void nova_ui_set_status(const char *message){
    if(!message)return;
    size_t n=strlen(message);if(n>=sizeof(status_text))n=sizeof(status_text)-1u;
    memcpy(status_text,message,n);status_text[n]='\0';if(initialized)render();
}
void nova_ui_poll(void){
    if(!initialized)return;
    nova_input_poll();uint8_t events=nova_input_take_events();bool changed=false;
    if(events&NOVA_BUTTON_EVENT(NOVA_BUTTON_UP)){selected=(uint8_t)((selected+3u)%4u);changed=true;}
    if(events&NOVA_BUTTON_EVENT(NOVA_BUTTON_DOWN)){selected=(uint8_t)((selected+1u)%4u);changed=true;}
    if(events&NOVA_BUTTON_EVENT(NOVA_BUTTON_BACK)){strcpy(status_text,"SYSTEM READY");changed=true;}
    if(events&NOVA_BUTTON_EVENT(NOVA_BUTTON_SELECT)){
        static const char *const messages[]={"NO APPS INSTALLED","SD FILE BROWSER","USB NOT CONNECTED","NOVA SYSTEM INFO"};
        size_t n=strlen(messages[selected]);memcpy(status_text,messages[selected],n+1u);changed=true;
    }
    if(changed)render();
    (void)nova_uptime_ms();
}
