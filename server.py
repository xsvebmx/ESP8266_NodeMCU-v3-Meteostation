from flask import Flask, request, send_from_directory, jsonify
from datetime import datetime
import json
import os
import pandas as pd
import matplotlib.pyplot as plt

app = Flask(__name__)

DATA_FILE = "data.jsonl"
PLOTS_DIR = "plots"
VALID_PARAMS = ["temperature", "humidity", "pressure", "pressure_mmHg", "altitude"]

# создать папку для графиков
os.makedirs(PLOTS_DIR, exist_ok=True)

def save_data_and_update_plots(data):
    # Добавить метку времени
    data["timestamp"] = datetime.now().isoformat()
    with open(DATA_FILE, "a") as f:
        f.write(json.dumps(data, ensure_ascii=False) + "\n")

    # Перерисовать графики
    df = pd.read_json(DATA_FILE, lines=True)
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

# основной маршрут POST
@app.route("/data", methods=["POST"])
def receive_data():
    data = request.get_json()
    if not data:
        return jsonify({"error": "Нет данных"}), 400
    save_data_and_update_plots(data)
    print("Получено:", data)
    return jsonify({"status": "ok"})

# получение последних данных (GET)
@app.route("/data", methods=["GET"])
def get_latest_data():
    if not os.path.exists(DATA_FILE):
        return jsonify({"error": "Файл не найден"}), 404
    with open(DATA_FILE, "r") as f:
        lines = f.readlines()
        if not lines:
            return jsonify({"error": "Нет данных"}), 404
        return jsonify(json.loads(lines[-1]))

# выдача графика
@app.route("/plot/<param>.png")
def serve_plot(param):
    if param not in VALID_PARAMS:
        return "Неверный параметр", 404
    file_path = os.path.join(PLOTS_DIR, f"{param}.png")
    if not os.path.exists(file_path):
        return "График не найден", 404
    return send_from_directory(PLOTS_DIR, f"{param}.png")

if __name__ == "__main__":
    if not os.path.exists(DATA_FILE):
        open(DATA_FILE, "w").close()
    app.run(host="0.0.0.0", port=5000)
