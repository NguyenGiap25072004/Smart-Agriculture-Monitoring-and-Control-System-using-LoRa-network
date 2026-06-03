import pandas as pd
import numpy as np
from sklearn.preprocessing import MinMaxScaler
from tensorflow.keras.models import Sequential
from tensorflow.keras.layers import LSTM, Dense, Dropout
from tensorflow.keras.optimizers import Adam
import joblib
import matplotlib.pyplot as plt

# 1. TIỀN XỬ LÝ DỮ LIỆU
print("1. Đang tải và làm sạch dữ liệu...")
df = pd.read_csv('sensor_data.csv')
df['Timestamp'] = pd.to_datetime(df['Timestamp'])
df = df.sort_values('Timestamp')

# Lọc lấy dữ liệu Node 1 (làm tương tự cho Node 2 sau)
df_node1 = df[df['Node_ID'] == 'Node1'].copy()
df_node1.set_index('Timestamp', inplace=True)
df_node1.drop(columns=['Node_ID'], inplace=True)
df_node1.fillna(method='ffill', inplace=True)

print("2. Đang chuẩn hóa dữ liệu...")
scaler = MinMaxScaler(feature_range=(0, 1))
scaled_data = scaler.fit_transform(df_node1)

# LƯU LẠI "CÁI CÂN" ĐỂ DÙNG CHO REAL-TIME SAU NÀY
joblib.dump(scaler, 'scaler_node1.gz') 
print("-> Đã lưu Scaler thành file scaler_node1.gz")

SOIL_COL_INDEX = 3 # Vị trí cột Soil Moisture (Temp=0, Hum=1, Light=2, Soil=3)
HISTORY_STEPS = 12 # 60 phút quá khứ
FUTURE_STEPS = 6   # 30 phút tương lai

def create_sequences(data, history_steps, future_steps, target_col_index):
    X, y = [], []
    for i in range(len(data) - history_steps - future_steps + 1):
        X.append(data[i : (i + history_steps), :])
        y.append(data[i + history_steps + future_steps - 1, target_col_index])
    return np.array(X), np.array(y)

X, y = create_sequences(scaled_data, HISTORY_STEPS, FUTURE_STEPS, SOIL_COL_INDEX)

# CHIA TẬP DỮ LIỆU (80% để Học - 20% để Thi thử)
# Lưu ý: Chuỗi thời gian không được xáo trộn ngẫu nhiên, phải cắt theo thứ tự thời gian
split_idx = int(len(X) * 0.8)
X_train, X_test = X[:split_idx], X[split_idx:]
y_train, y_test = y[:split_idx], y[split_idx:]

print(f"-> Tập huấn luyện (Train): {X_train.shape}")
print(f"-> Tập kiểm tra (Test): {X_test.shape}")

# 2. XÂY DỰNG KIẾN TRÚC LSTM
print("\n3. Khởi tạo mạng LSTM...")
model = Sequential()
model.add(LSTM(units=64, return_sequences=True, input_shape=(X_train.shape[1], X_train.shape[2])))
model.add(Dropout(0.2))
model.add(LSTM(units=32, return_sequences=False))
model.add(Dropout(0.2))
model.add(Dense(units=1))

model.compile(optimizer=Adam(learning_rate=0.001), loss='mean_squared_error')

# 3. HUẤN LUYỆN VÀ LƯU MÔ HÌNH (TRAINING)
print("\n4. Bắt đầu quá trình huấn luyện (Training)...")
# Epochs=50 có nghĩa là bắt AI giải đi giải lại bộ đề thi 50 lần để rút kinh nghiệm
history = model.fit(X_train, y_train, epochs=50, batch_size=16, validation_data=(X_test, y_test), verbose=1)

# LƯU LẠI "BỘ NÃO" CỦA AI
model.save('lstm_soil_model_node1.h5')
print("-> Đã lưu mô hình thành file lstm_soil_model_node1.h5")

# 4. VẼ BIỂU ĐỒ ĐÁNH GIÁ 
plt.figure(figsize=(10, 6))
plt.plot(history.history['loss'], label='Sai số lúc Học (Train Loss)')
plt.plot(history.history['val_loss'], label='Sai số lúc Thi (Validation Loss)')
plt.title('Biểu đồ đánh giá quá trình huấn luyện LSTM')
plt.xlabel('Epochs (Số vòng lặp)')
plt.ylabel('Sai số (Mean Squared Error)')
plt.legend()
plt.grid(True)
plt.savefig('training_loss_chart.png')
print("-> Đã lưu biểu đồ đánh giá thành training_loss_chart.png")
