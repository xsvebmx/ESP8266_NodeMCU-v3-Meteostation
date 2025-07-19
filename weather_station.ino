#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <ESP8266HTTPClient.h>

#define EEPROM_SIZE 512
#define BME_ADDR 0x76

#define RESET_PIN D7          // GPIO13
#define CHANGE_RESULTS_PIN D6 // Пин для кнопки сброса данных

Adafruit_BME280 bme;
ESP8266WebServer server(80);
WiFiClient wifiClient;

struct Config {
  char mode; // 'L' - local, 'A' - API
  char apiUrl[100];
  char wifiSSID[100];
  char wifiPSWD[100];
  char apSSID[100];
  char apPSWD[100];
};

Config config;

void saveConfig() {
  EEPROM.put(0, config);
  EEPROM.commit();
}

void cleanALL() {
  Serial.println("EEPROM повреждена или не инициализирована. Сброс настроек...");
  for (int i = 0; i < EEPROM_SIZE; i++) EEPROM.write(i, 0xFF);
  EEPROM.commit();
  config.mode = 'L';
  strcpy(config.apiUrl, "http://192.168.0.129:5000/data");
  strcpy(config.wifiSSID, "your_SSID");
  strcpy(config.wifiPSWD, "your_PASSWORD");
  strcpy(config.apSSID, "Meteostation");
  strcpy(config.apPSWD, "12345678");
  saveConfig();
}

void loadConfig() {
  EEPROM.get(0, config);
  bool invalid = false;
  if (config.mode != 'L' && config.mode != 'A') invalid = true;
  if (config.apiUrl[0] == '\0' || strlen(config.apiUrl) > 99) invalid = true;
  if (config.wifiSSID[0] == '\0' || strlen(config.wifiSSID) > 99) invalid = true;
  if (config.wifiPSWD[0] == '\0' || strlen(config.wifiPSWD) > 99) invalid = true;
  if (config.apSSID[0] == '\0' || strlen(config.apSSID) > 99) invalid = true;
  if (config.apPSWD[0] == '\0' || strlen(config.apPSWD) > 99) invalid = true;
  if (invalid) {
    Serial.println("EEPROM повреждена или не инициализирована. Сброс настроек...");
    cleanALL();
  }
}

void handleRoot() {
  blinkLED();
  String html = R"rawliteral(
  <html>
    <head>
      <meta charset="UTF-8">
      <title>Настройки</title>
      <style>
        body {
          background-color: #0A2227;
          color: #ecf0f1;
          font-family: Arial, sans-serif;
          margin: 0;
          padding: 20px;
        }
        h2 {
          text-align: center;
          color: #FFFFFF;
        }
        form {
          max-width: 400px;
          margin: 0 auto;
          background: #0C0E12;
          padding: 20px;
          border-radius: 10px;
          box-shadow: 0 0 3px #000;
        }
        input[type="radio"] {
          margin-right: 10px;
        }
        input[type="text"] {
          width: 100%;
          padding: 8px;
          margin-top: 5px;
          margin-bottom: 15px;
          border: none;
          border-radius: 5px;
        }
        input[type="submit"] {
          background-color: #064A47;
          color: white;
          border: none;
          padding: 10px 20px;
          text-transform: uppercase;
          border-radius: 5px;
          cursor: pointer;
          width: 100%;
        }
        input[type="submit"]:hover {
          background-color: #007864;
        }
        label {
          display: block;
          margin-top: 10px;
          margin-bottom: 5px;
        }
      </style>
    </head>
    <body>
      <h2>Настройки</h2>
      <form action="/save" method="post">
        <label>Режим:</label>
        <input type="radio" name="mode" value="L" %LCHK%> Локальный
        <input type="radio" name="mode" value="A" %ACHK%> API

        <label>WIFI SSID:</label>
        <input type="text" name="wifiSSID" value="%WIFI_SSID%">
        <label>WIFI Password:</label>
        <input type="text" name="wifiPSWD" value="%WIFI_PSWD%">

        <label>AP SSID:</label>
        <input type="text" name="apSSID" value="%AP_SSID%">
        <label>AP Password:</label>
        <input type="text" name="apPSWD" value="%AP_PSWD%">

        <label>API URL:</label>
        <input type="text" name="api" value="%API%">

        <input type="submit" value="Сохранить">
      </form>
    </body>
  </html>
  )rawliteral";

  html.replace("%LCHK%", config.mode == 'L' ? "checked" : "");
  html.replace("%ACHK%", config.mode == 'A' ? "checked" : "");
  html.replace("%API%", config.apiUrl);
  html.replace("%WIFI_SSID%", config.wifiSSID);
  html.replace("%WIFI_PSWD%", config.wifiPSWD);
  html.replace("%AP_SSID%", config.apSSID);
  html.replace("%AP_PSWD%", config.apPSWD);

  server.send(200, "text/html", html);
}

