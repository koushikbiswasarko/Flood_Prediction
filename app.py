from flask import Flask, request, jsonify
import tensorflow as tf
from tensorflow.keras.models import load_model
import numpy as np
import csv, os
from datetime import datetime
import socket

app = Flask(__name__)
LOG_FILE = 'flood_data_log.csv'
MODEL_PATH = 'D:\\Water_Management\\model.h5'  # ✅ Use .h5 file

# ==== Load .h5 model ====
print("Loading LSTM model (.h5)...")
try:
    model = load_model(MODEL_PATH)
    print("✅ Model loaded successfully.")
except Exception as e:
    print(f"❌ Failed to load model: {e}")
    exit(1)

def init_csv():
    if not os.path.exists(LOG_FILE):
        with open(LOG_FILE, 'w', newline='') as file:
            writer = csv.writer(file)
            writer.writerow(['Time', 'Temp', 'Humidity', 'Soil', 'Soil_D', 'Rain', 'Rain_D', 'Level', 'Flow', 'Risk'])

init_csv()

@app.route('/predict', methods=['POST'])
def predict():
    try:
        data = request.get_json(force=True)
        temp = float(data.get("temp", 0))
        humidity = float(data.get("humidity", 0))
        soil = float(data.get("soil_analog", 0))
        soil_dig = int(data.get("soil_digital", 0))
        rain = float(data.get("rain_analog", 0))
        rain_dig = int(data.get("rain_digital", 0))
        level = float(data.get("level", 0))
        flow = float(data.get("flow", 0))
    except Exception as e:
        return jsonify({"error": f"Invalid input data: {str(e)}"}), 400

    # === Preprocess for model ===
    input_data = np.array([[temp, humidity, soil, soil_dig, rain, rain_dig, level, flow]], dtype=np.float32)
    input_reshaped = input_data.reshape(1, 1, 8)  # LSTM needs 3D input

    # === Predict ===
    try:
        preds = model.predict(input_reshaped)
    except Exception as e:
        return jsonify({"error": f"Model prediction failed: {str(e)}"}), 500

    # === Interpret prediction ===
    risk_levels = ["LOW", "MODERATE", "HIGH"]
    risk = risk_levels[int(np.argmax(preds))]

    # === Log to CSV ===
    with open(LOG_FILE, 'a', newline='') as file:
        writer = csv.writer(file)
        writer.writerow([
            datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
            temp, humidity, soil, soil_dig, rain, rain_dig, level, flow, risk
        ])

    return jsonify({
        "risk": risk,
        "probabilities": preds.tolist(),
        "received": data
    })

if __name__ == '__main__':
    ip = socket.gethostbyname(socket.gethostname())
    print(f"Server runs at: http://{ip}:5000")
    app.run(host='0.0.0.0', port=5000, debug=True)