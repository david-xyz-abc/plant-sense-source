// Plant health dashboard for NodeMCU ESP8266
// Soil moisture module: VCC -> 3V3, GND -> G, A0 -> A0, DO -> D0
// DHT11 (optional): VCC -> 3V3, DAT -> D7, GND -> G
// Open http://plant.local or the IP printed on serial

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <DHT.h>

// Put your WiFi details here (2.4 GHz network only),
// or in a local secrets.h (gitignored) with the same two defines
#if __has_include("secrets.h")
#include "secrets.h"
#else
#define WIFI_SSID "your-wifi-ssid"
#define WIFI_PASSWORD "your-wifi-password"
#endif

// Soil calibration: raw A0 reading in air (dry) and in water (wet)
const int SOIL_RAW_DRY = 1024;
const int SOIL_RAW_WET = 350;

const int SOIL_DIGITAL_PIN = D0;
const int DHT_PIN = D7;

DHT dht(DHT_PIN, DHT11);
ESP8266WebServer server(80);

int soilRaw = 0;
float airTemp = NAN, airHum = NAN;
unsigned long lastSoilRead = 0, lastDhtRead = 0;

const char PAGE[] PROGMEM = R"html(<!doctype html>
<html><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Plant Monitor</title>
<style>
:root{--bg:#f4f7f4;--card:#fff;--fg:#1c1e21;--muted:#6b7280;--line:#16a34a;--grid:#e5e7eb;--track:#e8ece8}
@media(prefers-color-scheme:dark){:root{--bg:#101410;--card:#1a1f1a;--fg:#e8eaed;--muted:#9aa0a6;--line:#4ade80;--grid:#2a302a;--track:#262c26}}
*{box-sizing:border-box}
body{margin:0;font-family:system-ui,sans-serif;background:var(--bg);color:var(--fg);padding:16px}
.wrap{max-width:560px;margin:0 auto;display:grid;gap:12px}
.card{background:var(--card);border-radius:14px;padding:18px;box-shadow:0 1px 3px rgba(0,0,0,.08)}
h1{font-size:20px;margin:4px 0}
h2{font-size:13px;margin:0 0 8px;color:var(--muted);font-weight:600;text-transform:uppercase;letter-spacing:.04em}
.big{font-size:56px;font-weight:700;line-height:1;font-variant-numeric:tabular-nums}
.big small{font-size:24px;color:var(--muted);font-weight:600}
.status{display:inline-block;margin-top:10px;padding:4px 10px;border-radius:999px;font-size:14px;font-weight:600}
.dry{background:#fde68a;color:#78350f}.ok{background:#bbf7d0;color:#14532d}.wet{background:#bfdbfe;color:#1e3a8a}
.meter{height:12px;border-radius:6px;background:var(--track);margin-top:14px;overflow:hidden}
.meter div{height:100%;width:0;background:var(--line);transition:width .4s}
.sub{color:var(--muted);font-size:13px;margin-top:8px;font-variant-numeric:tabular-nums}
.row{display:grid;grid-template-columns:1fr 1fr;gap:12px}
.val{font-size:32px;font-weight:700;font-variant-numeric:tabular-nums}
.na{color:var(--muted);font-size:15px;font-weight:500}
canvas{width:100%;height:140px;display:block}
.off{opacity:.45}
</style></head><body><div class="wrap" id="wrap">
<h1>&#127793; Plant Monitor</h1>
<div class="card">
 <h2>Soil moisture</h2>
 <div class="big" id="m">--<small>%</small></div>
 <span class="status" id="st">waiting</span>
 <div class="meter"><div id="bar"></div></div>
 <div class="sub" id="raw">raw --</div>
</div>
<div class="row">
 <div class="card"><h2>Air temp</h2><div class="val" id="t"><span class="na">--</span></div></div>
 <div class="card"><h2>Humidity</h2><div class="val" id="h"><span class="na">--</span></div></div>
</div>
<div class="card"><h2>Moisture, last 10 min</h2><canvas id="c"></canvas></div>
</div><script>
const hist=[],MAX=300,c=document.getElementById('c'),x=c.getContext('2d'),$=id=>document.getElementById(id);
function draw(){
 const w=c.width=c.clientWidth*devicePixelRatio,h=c.height=c.clientHeight*devicePixelRatio,s=getComputedStyle(document.body);
 x.strokeStyle=s.getPropertyValue('--grid');x.lineWidth=1;x.fillStyle=s.getPropertyValue('--muted');x.font=(11*devicePixelRatio)+'px system-ui';
 [0,50,100].forEach(p=>{const y=h-p/100*(h-2)-1;x.beginPath();x.moveTo(0,y);x.lineTo(w,y);x.stroke();x.fillText(p+'%',4,Math.max(12*devicePixelRatio,y-4))});
 if(hist.length<2)return;
 x.strokeStyle=s.getPropertyValue('--line');x.lineWidth=2*devicePixelRatio;x.beginPath();
 hist.forEach((p,i)=>{const X=i/(MAX-1)*w,Y=h-p/100*(h-2)-1;i?x.lineTo(X,Y):x.moveTo(X,Y)});x.stroke()}
async function tick(){
 try{const d=await (await fetch('/data')).json();$('wrap').classList.remove('off');
  $('m').innerHTML=d.moisture+'<small>%</small>';$('bar').style.width=d.moisture+'%';
  const st=d.moisture<30?['dry','Needs water']:d.moisture>80?['wet','Very wet']:['ok','Healthy'];
  $('st').className='status '+st[0];$('st').textContent=st[1];
  $('raw').textContent='raw '+d.raw+' · probe says '+(d.wet?'wet':'dry');
  $('t').innerHTML=d.temp==null?'<span class="na">sensor offline</span>':d.temp.toFixed(1)+'°C';
  $('h').innerHTML=d.hum==null?'<span class="na">sensor offline</span>':Math.round(d.hum)+'%';
  hist.push(d.moisture);if(hist.length>MAX)hist.shift();draw();
 }catch(e){$('wrap').classList.add('off');$('st').className='status';$('st').textContent='disconnected'}}
setInterval(tick,2000);tick();addEventListener('resize',draw);
</script></body></html>)html";

int moisturePercent() {
  long p = (long)(SOIL_RAW_DRY - soilRaw) * 100 / (SOIL_RAW_DRY - SOIL_RAW_WET);
  return constrain(p, 0, 100);
}

String jsonNum(float v, int decimals) {
  return isnan(v) ? String("null") : String(v, decimals);
}

void handleData() {
  String json = String("{\"moisture\":") + moisturePercent() +
                ",\"raw\":" + soilRaw +
                ",\"wet\":" + (digitalRead(SOIL_DIGITAL_PIN) == LOW ? "true" : "false") +
                ",\"temp\":" + jsonNum(airTemp, 1) +
                ",\"hum\":" + jsonNum(airHum, 0) + "}";
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  pinMode(SOIL_DIGITAL_PIN, INPUT);
  dht.begin();

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("\nConnecting to WiFi");
  unsigned long attemptStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    // some routers/hotspots reject the first attempt; start over if stuck
    if (millis() - attemptStart > 15000) {
      Serial.println("\nRetrying WiFi");
      WiFi.disconnect();
      WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
      attemptStart = millis();
    }
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:   Serial.println("network not found (2.4 GHz only)"); break;
      case WL_WRONG_PASSWORD:  Serial.println("wrong password"); break;
      default:                 Serial.print("."); break;
    }
  }
  Serial.printf("\nConnected! Open http://%s or http://plant.local\n",
                WiFi.localIP().toString().c_str());

  MDNS.begin("plant");
  MDNS.addService("http", "tcp", 80);
  server.on("/", [] { server.send_P(200, "text/html", PAGE); });
  server.on("/data", handleData);
  server.begin();
}

void loop() {
  server.handleClient();
  MDNS.update();
  if (millis() - lastSoilRead > 500) {
    lastSoilRead = millis();
    long sum = 0;
    for (int i = 0; i < 10; i++) sum += analogRead(A0);
    soilRaw = sum / 10;
  }
  if (millis() - lastDhtRead > 2000) {  // DHT11 needs ~2s between reads
    lastDhtRead = millis();
    airTemp = dht.readTemperature();
    airHum = dht.readHumidity();
  }
}
