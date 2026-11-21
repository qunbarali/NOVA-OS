#ifndef NOVA_UI_H
#define NOVA_UI_H
#include <stdbool.h>
void nova_ui_init(void);
void nova_ui_poll(void);
void nova_ui_set_status(const char *message);
#endif
