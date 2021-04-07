#include <stdio.h>
#include "nvs_flash.h"

#include "ble.h"

#ifdef CONFIG_USE_GUI
#include "gui.h"
#endif

void app_main(void)
{
    nvs_flash_init();
    init_ble();
    // init_eth();
}

