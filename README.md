# 🌦️ ESP8266 NodeMCU v3 Meteostation

Простая метеостанция на базе **ESP8266 NodeMCU v3** и датчика **BME280**, которая умеет:

* Работать в 2 режимах:
  * **Локальный веб-интерфейс** — показывает данные прямо на странице устройства.
  * **API-режим** — отправляет данные на сервер Flask и строит графики.

---

## ⚙️ Возможности

### ESP8266:

* Wi-Fi: режим точки доступа и подключения к сети.
* Измеряет:

  * Температуру (°C)
  * Влажность воздуха (%)
  * Давление (гПа и мм рт. ст.)
  * Высоту (м)
* Настройка режима и URL через веб-страницу.
* EEPROM для хранения настроек.

### Flask-сервер:

* Получает данные по POST (`/data`)
* Отдаёт последние замеры (`GET /data`)
* Автоматически строит графики в формате PNG:

  * `/plot/temperature.png`
  * `/plot/humidity.png`
  * `/plot/pressure.png`
  * `/plot/pressure_mmHg.png`
  * `/plot/altitude.png`

---

## 📦 Структура проекта

```
ESP8266_NodeMCU-v3-Meteostation/
├── server.py         # Flask сервер
├── data.jsonl        # История замеров (JSON Lines)
├── plots/            # Графики
│   ├── temperature.png
│   ├── humidity.png
│   ├── pressure_mmHg.png
│   ├── pressure.png
│   └── temperature.png
├── main              # Код для прошивки ESP8266
└── README.md         # Документация
```

---

## 🔌 Подключение BME280 к ESP8266

| BME280 | ESP8266 NodeMCU v3 |
| ------ | ------------------ |
| VIN    | 3V3                |
| GND    | GND                |
| SDA    | D2 (GPIO4)         |
| SCL    | D1 (GPIO5)         |

Адрес I2C по умолчанию: `0x76`

---

## 🚀 Быстрый старт

### 1. Подключите BME280 и прошейте ESP8266

* Используйте код из `main`
* При первом запуске будет создана точка доступа:

  * SSID: `NodeConfig`
  * Пароль: `12345678`

Зайдите на [http://192.168.4.1](http://192.168.4.1), выберите режим работы:

* `L` — локальный (страница `/data`)
* `A` — отправка данных на сервер Flask

### 2. Запустите Flask-сервер

```bash
pip install flask pandas matplotlib
python server.py
```

Данные сохраняются, графики обновляются автоматически.

---

## 📡 Пример данных (API)

* **POST /data**

  * `Content-Type: application/json`
  * Пример:

    ```json
    {
      "temperature": 24.7,
      "humidity": 45.1,
      "pressure": 1010.2,
      "pressure_mmHg": 757.7,
      "altitude": 128.3
    }
    ```

* **GET /data** — возвращает последний замер

* **GET /plot/{param}.png** — отдаёт график по параметру

---

## Вывод графиков

Температура
![temperature](plots/temperature.png)
Влажность
![humidity](plots/humidity.png)
Давление (гПа)
![pressure](plots/pressure.png) 
Давление (мм рт. ст.) 
![pressure\_mmHg](plots/pressure_mmHg.png) 
Высота
![altitude](plots/altitude.png)

---