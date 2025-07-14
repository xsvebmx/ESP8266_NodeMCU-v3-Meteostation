#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>

#define EEPROM_SIZE 512
#define BME_ADDR 0x76

Adafruit_BME280 bme;
ESP8266WebServer server(80);
WiFiClient wifiClient;

struct Config {
  char mode; // 'L' - local, 'A' - API
  char apiUrl[100];
};

Config config;

void saveConfig() {
  EEPROM.put(0, config);
  EEPROM.commit();
}

void cleanALL() {

   Serial.println("EEPROM повреждена или не инициализирована. Сброс настроек...");
    
    // Очистим весь EEPROM (по желанию)
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0xFF);

    // Установим дефолтные значения
    config.mode = 'L'; // режим по умолчанию — локальный
    strcpy(config.apiUrl, "http://192.168.1.129:5000/data");

    saveConfig();
}

void loadConfig() {
  EEPROM.get(0, config);

  bool invalid = false;

  // Проверка допустимости режима
  if (config.mode != 'L' && config.mode != 'A') invalid = true;

  // Проверка длины строки
  if (config.apiUrl[0] == '\0' || strlen(config.apiUrl) > 99) invalid = true;

  // 🔧 СБРОС EEPROM ПРИ НЕВАЛИДНЫХ ДАННЫХ (можно отключить позже)
  if (invalid) {
    Serial.println("EEPROM повреждена или не инициализирована. Сброс настроек...");
    
    // Очистим весь EEPROM (по желанию)
    for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0xFF);

    // Установим дефолтные значения
    config.mode = 'L'; // режим по умолчанию — локальный
    strcpy(config.apiUrl, "http://192.168.1.129:5000/data");

    saveConfig();
  }
}

void handleRoot() {
  blinkLED();
  String html = R"rawliteral(
    <html><body style='color:white;background-color:gray;'>
    <h2>settings</h2>
    <form action="/save" method="post">
      Mode:<br>
      <input type="radio" name="mode" value="L" %LCHK%> local<br>
      <input type="radio" name="mode" value="A" %ACHK%> API<br><br>
      API URL:<br>
      <input type="text" name="api" value="%API%"><br><br>
      <input type="submit" value="save">
    </form>
    </body></html>
  )rawliteral";

  html.replace("%LCHK%", config.mode == 'L' ? "checked" : "");
  html.replace("%ACHK%", config.mode == 'A' ? "checked" : "");
  html.replace("%API%", config.apiUrl);

  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("mode")) config.mode = server.arg("mode")[0];
  if (server.hasArg("api")) server.arg("api").toCharArray(config.apiUrl, 100);
  saveConfig();
  server.send(200, "text/html", "<html><body>saved. rebooting.. <br> go to /data </body></html>");
  delay(1000);
  ESP.restart();
}

void setupWiFi() {
  blinkLED();
  WiFi.mode(WIFI_STA);
  WiFi.begin("wifi-example", "12345678");
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(".");
    blinkLED();
  }
}

void setupAP() {
  blinkLED();
  WiFi.mode(WIFI_AP);
  WiFi.softAP("NodeConfig", "12345678");
  Serial.println("AP Mode: NodeConfig (12345678)");
}

void sendToApi(float t, float h, float p, float mmHg, float altitude) {
  blinkLED();
  HTTPClient http;
  String payload = "{";
  payload += "\"temperature\":" + String(t, 2) + ",";
  payload += "\"humidity\":" + String(h, 2) + ",";
  payload += "\"pressure\":" + String(p, 2) + ",";
  payload += "\"pressure_mmHg\":" + String(mmHg, 2) + ",";
  payload += "\"altitude\":" + String(altitude, 2);
  payload += "}";
  http.begin(wifiClient, config.apiUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);
  http.end();
}

void handleDataPage() {
  blinkLED();
  float t = bme.readTemperature();
  float h = bme.readHumidity();
  float p = bme.readPressure() / 100.0;
  float mmHg = p * 0.75006;
  float altitude = bme.readAltitude(1013);

  String page = "<html><body>";
  page += "temperature: " + String(t, 2) + " C<br>";
  page += "humidity: " + String(h, 2) + " %<br>";
  page += "altitude: " + String(altitude, 2) + " m<br>";
  page += "pressure: " + String(mmHg, 2) + " mm.rt.st.<br>";
  page += "pressure_hpa: " + String(p, 2) + " hpa";
  page += "</body></html>";

  server.send(200, "text/html", page);
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();
  pinMode(LED_BUILTIN, OUTPUT);

  if (!bme.begin(BME_ADDR)) {
    Serial.println("BME280 not found!");
    while (1);
  }

  if (config.mode == 'L') {
    setupAP();
    server.on("/", handleRoot);
    server.on("/save", HTTP_POST, handleSave);
    server.on("/data", handleDataPage);
    server.begin();
  } else {
    setupWiFi();
  }
}

void blinkLED() {
  digitalWrite(LED_BUILTIN, LOW);
  delay(50);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  if (config.mode == 'L') {
    server.handleClient();
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      float t = bme.readTemperature();
      float h = bme.readHumidity();
      float p = bme.readPressure() / 100.0;
      float mmHg = p * 0.75006;
      float altitude = bme.readAltitude(1013);
      sendToApi(t, h, p, mmHg, altitude);
    }
    delay(10000);
  }
}
