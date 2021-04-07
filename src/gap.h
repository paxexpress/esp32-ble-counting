#ifndef __GAP_H_
#define __GAP_H_

#include <stdio.h>

uint8_t ble_addr_type;

void ble_app_advertise(void);
void init_gap(void);

uint16_t connection_handle;

#endif