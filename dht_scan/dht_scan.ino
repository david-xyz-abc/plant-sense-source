// Tries a DHT11 on each safe NodeMCU pin to find where DAT is connected

#include <DHT.h>

const int PINS[] = {D1, D2, D5, D6, D7};
const char *NAMES[] = {"D1", "D2", "D5", "D6", "D7"};

void setup() {
  Serial.begin(115200);
}

void loop() {
  for (int i = 0; i < 5; i++) {
    DHT dht(PINS[i], DHT11);
    dht.begin();
    delay(2000);
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(t) || isnan(h)) Serial.printf("%s: no DHT11\n", NAMES[i]);
    else Serial.printf("%s: FOUND DHT11 %.1f C, %.0f %%RH\n", NAMES[i], t, h);
  }
  Serial.println("---");
}