void handleSave() {
  if (server.hasArg("mode")) config.mode = server.arg("mode")[0];
  if (server.hasArg("api")) server.arg("api").toCharArray(config.apiUrl, 100);
   if (server.hasArg("wifiSSID")) server.arg("wifiSSID").toCharArray(config.wifiSSID, 100);
  if (server.hasArg("wifiPSWD")) server.arg("wifiPSWD").toCharArray(config.wifiPSWD, 100);
  if (server.hasArg("apSSID")) server.arg("apSSID").toCharArray(config.apSSID, 100);
  if (server.hasArg("apPSWD")) server.arg("apPSWD").toCharArray(config.apPSWD, 100);
  Serial.println("Сохранение настроек...");
  Serial.printf("Mode: %c\n", config.mode);
  Serial.printf("API URL: %s\n", config.apiUrl);
  Serial.printf("WIFI SSID: %s\n", config.wifiSSID);
  Serial.printf("WIFI Password: %s\n", config.wifiPSWD);
  Serial.printf("AP SSID: %s\n", config.apSSID);
  Serial.printf("AP Password: %s\n", config.apPSWD);
  saveConfig();
  server.send(200, "text/html", "<html><body>saved. rebooting.. <br> yappi</body></html>");
  Serial.println("Настройки сохранены. Перезагрузка...");
  delay(1000);
  ESP.restart();
}

void setupWiFi() {
  blinkLED();
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSSID, config.wifiPSWD);
  WiFi.setAutoReconnect(true);
  Serial.println("Connecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(config.wifiSSID);
  Serial.print("Password: ");
  Serial.println(config.wifiPSWD);
  for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
    delay(500);
    Serial.print(i);
    blinkLED();
  }
}

void setupAP() {
  blinkLED();
  WiFi.mode(WIFI_AP);
  WiFi.softAP(config.apSSID, config.apPSWD);
  Serial.print("AP Mode: ");
  Serial.println(config.apSSID);
  Serial.print("Password: ");
  Serial.println(config.apPSWD);
  Serial.print("IP Address: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("Web server started.");
  Serial.println("Connect to the AP and open http://192.168.4.1 in your browser to see the data.");
  Serial.println("МЯУ ЕПТА");
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

void sendChangeResultsRequest() {
    if (WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        String url = "http://192.168.0.129:5000/change-results";
        http.begin(wifiClient, url);
        int httpResponseCode = http.GET();
        if (httpResponseCode == 200) {
            String response = http.getString();
            Serial.println("Сервер ответил: " + response);
        } else {
            Serial.println("Ошибка. Код: " + String(httpResponseCode));
        }
        http.end();
    }
}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(EEPROM_SIZE);
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);
  pinMode(CHANGE_RESULTS_PIN, INPUT_PULLUP); // Настройка пина D6 с подтяжкой к HIGH

  delay(200);

  if (digitalRead(RESET_PIN) == LOW) {
    Serial.println("Кнопка сброса зажата. Выполняется сброс...");
    cleanALL();
  } else {
    Serial.println("Обычная загрузка...");
    loadConfig();
  }

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
      float altitude = bme.readAltitude(1006.51);
      sendToApi(t, h, p, mmHg, altitude);
    }
    delay(3000);
  }

  // Проверка состояния кнопки сброса данных
  static bool lastButtonState = HIGH;
  bool currentButtonState = digitalRead(CHANGE_RESULTS_PIN);
  if (lastButtonState == HIGH && currentButtonState == LOW) {
    Serial.println("Кнопка сброса данных зажата. Отправка запроса...");
    sendChangeResultsRequest();
    blinkLED(); // Индикация действия
  }
  lastButtonState = currentButtonState;
}
