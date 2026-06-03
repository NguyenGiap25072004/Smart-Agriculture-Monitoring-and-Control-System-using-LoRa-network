import firebase_admin
from firebase_admin import credentials, db
import pandas as pd
import time
from datetime import datetime
import os

# 1. Khởi tạo kết nối với Firebase
cred = credentials.Certificate("firebase_key.json")
if not firebase_admin._apps:
    firebase_admin.initialize_app(cred, {
        'databaseURL': 'https://smartfarmvju-default-rtdb.asia-southeast1.firebasedatabase.app/'
    })

csv_file = "sensor_data.csv"

# BỘ NHỚ TẠM: Lưu ID mới nhất của từng Node để đối chiếu
last_saved_ids = {
    'Node1': None,
    'Node2': None
}

def fetch_and_save_data():
    nodes = ['Node1', 'Node2']
    
    for node_name in nodes:
        try:
            latest_id = db.reference(f'{node_name}/latestID').get()
            
            # KIỂM TRA ĐIỀU KIỆN: Chỉ lưu khi có latest_id VÀ latest_id này phải khác với ID lần trước đã lưu
            if latest_id is not None and latest_id != last_saved_ids[node_name]:
                node_path = f'{node_name}/{latest_id}'
                ref = db.reference(node_path)
                data = ref.get()

                if data:
                    new_row = {
                        'Timestamp': datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
                        'Node_ID': node_name,
                        'Temperature': data.get('Temperature', 0),
                        'Humidity': data.get('Humidity', 0),
                        'Light': data.get('Light', 0),
                        'Soil_Moisture': data.get('SoilMoisture', 0)
                    }
                    
                    df_new = pd.DataFrame([new_row])
                    
                    # Lưu vào file CSV
                    if not os.path.isfile(csv_file):
                        df_new.to_csv(csv_file, index=False)
                    else:
                        df_new.to_csv(csv_file, mode='a', header=False, index=False)
                    
                    print(f"[{new_row['Timestamp']}] ĐÃ LƯU MỚI {node_name} (ID:{latest_id}) | Nhiệt độ: {new_row['Temperature']}°C | Ẩm khí: {new_row['Humidity']}% | Ẩm đất: {new_row['Soil_Moisture']}%")
                    
                    # Cập nhật lại bộ nhớ tạm
                    last_saved_ids[node_name] = latest_id
            
            # Bỏ dòng lệnh else in ra thông báo "Không tìm thấy" để Terminal đỡ bị rác khi không có data mới

        except Exception as e:
            print(f"Lỗi khi lấy dữ liệu từ {node_name}:", e)

# 3. Vòng lặp chạy liên tục
print(" BẮT ĐẦU THU THẬP DỮ LIỆU NHÀ KÍNH (DATA LOGGING)")
print("Nhấn Ctrl+C trên bàn phím để dừng chương trình.\n")

while True:
    fetch_and_save_data()
    # Tạm dừng 5 phút (300 giây)
    time.sleep(300)
