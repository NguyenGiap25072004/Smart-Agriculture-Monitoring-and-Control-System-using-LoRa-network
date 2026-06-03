import firebase_admin
from firebase_admin import credentials, db
import numpy as np
import pandas as pd            
from tensorflow.keras.models import load_model
import joblib
import time
from datetime import datetime

# 1. KHỞI TẠO VÀ TẢI MÔ HÌNH (DÙNG CHUNG)
print("Đang khởi động Hệ thống AI Real-time (Multi-Node)...")

cred = credentials.Certificate("firebase_key.json")
if not firebase_admin._apps:
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://smartfarmvju-default-rtdb.asia-southeast1.firebasedatabase.app/'
    })

# Dùng chung Bộ não và Cái cân cho toàn bộ các Node trong cùng 1 nhà kính
model = load_model('lstm_soil_model_node1.h5')
scaler = joblib.load('scaler_node1.gz')
print("Đã nạp thành công Model và Scaler!")

# Danh sách các Node cần AI đi tuần tra (Có thể mở rộng thêm Node 3, 4,... thì chỉ cần ghi thêm vào đây)
ACTIVE_NODES = ['Node1', 'Node2']

# 2. HÀM LẤY 12 DÒNG DỮ LIỆU GẦN NHẤT
def get_latest_12_records(node_name):
    # Lấy dư ra 15 bản ghi để đề phòng vướng biến latestID hoặc dữ liệu rác
    ref = db.reference(f'/{node_name}').order_by_key().limit_to_last(15)
    data = ref.get()
    
    if not data:
        return None 
    
    records = []
    for key, val in data.items():
        if key == 'latestID': 
            continue # Bỏ qua biến latestID
            
        # Kiểm tra chắc chắn val là dữ liệu cảm biến (dict)
        if isinstance(val, dict) and 'Temperature' in val:
            records.append({
                'Temperature': val.get('Temperature', 0),
                'Humidity': val.get('Humidity', 0),
                'Light': val.get('Light', 0),
                'SoilMoisture': val.get('SoilMoisture', 0)
            })
    
    # Chỉ lấy đúng 12 bản ghi cuối cùng (mới nhất)
    if len(records) < 12:
        return None
        
    latest_12 = records[-12:]
    
    # Đóng gói thành DataFrame có tên cột 
    df_records = pd.DataFrame(latest_12)
    return df_records

# 3. VÒNG LẶP TUẦN TRA ĐA ĐIỂM
print("\nAI ĐÃ VÀO CA TRỰC! Đang theo dõi các Node...")

while True:
    for node in ACTIVE_NODES:
        try:
            # 1. Lấy dữ liệu 60 phút qua của từng Node
            recent_data = get_latest_12_records(node)
            
            if recent_data is not None:
                # 2. Tiền xử lý (Lúc này recent_data đã là DataFrame chuẩn)
                scaled_input = scaler.transform(recent_data)
                
                # Đóng gói thành ma trận 3D (1, 12, 4)
                X_input = scaled_input.reshape(1, 12, 4)
                
                # 3. AI Dự đoán
                scaled_prediction = model.predict(X_input, verbose=0)[0][0]
                
                # 4. Dịch ngược kết quả
                dummy_array = np.zeros((1, 4))
                dummy_array[0, 3] = scaled_prediction
                real_prediction = scaler.inverse_transform(dummy_array)[0, 3]
                real_prediction = round(real_prediction, 1)
                
                # 5. Phân loại Logic
                decision = "SAFE"
                if real_prediction > 40:
                    decision = "SAFE"
                elif 30 <= real_prediction <= 40:
                    decision = "WARNING"
                elif real_prediction < 30:
                    decision = "PUMP_ON"
                    
                print(f"[{datetime.now().strftime('%H:%M:%S')}] {node} | DỰ BÁO 30P TỚI: {real_prediction}% -> LỆNH: {decision}")
                
                # 6. Gửi lệnh lên Firebase
                db.reference(f'/Network_Config/Active_Nodes/{node}/ai_predicted_soil').set(float(real_prediction))
                db.reference(f'/Network_Config/Active_Nodes/{node}/ai_decision').set(decision)
                db.reference(f'/Network_Config/Active_Nodes/{node}/control_mode').set("AI") 
                
            else:
                print(f"[{datetime.now().strftime('%H:%M:%S')}] {node}: Chưa đủ 12 bản ghi hợp lệ để dự đoán.")
                
        except Exception as e:
            print(f"Lỗi tại {node}: {e}")
            
    print("-" * 50)
    time.sleep(300)
