// Plant monitor sensor test for NodeMCU ESP8266
// Soil moisture module: VCC -> 3V3, GND -> G, A0 -> A0, DO -> D0
// DHT11: VCC -> 3V3, DAT -> D7, GND -> G
// Avoid D3, D4, D8 (boot pins) and SD0-SD3 (flash)

#include <DHT.h>

const int SOIL_DIGITAL_PIN = D0;
const int DHT_PIN = D7;

DHT dht(DHT_PIN, DHT11);

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_DIGITAL_PIN, INPUT);
  dht.begin();
}

void loop() {
  int soilRaw = analogRead(A0);  // 0-1023 for 0-3.3V, higher = drier
  int soilMv = soilRaw * 3300L / 1023;
  int soilDo = digitalRead(SOIL_DIGITAL_PIN);  // module LED threshold, LOW = wet
  float temp = dht.readTemperature();
  float hum = dht.readHumidity();

  Serial.printf("Soil: raw=%4d (%4d mV) DO=%s  |  ", soilRaw, soilMv, soilDo ? "dry" : "wet");
  if (isnan(temp) || isnan(hum)) Serial.println("DHT11: read failed");
  else Serial.printf("DHT11: %.1f C, %.0f %%RH\n", temp, hum);
  delay(2000);  // DHT11 needs ~1-2s between reads
}
