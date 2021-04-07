#ifndef __GATT_H_
#define __GATT_H_

#include <stdio.h>
#include <stdbool.h>

uint16_t mqtt_in_message_chr_handle;
bool mqtt_notify_active;

void init_gatt(void);
void send_mqtt_in_message_notification(int length, char *message);

#endif