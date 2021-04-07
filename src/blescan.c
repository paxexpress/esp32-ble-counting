#include "globals.h"
#include "blescan.h"

#define BT_BD_ADDR_HEX(addr)                                                   \
  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]

// local Tag for logging
static const char TAG[] = "bluetooth";
int initialized_ble = 0;
uint16_t used_blescantime = 0;

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

uint32_t counter = 0;

// using IRAM_ATTR here to speed up callback function
IRAM_ATTR void gap_callback_handler(esp_gap_ble_cb_event_t event,
                                    esp_ble_gap_cb_param_t *param) {

  esp_ble_gap_cb_param_t *p = (esp_ble_gap_cb_param_t *)param;

  switch (event) {
  case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT:
    // restart scan
    ESP_ERROR_CHECK(esp_ble_gap_start_scanning(used_blescantime));
    break;

  case ESP_GAP_BLE_SCAN_RESULT_EVT:
    // evaluate scan results
    if (p->scan_rst.search_evt ==
        ESP_GAP_SEARCH_INQ_CMPL_EVT) // Inquiry complete, scan is done
    {                                // restart scan
      ESP_ERROR_CHECK(esp_ble_gap_start_scanning(used_blescantime));
      return;
    }

    if (p->scan_rst.search_evt ==
        ESP_GAP_SEARCH_INQ_RES_EVT) // Inquiry result for a peer device
    {                               // evaluate sniffed packet
      printf("\e[1;36mFound: %02x:%02x:%02x:%02x:%02x:%02x\e[0m\n", BT_BD_ADDR_HEX(p->scan_rst.bda));
      if (filter_mac(p->scan_rst.bda))
      {
        ESP_LOGI("counter", "%d", counter+=1);
      }


    } // evaluate sniffed packet
    break;
  default:
    break;
  } // switch
} // gap_callback_handler

esp_err_t register_ble_callback() {
  ESP_LOGI(TAG, "Register GAP callback");

  // This function is called when gap event occurs, such as scan result.
  // register the scan callback function to the gap module
  ESP_ERROR_CHECK(esp_ble_gap_register_callback(&gap_callback_handler));

  static esp_ble_scan_params_t ble_scan_params = {
    .scan_type = BLE_SCAN_TYPE_PASSIVE,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,

    .scan_interval = 0,
    .scan_window = 0,

    .scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE // Report each packet - no de-duplication.
  };

  ble_scan_params.scan_interval = (uint16_t)(10 * 10 / 0.625); // Time = N * 0.625 msec
  ble_scan_params.scan_window = (uint16_t)(10 / 0.625); // Time = N * 0.625 msec

  ESP_LOGI(TAG, "Set GAP scan parameters");

  // This function is called to set scan parameters.
  ESP_ERROR_CHECK(esp_ble_gap_set_scan_params(&ble_scan_params));

  return ESP_OK;

} // register_ble_callback

void start_BLE_scan() {
  ESP_LOGI(TAG, "Initializing bluetooth scanner ...");

  // Initialize BT controller to allocate task and other resource.
  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_bt_controller_init(&bt_cfg));
  ESP_ERROR_CHECK(esp_bt_controller_enable(ESP_BT_MODE_BLE));

  ESP_ERROR_CHECK(esp_bluedroid_init());
  ESP_ERROR_CHECK(esp_bluedroid_enable());

  // Register callback function for capturing bluetooth packets
  ESP_ERROR_CHECK(register_ble_callback());

  ESP_LOGI(TAG, "Bluetooth scanner started");
  initialized_ble = 1;
} // start_BLEscan

void stop_BLE_scan(void) {
  if (initialized_ble) {
    ESP_LOGI(TAG, "Shutting down bluetooth scanner ...");
    ESP_ERROR_CHECK(esp_ble_gap_register_callback(NULL));
    ESP_ERROR_CHECK(esp_bluedroid_disable());
    ESP_ERROR_CHECK(esp_bluedroid_deinit());
    ESP_ERROR_CHECK(esp_bt_controller_disable());
    ESP_ERROR_CHECK(esp_bt_controller_deinit());
    ESP_ERROR_CHECK(esp_coex_preference_set(ESP_COEX_PREFER_WIFI));
    ESP_LOGI(TAG, "Bluetooth scanner stopped");
    initialized_ble = 0;
  }
} // stop_BLEscan
