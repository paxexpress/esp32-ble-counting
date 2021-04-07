#include "gap.h"
#include "gatt.h"

#include <stdio.h>
#include <stdbool.h>
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "esp_system.h"

#define BT_BD_ADDR_HEX(addr)                                                   \
  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

uint8_t manufacturer_data[] = {0xFF, 0xFF, 0x47, 0x44, 0x4C, 0x59, 0x00, 0x01};

uint16_t itvl_min = 9900; // minimum advertising interval in ms
uint16_t itvl_max = 10000; // maximum advertising interval in ms
char device_name[8];

uint32_t counter = 0;

struct ble_gap_disc_params ble_gap_disc_params;

static int ble_gap_event_adv(struct ble_gap_event *event, void *arg)
{
    switch(event->type)
    {
        case BLE_GAP_EVENT_CONNECT:
            ESP_LOGI("GAP", "BLE_GAP_EVENT_CONNECT %s", event->connect.status == 0 ? "OK": "Failed");
            if(event->connect.status != 0) //advertise again if connection failed
            {
                ble_app_advertise();
            }
            connection_handle = event->connect.conn_handle;
            break;
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI("GAP", "BLE_GAP_EVENT_DISCONNECT");
            ble_app_advertise();
            break;
        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI("GAP", "BLE_GAP_EVENT_ADV_COMPLETE");
            ble_app_advertise();
            break;
        case BLE_GAP_EVENT_SUBSCRIBE:
            ESP_LOGI("GAP", "BLE_GAP_EVENT_SUBSCRIBE");
            if(event->subscribe.attr_handle == mqtt_in_message_chr_handle)
            {
                if(event->subscribe.cur_notify) {
                    printf("mqtt notification turned on\n");
                    mqtt_notify_active = 1;
                } else
                {
                    mqtt_notify_active = 0;
                    printf("mqtt notification turned off\n");
                }
            }
            break;
        default:
            break;
    }
    return 0;
}

int filter_mac(uint8_t *mac_addr)
{
    uint8_t filter_addresses[][6] = {
        {0x11, 0x11, 0x11, 0x11, 0x11, 0x11},
//      {0x22, 0x22, 0x22, 0x22, 0x22, 0x22},
//      {0x33, 0x33, 0x33, 0x33, 0x33, 0x33},
        };
    for (size_t i = 0; i < 1; i++) // let this for loop count to the number of mac addresses defined above, e.g. `for (size_t i = 0; i < 4; i++)` if you defined 4 mac addresses
    {
        for (size_t j = 0; j < 6; j++)
        {
            if (filter_addresses[i][j] != mac_addr[j])
            {
                printf("\e[1;36mmac address filtered: %02x:%02x:%02x:%02x:%02x:%02x\e[0m\n", BT_BD_ADDR_HEX(mac_addr)); // comment this line in if you use the filter as whitelist; use `if (filter_mac())` in your code
                break;
            }
            else if (j == 5)
            {
                //printf("\e[1;36mmac address filtered: %02x:%02x:%02x:%02x:%02x:%02x\e[0m\n", BT_BD_ADDR_HEX(mac_addr)); // comment this line in if you use the filter as blacklist; use `if (!filter_mac())`in your code
                return 1;
            }
        }
    }
    return 0;
}

static int ble_gap_event_disc(struct ble_gap_event *event, void *arg)
{
    uint8_t mac_addr[6];
    switch(event->type)
    {
        case BLE_GAP_EVENT_DISC_COMPLETE:
            // restart scan
            ble_gap_disc(ble_addr_type, BLE_HS_FOREVER, &ble_gap_disc_params, ble_gap_event_disc, NULL);
            break;

        case BLE_GAP_EVENT_DISC:
            // evaluate scan results
            memset(mac_addr, 0, 6);
            mac_addr[0] = event->disc.addr.val[5];
            mac_addr[1] = event->disc.addr.val[4];
            mac_addr[2] = event->disc.addr.val[3];
            mac_addr[3] = event->disc.addr.val[2];
            mac_addr[4] = event->disc.addr.val[1];
            mac_addr[5] = event->disc.addr.val[0];
            if (filter_mac(mac_addr)) {
                ESP_LOGI("counter", "%d", counter++);
                printf("Device address: %02x:%02x:%02x:%02x:%02x:%02x\n", BT_BD_ADDR_HEX(mac_addr));
                printf("RSSI          : %d\n", event->disc.rssi);
                printf("adv payload:");
                for (size_t i = 0; i < event->disc.length_data; i++)
                {
                    printf(" %.2x", event->disc.data[i]);
                }
                printf("\n");
            }
            break;
        default:
            break;
    }
    return 0;
}

void ble_app_advertise(void)
{
    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_DISC_LTD;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)ble_svc_gap_device_name(); // the device name was already set in init_gap(), so we cen get it back here
    fields.name_len = strlen(ble_svc_gap_device_name());
    fields.name_is_complete = 1;
    fields.mfg_data = manufacturer_data;
    fields.mfg_data_len = sizeof(manufacturer_data);
    ble_gap_adv_set_fields(&fields);


    struct ble_gap_adv_params adv_params;
    memset (&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    adv_params.itvl_min = itvl_min;
    adv_params.itvl_max = itvl_max;
    //ble_gap_adv_start(ble_addr_type, NULL, BLE_HS_FOREVER, &adv_params, ble_gap_event_adv, NULL); // comment this line in for advertising
    ble_gap_disc_params.filter_duplicates = 0;
    ble_gap_disc_params.passive = 1;
    ble_gap_disc_params.itvl = 10 * 10 / 0.625;
    ble_gap_disc_params.window = 10 / 0.625;
    ble_gap_disc_params.filter_policy = 0;
    ble_gap_disc_params.limited = 0;
    ble_gap_disc(ble_addr_type, BLE_HS_FOREVER, &ble_gap_disc_params, ble_gap_event_disc, NULL); // comment this line in for ble scanning
    uint8_t mac_address[6];
    esp_read_mac(mac_address, 2);
    printf("\e[1;36mMy own mac address is: %02x:%02x:%02x:%02x:%02x:%02x\e[0m\n", BT_BD_ADDR_HEX(mac_address));
    printf("\e[1;36mitvl_min: %d, itvl_max %d\e[0m\n", itvl_min, itvl_max);

}

void init_gap(void)
{
    sprintf(device_name, "G_%05d", itvl_min);
    printf("\e[1;36mdevice_name: %s\e[0m\n", device_name);
    ble_svc_gap_device_name_set(device_name);
    ble_svc_gap_init();
}