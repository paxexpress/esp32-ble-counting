#include "ble.h"
#include "gatt.h"
#include "gap.h"

#include <stdio.h>
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"


void ble_app_on_sync(void)
{
    ble_hs_id_infer_auto(0, &ble_addr_type);
    ble_app_advertise();
}

void host_task(void *param)
{
    nimble_port_run();
}

void init_ble(void)
{
    printf("init hci and controller 1\n");
    esp_nimble_hci_and_controller_init();
    printf("init hci and controller 2\n");
    esp_nimble_hci_and_controller_init();
    printf("nimble port init 1\n");
    nimble_port_init();
    printf("nimble port init 2\n");
    nimble_port_init();

    init_gap();
    init_gatt();
    
    ble_hs_cfg.sync_cb = ble_app_on_sync;
    printf("nimble_port_freertos_init 1\n");
    nimble_port_freertos_init(host_task);
    printf("nimble_port_freertos_init 2\n");
    nimble_port_freertos_init(host_task);
}