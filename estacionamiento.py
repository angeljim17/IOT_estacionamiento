import numpy as np
from ultralytics import YOLO
import cv2
import pyrealsense2 as rs
import time
import paho.mqtt.client as mqtt
from collections import deque

# --- MQTT Configuración ---
broker = "test.mosquitto.org"
port = 1883
topic = "vehiculos/conteo"
client_id = "DeteccionVehiculosPython"

client = mqtt.Client(client_id=client_id, protocol=mqtt.MQTTv311)
client.connect(broker, port, 60)

# --- Inicializar cámara RealSense ---
def initialize_camera():
    pipeline = rs.pipeline()
    config = rs.config()
    config.enable_stream(rs.stream.depth, 640, 480, rs.format.z16, 30)
    config.enable_stream(rs.stream.color, 640, 480, rs.format.bgr8, 30)
    pipeline.start(config)
    return pipeline

pipeline = initialize_camera()
model = YOLO('yolov8n.pt')

print("Sistema de Detección de Vehículos iniciado")
print("Enviando datos MQTT al broker Mosquitto...")

last_publish = time.time()

# Colores para cada tipo
colors = {
    "auto": (0, 255, 0),       # Verde
    "moto": (255, 0, 0),       # Azul
    "camion": (0, 255, 255)    # Amarillo
}

# Clases permitidas (COCO)
allowed_classes = [1, 2, 3, 5, 7]  # 1=bicycle,2=car,3=motorcycle,5=bus,7=truck

# Lugares iniciales
total_autos = 5
total_camiones = 5
total_motos = 3
available_autos = total_autos
available_camiones = total_camiones
available_motos = total_motos

# Seguimiento de vehículos detectados
tracked_vehicles = {
    "auto": deque(maxlen=50),
    "camion": deque(maxlen=50),
    "moto": deque(maxlen=50)
}

DIST_THRESHOLD = 50  # píxeles

def vehicle_exists(tracked_list, center):
    for c in tracked_list:
        if np.linalg.norm(np.array(center) - np.array(c)) < DIST_THRESHOLD:
            return True
    return False

def update_tracked(tracked_list, current_centers):
    new_tracked = deque(maxlen=50)
    for c in current_centers:
        new_tracked.append(c)
    return new_tracked

while True:
    try:
        frames = pipeline.wait_for_frames()
        color_frame = frames.get_color_frame()
        if not color_frame:
            continue

        img = np.asanyarray(color_frame.get_data())
        results = model(img, stream=True)

        current_auto = []
        current_camion = []
        current_moto = []

        for r in results:
            for box in r.boxes:
                cls = int(box.cls[0])
                if cls not in allowed_classes:
                    continue

                x1, y1, x2, y2 = box.xyxy[0].cpu().numpy().astype(int)
                x_center = int((x1 + x2) / 2)
                y_center = int((y1 + y2) / 2)

                if cls == 2:  # Auto
                    current_auto.append((x_center, y_center))
                    cv2.rectangle(img, (x1, y1), (x2, y2), colors["auto"], 2)
                    cv2.putText(img, "Auto", (x1, y1-10), cv2.FONT_HERSHEY_SIMPLEX, 0.8, colors["auto"], 2)
                elif cls in [5, 7]:  # Camion
                    current_camion.append((x_center, y_center))
                    cv2.rectangle(img, (x1, y1), (x2, y2), colors["camion"], 2)
                    cv2.putText(img, "Camion", (x1, y1-10), cv2.FONT_HERSHEY_SIMPLEX, 0.8, colors["camion"], 2)
                elif cls in [1, 3]:  # Moto
                    current_moto.append((x_center, y_center))
                    cv2.rectangle(img, (x1, y1), (x2, y2), colors["moto"], 2)
                    cv2.putText(img, "Moto", (x1, y1-10), cv2.FONT_HERSHEY_SIMPLEX, 0.8, colors["moto"], 2)

        # --- Actualizar contadores dinámicos ---
        # Autos
        entered_auto = [c for c in current_auto if not vehicle_exists(tracked_vehicles["auto"], c)]
        exited_auto = [c for c in tracked_vehicles["auto"] if not vehicle_exists(current_auto, c)]
        available_autos -= len(entered_auto)
        available_autos += len(exited_auto)
        available_autos = max(0, min(total_autos, available_autos))

        # Camiones
        entered_camion = [c for c in current_camion if not vehicle_exists(tracked_vehicles["camion"], c)]
        exited_camion = [c for c in tracked_vehicles["camion"] if not vehicle_exists(current_camion, c)]
        available_camiones -= len(entered_camion)
        available_camiones += len(exited_camion)
        available_camiones = max(0, min(total_camiones, available_camiones))

        # Motos
        entered_moto = [c for c in current_moto if not vehicle_exists(tracked_vehicles["moto"], c)]
        exited_moto = [c for c in tracked_vehicles["moto"] if not vehicle_exists(current_moto, c)]
        available_motos -= len(entered_moto)
        available_motos += len(exited_moto)
        available_motos = max(0, min(total_motos, available_motos))

        # Actualizar listas de seguimiento
        tracked_vehicles["auto"] = update_tracked(tracked_vehicles["auto"], current_auto)
        tracked_vehicles["camion"] = update_tracked(tracked_vehicles["camion"], current_camion)
        tracked_vehicles["moto"] = update_tracked(tracked_vehicles["moto"], current_moto)

        # Fondo semitransparente
        overlay = img.copy()
        cv2.rectangle(overlay, (15, 10), (300, 160), (0, 0, 0), -1)
        alpha = 0.5
        img = cv2.addWeighted(overlay, alpha, img, 1 - alpha, 0)

        # Mostrar contadores
        cv2.putText(img, f"Lugares Autos: {available_autos}", (20, 40), cv2.FONT_HERSHEY_SIMPLEX, 1, colors["auto"], 2)
        cv2.putText(img, f"Lugares Camiones: {available_camiones}", (20, 80), cv2.FONT_HERSHEY_SIMPLEX, 1, colors["camion"], 2)
        cv2.putText(img, f"Lugares Motos: {available_motos}", (20, 120), cv2.FONT_HERSHEY_SIMPLEX, 1, colors["moto"], 2)

        cv2.imshow("🚗 Conteo de Vehículos", img)

        # Enviar por MQTT cada medio segundo
        if time.time() - last_publish >= 0.5:
            payload = f"{available_autos},{available_motos},{available_camiones}"
            client.publish(topic, payload)
            print(f"📡 MQTT -> {payload}")
            last_publish = time.time()

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    except Exception as e:
        print(f"⚠️ Error: {e}")
        print("Reintentando cámara...")
        try:
            pipeline.stop()
        except:
            pass
        time.sleep(2)
        pipeline = initialize_camera()

# Graceful exit MQTT
client.loop_stop()
client.disconnect()

pipeline.stop()
cv2.destroyAllWindows()
print("Sistema cerrado correctamente")
