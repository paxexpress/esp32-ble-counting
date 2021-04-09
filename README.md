 # ble-scan-comparison

This is some code to compare ble scanning performance of nimble vs bluedroid and also nimble with and without advertising simultaneously.
Recieved advertising packages are counted.
A MAC address filter can be applied.

## Setup

**NOTE:** The setup is kind of complex. I didn't want to change too much of the code (apart from some cleaning up) to make my results reproducible.
Some parts of the code could be optimized in the way they are configurable (e.g. mac filtering).

### NimBLE

The branch `nimble` is set up for ble scanning and advertising with nimble.
In `src/gap.c` you can tweak the setup:

* Enable/disable advertising by commenting in/out `ble_gap_adv_start(...)` in line 154
    + The advertising interval can be modified by changing `itvl_min` and `itvl_max` in lines 21f.
    + `itvl_min` will be part of the advertising name, e.g. `G_09900`
* Enable/disable scanning by commenting in/out `ble_gap_disc(...)` in line 161.
    + The scan parameters can be modified by changing `ble_gap_disc_params` in lines 155-160.

### Bluedroid

The branch `bluedroid` is set up for ble scanning with bluedroid.
In `src/blescan.c` you can tweak the setup:

* The scan parameters can be modified by changing `ble_scan_params` in lines 85-97.
  Note that scan window and interval are set again/overwritten in lines 96f. after the struct is initialized.

### filtering mac addresses

The device counts advertising packages. You can filter by mac address so that only packages from particular mac address(es) will be counted or particular mac address(es) will be ignored.

To whitelist mac address(es), use `if (filter_mac(mac_addr))` (nimble, in `src/gap.c line 114`) or `if (filter_mac(p->scan_rst.bda))` (bluedroid, in `src/blescan.c line 65`).
To blacklist mac address(es), negate the condition (`if (!filter_mac)`).

Add mac addresses to black-/whitelist in `filter_mac(...)` function. Make shure to let the outer for loop of the `filter_mac()` function count to the number of black-/whitelisted mac addresses (e.g. `for size_t i = 0; i < 4; i++)` if you black-/whitelisted 4 mac addresses).
To show the correct filter message in terminal, comment in the first `printf(...)` statement of the `filter_mac()` function if you want to whitelist mac address(es) or comment in the second one if you want to blacklist mac address(es).



# Experimenting results

## Setup

* A esp32 is running GAP advertising with an advertising period of 1000-1100 ms.
* Two esp32 are running a BLE scanner. They are located on both sides next to the advertising esp32 in a distance of approx. 5 cm. They are not shielded from other RF. They only count advertising packages from the esp32 mentioned above.Other advertising packages from other devices will not be counted. However, other packages will be registered. Filtering takes place after they are registered. The number of counted advertising packages are compared for different scanning setups:
    + a) esp32 scanner with advertising (period 100-200 ms) vs esp32 scanner with advertising (period 9900-10000 ms)
    + b) esp32 scanner with advertising (period 100-200 ms) vs esp32 scanner without advertising
    + c) esp32 scanner without advertising vs. `hcitool lescan` on linux machine
    + d) esp32 scanner (NimBLE) vs. esp32 scanner (Bluedroid)

## Results

### a)

In a scanning period of 30 minutes, there was no relevant difference between a scanner running advertisement with an advertising period of 100-200 ms (249 counts) vs a scanner running advertisement with an advertising period of 9900-10000 ms (255 counts).

### b)

In a scanning period of 15 minutes, there also was no relevant difference between a scanner running advertisement with an advertising period of 100-200 ms (125 counts) vs a scannier without advertising (124 counts).

### c)

In a scanning period of 10 minutes, an esp32 scanner without advertising counted 84 advertising packages while, `hcitool lescan` counted 567 advertising packages.

### d)

In a scanning period of 15 minutes, a bluedroid scanner registered 864 ble advertising packages while a nimble scanner only registered 128. Both scanners were configured with similar scan settings\*. However, nimble can be tewaked to scan faster. With these tewaked settings, simultaneously advertising seems to make a little difference. In a 15 minute period of scanning an esp32 advertiser with an advertising period of 100-200 ms, a nimble scanner without advertising counted 4916 advertising packages while a nimble scanner with an advertising period of 100-200 ms counted 4644 advertising packages.<br>
A nimble scanner with an advertising period of 100-200 ms vs one with 9900-10000 ms counted 3149 vs 3250 advertising packages within approx. 10 minutes.<br>
A nimble scanner with an advertising period of 9900-10000 ms vs one without advertising counted 3264 vs 3305 advertising packages within 10 minutes.

\*similar scan settings:<br>
nimble: `filter_duplicates = 0; p ssive = 1; itvl = 10 * 10 / 0.625; window = 10 / 0.625; filter_policy = 0; limited = 0;`<br>
bluedroid: `scan_type = BLE_SCAN_TYPE_PASSIVE, scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL, scan_duplicate = BLE_SCAN_DUPLICATE_DISABLE, scan_interval = 10 * 10 / 0.625; scan_window = 10 / 0.625`;<br>
Note that scan interval and scan window have to be given in 0.625 ms units in nimble and bluedroid.

## Notes

* I noticed that the number of counted packages different esp32 boards (same hardware) scanning in in parallel (no filter except from filtering each other), drifted away from each other on some boards.
