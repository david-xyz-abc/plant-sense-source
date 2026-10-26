# Plant monitor (NodeMCU ESP8266)

Soil moisture (and optional DHT11 air temperature/humidity) dashboard served over WiFi.

| Sketch | What it does |
|---|---|
| `plant_test/` | Prints soil moisture and DHT11 readings on serial |
| `dht_scan/` | Looks for a DHT11 on each safe pin (wiring check) |
| `plant_web/` | Live dashboard at `http://plant.local` |

## Wiring

Soil moisture module (4-pin side):

| Module | NodeMCU |
|---|---|
| VCC | `3V3` |
| GND | `G` |
| A0 | `A0` |
| DO | `D0` |

The module's 2-pin `+`/`-` side goes to the fork probe (either way round).

DHT11 (optional): VCC -> `3V3`, DAT -> `D7`, GND -> `G`.

Avoid `D3`, `D4`, `D8` (boot pins) and `SD0`-`SD3` (flash). Power sensors from `3V3`, not `Vin`.

## WiFi

Set your WiFi at the top of `plant_web/plant_web.ino` (replace `your-wifi-ssid` /
`your-wifi-password`), or in a local `plant_web/secrets.h` (gitignored).
The ESP8266 only supports **2.4 GHz** networks; set phone hotspots to 2.4 GHz.

## Calibration

`SOIL_RAW_DRY` / `SOIL_RAW_WET` in `plant_web.ino` map the raw A0 reading to 0-100%.
Read the raw value with the probe in air and in water, and put those in.

## Build / upload

```sh
arduino-cli lib install "DHT sensor library" "Adafruit Unified Sensor"
arduino-cli compile --fqbn esp8266:esp8266:nodemcuv2 plant_web
arduino-cli upload  --fqbn esp8266:esp8266:nodemcuv2 -p /dev/ttyUSB0 plant_web
```
