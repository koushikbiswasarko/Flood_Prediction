import numpy as np
import pandas as pd
import tensorflow as tf
from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler, LabelEncoder

# === 1. Load data ===
df = pd.read_csv("flood_dataset.csv")

# === 2. Encode target labels ===
label_encoder = LabelEncoder()
df["Flood Risk"] = label_encoder.fit_transform(df["Flood Risk"])
# Now: LOW=0, MODERATE=1, HIGH=2

# === 3. Define features and target ===
features = [
    "temp", "humidity", "soil_analog", "soil_digital",
    "rain_analog", "rain_digital", "level", "flow"
]
target = "Flood Risk"

X = df[features].values
y = df[target].values

# === 4. Normalize features ===
scaler = StandardScaler()
X = scaler.fit_transform(X)

# === 5. Reshape for LSTM: [samples, time_steps, features]
X = X.reshape((X.shape[0], 1, X.shape[1]))
y = tf.keras.utils.to_categorical(y, num_classes=3)  # convert to one-hot

# === 6. Split data ===
X_train, X_val, y_train, y_val = train_test_split(X, y, test_size=0.2, random_state=42)

# === 7. Build LSTM model ===
model = tf.keras.Sequential([
    tf.keras.layers.Input(shape=(1, len(features))),
    tf.keras.layers.LSTM(64),
    tf.keras.layers.Dense(32, activation="relu"),
    tf.keras.layers.Dense(3, activation="softmax")
])

model.compile(optimizer="adam", loss="categorical_crossentropy", metrics=["accuracy"])
model.summary()

# === 8. Train ===
model.fit(X_train, y_train, epochs=30, batch_size=16, validation_data=(X_val, y_val))

# === 9. Save model (.h5 format for Flask)
model.save("model.h5")  # ✅ This is what Flask will load

print("✅ Model saved as model.h5")