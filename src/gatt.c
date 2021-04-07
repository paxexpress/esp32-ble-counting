#include "gatt.h"
#include "gap.h"

#include <stdio.h>
#include <stdbool.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_nimble_hci.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gatt/ble_svc_gatt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_freertos_hooks.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include <stdbool.h>

#define TAG "gatt"

#define GUIDELY_PROVISIONING_SERVICE BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01)
#define PROVISIONING_STATE_CHR       BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x01)
#define DEVICE_ID_CHR                BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x02)
#define MAC_WIFI_STA_CHR             BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x03)
#define MAC_WIFI_SOFTAP_CHR          BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x04)
#define MAC_BLUETOOTH_CHR            BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x05)
#define MAC_ETHERNET_CHR             BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x06)
#define CURR_IP_CHR                  BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x07)
#define MQTT_CONN_STATUS_CHR         BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x08)
#define MQTT_URI                     BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x09)
#define MQTT_IN_MESSAGE_CHR          BLE_UUID128_DECLARE(0x47, 0x75, 0x69, 0x64, 0x65, 0x6C, 0x79, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x0A)
#define CLIENT_CHARACTERISTIC_CONFIGURATION BLE_UUID16_DECLARE(0x2902)

char recieved_mqtt_message[256];
int recieved_mqtt_message_len;
xSemaphoreHandle recieved_mqtt_message_mutex;

int provisioning_state_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    return 0;
}

int device_id_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    return 0;
}

int mac_wifi_sta_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t mac_address[6];
    esp_read_mac(mac_address, 0);
    os_mbuf_append(ctxt->om, mac_address, sizeof(mac_address));
    return 0;
}

int mac_wifi_softap_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t mac_address[6];
    esp_read_mac(mac_address, 1);
    os_mbuf_append(ctxt->om, mac_address, sizeof(mac_address));
    return 0;
}

int mac_bluetooth_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t mac_address[6];
    esp_read_mac(mac_address, 2);
    os_mbuf_append(ctxt->om, mac_address, sizeof(mac_address));
    return 0;
}

int mac_ethernet_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    uint8_t mac_address[6];
    esp_read_mac(mac_address, 3);
    os_mbuf_append(ctxt->om, mac_address, sizeof(mac_address));
    return 0;
}

int curr_ip_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
//    os_mbuf_append(ctxt->om, ip_str, strlen(ip_str));
    return 0;
}

int mqtt_conn_status_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
//    // alternative: send 0 and 1 instead of strings
//    if (mqtt_connection_status == 1) {
//        os_mbuf_append(ctxt->om, "connected", strlen("connected"));
//    } else {
//        os_mbuf_append(ctxt->om, "not connected", strlen("not connected"));
//    }
    return 0;
}

int mqtt_uri_chr_cb(uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
//    if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR)
//    {
//        nvs_handle handle;
//        size_t length;
//        nvs_open("prov_data", NVS_READONLY, &handle);
//        nvs_get_str(handle, "mqtt_uri", NULL, &length);
//        char mqtt_uri[length];
//        nvs_get_str(handle, "mqtt_uri", mqtt_uri, &length);
//        nvs_close(handle);
//        os_mbuf_append(ctxt->om, mqtt_uri, strlen(mqtt_uri));
//     } else if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR)
//     {
//        nvs_handle handle;
//        uint16_t length;
//        char raw_data[256];
//        memset(raw_data, 0, 256);
//        ble_hs_mbuf_to_flat(ctxt->om, raw_data, 512, &length);
//        length += 1;
//        char mqtt_uri[length];
//        memcpy(mqtt_uri, raw_data, length);
//        nvs_open("prov_data", NVS_READWRITE, &handle);
//        nvs_set_str(handle, "mqtt_uri", mqtt_uri);
//        nvs_commit(handle);
//        nvs_close(handle);
//
//        change_mqtt_uri(mqtt_uri);
//    }
    return 0;
}

int mqtt_in_message_chr_cb (uint16_t conn_handle, uint16_t attr_handle, struct ble_gatt_access_ctxt *ctxt, void *arg)
{
//    if(xSemaphoreTake(recieved_mqtt_message_mutex, 1000 / portTICK_PERIOD_MS))
//    {
//        os_mbuf_append(ctxt->om, recieved_mqtt_message, recieved_mqtt_message_len);
//        xSemaphoreGive(recieved_mqtt_message_mutex);
//    } else {
//        ESP_LOGE(TAG, "Read recieved mqtt message from buffer timed out.");
//    }
    return 0;
}

void send_mqtt_in_message_notification(int length, char *message)
{
//    if (mqtt_notify_active) {
//        if (xSemaphoreTake(recieved_mqtt_message_mutex, 1000 / portTICK_PERIOD_MS))
//        {
//            if (length > 256) {
//                ESP_LOGW(TAG, "Message recieved via MQTT is too long for buffer.");
//                recieved_mqtt_message_len = 256;
//            } else {
//                recieved_mqtt_message_len = length;
//            }
//            memcpy(recieved_mqtt_message, message, recieved_mqtt_message_len);
//            xSemaphoreGive(recieved_mqtt_message_mutex);
//            ble_gattc_notify(connection_handle, mqtt_in_message_chr_handle);
//        } else {
//            ESP_LOGE(TAG, "Write recieved mqtt message to buffer timed out.");
//        }
//    }
}

struct ble_gatt_svc_def gatt_svcs[] = {
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = GUIDELY_PROVISIONING_SERVICE,
        .characteristics = (struct ble_gatt_chr_def[]) {
            {
                .uuid = PROVISIONING_STATE_CHR,
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = provisioning_state_chr_cb
            },
            {
                .uuid = DEVICE_ID_CHR,
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = device_id_chr_cb
            },
            {
                .uuid = MAC_WIFI_STA_CHR,
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = mac_wifi_sta_chr_cb
            },
            {
                .uuid = MAC_WIFI_SOFTAP_CHR,
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = mac_wifi_softap_chr_cb
            },
            {
                .uuid = MAC_BLUETOOTH_CHR,
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = mac_bluetooth_chr_cb
            },
            {
                .uuid = MAC_ETHERNET_CHR,
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = mac_ethernet_chr_cb
            },
            {
                .uuid = CURR_IP_CHR,
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = curr_ip_chr_cb
            },
            {
                .uuid = MQTT_CONN_STATUS_CHR,
                .flags = BLE_GATT_CHR_F_READ,
                .access_cb = mqtt_conn_status_chr_cb
            },
            {
                .uuid = MQTT_URI,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                .access_cb = mqtt_uri_chr_cb
            },
            {
                .uuid = MQTT_IN_MESSAGE_CHR,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .val_handle = &mqtt_in_message_chr_handle,
                .access_cb = mqtt_in_message_chr_cb
            },
            {
                0
            }
        }
    },
    {
        0
    }
};

void init_gatt(void)
{
    recieved_mqtt_message_mutex = xSemaphoreCreateMutex();
    ble_svc_gatt_init();
    ble_gatts_count_cfg(gatt_svcs);
    ble_gatts_add_svcs(gatt_svcs);
}
