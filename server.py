from flask import Flask, request, send_from_directory, jsonify
from datetime import datetime
import json
import os
import pandas as pd
import matplotlib.pyplot as plt
import shutil  # Для операций с файлами и папками

app = Flask(__name__)

DATA_FILE = "data.jsonl"
PLOTS_DIR = "plots"
VALID_PARAMS = ["temperature", "humidity", "pressure", "pressure_mmHg", "altitude"]
ARCHIVES_DIR = "archives"  # Папка для хранения архивов

# Создать папку для графиков, если её нет
os.makedirs(PLOTS_DIR, exist_ok=True)

def save_data_and_update_plots(data):
    # Добавить метку времени
    data["timestamp"] = datetime.now().isoformat()
    with open(DATA_FILE, "a") as f:
        f.write(json.dumps(data, ensure_ascii=False) + "\n")

    # Перерисовать графики только если файл существует и не пуст
    if os.path.exists(DATA_FILE) and os.path.getsize(DATA_FILE) > 0:
        try:
            df = pd.read_json(DATA_FILE, lines=True)
            if df.empty:
                print("Нет данных для построения графиков.")
                return
            df["timestamp"] = pd.to_datetime(df["timestamp"])
            df.sort_values("timestamp", inplace=True)

            for param in VALID_PARAMS:
                if param not in df.columns:
                    continue
                plt.figure(figsize=(8, 3))
                plt.plot(df["timestamp"], df[param], label=param, color="red")
                plt.title(param)
                plt.xlabel("время")
                plt.ylabel(param)
                plt.grid(True)
                plt.tight_layout()
                plt.savefig(f"{PLOTS_DIR}/{param}.png")
                plt.close()
        except (FileNotFoundError, ValueError) as e:
            print(f"Не удалось сгенерировать графики: {e}")

# Основной маршрут POST для получения данных
@app.route("/data", methods=["POST"])
def receive_data():
    data = request.get_json()
    if not data:
        return jsonify({"error": "Нет данных"}), 400
    save_data_and_update_plots(data)
    print("Получено:", data)
    return jsonify({"status": "ok"})

# Получение последних данных (GET)
@app.route("/data", methods=["GET"])
def get_latest_data():
    if not os.path.exists(DATA_FILE):
        return jsonify({"error": "Файл не найден"}), 404
    with open(DATA_FILE, "r") as f:
        lines = f.readlines()
        if not lines:
            return jsonify({"error": "Нет данных"}), 404
        return jsonify(json.loads(lines[-1]))

# Выдача графика
@app.route("/plot/<param>.png")
def serve_plot(param):
    if param not in VALID_PARAMS:
        return "Неверный параметр", 404
    file_path = os.path.join(PLOTS_DIR, f"{param}.png")
    if not os.path.exists(file_path):
        return "График не найден", 404
    return send_from_directory(PLOTS_DIR, f"{param}.png")

# Маршрут для архивации данных и сброса
@app.route("/change-results", methods=["GET"])
def archive_data():
    try:
        # Создать папку для архивов, если её нет
        os.makedirs(ARCHIVES_DIR, exist_ok=True)
        
        # Создать новую папку с именем на основе текущей даты и времени
        archive_dir = os.path.join(ARCHIVES_DIR, f"archive_{datetime.now().strftime('%Y-%m-%d_%H-%M-%S')}")
        os.makedirs(archive_dir, exist_ok=True)
        
        # Скопировать файл data.jsonl в архив, если он существует
        if os.path.exists(DATA_FILE):
            shutil.copy(DATA_FILE, archive_dir)
        
        # Скопировать папку plots в архив, если она существует
        if os.path.exists(PLOTS_DIR):
            shutil.copytree(PLOTS_DIR, os.path.join(archive_dir, PLOTS_DIR))
        
        # Удалить оригинальный файл data.jsonl, если он существует
        if os.path.exists(DATA_FILE):
            os.remove(DATA_FILE)
        
        # Удалить папку plots и её содержимое, если она существует
        if os.path.exists(PLOTS_DIR):
            shutil.rmtree(PLOTS_DIR)
        
        # Создать новую пустую папку plots
        os.makedirs(PLOTS_DIR, exist_ok=True)
        
        return jsonify({"status": "archived and reset"})
    except Exception as e:
        return jsonify({"error": str(e)}), 500

if __name__ == "__main__":
    if not os.path.exists(DATA_FILE):
        open(DATA_FILE, "w").close()
    app.run(host="0.0.0.0", port=5000)
