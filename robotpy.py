import cv2
import time
import requests
import openai
import base64
from ultralytics import YOLO
from PIL import Image
from io import BytesIO

# === CONFIGURATION ===
ESP32_IP = "http://192.168.1.2"
CAMERA_INDEX = 3
ROTATE_DELAY = 0.25                  # Slightly slower burst rotation
CONFIDENCE_THRESHOLD = 0.3
YOLO_MODEL = "yolov8n.pt"

openai.api_key = "your_api_key"       # Your real key

# === LOAD MODEL ===
model = YOLO(YOLO_MODEL)
CLASSES = model.names

# === HELPER FUNCTIONS ===
def send_command(cmd):
    try:
        print(f"📡 Sending: {cmd}")
        requests.get(f"{ESP32_IP}/{cmd}", timeout=0.5)
    except:
        print(f"❌ Failed to send: {cmd}")

def image_to_base64(img):
    pil = Image.fromarray(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
    buf = BytesIO()
    pil.save(buf, format="JPEG", quality=40)
    return base64.b64encode(buf.getvalue()).decode()

def ask_gpt_to_select_target(frame_b64, crops_b64, desc):
    content = [
        {"type": "text", "text": f"Which of the following is the '{desc}'? Reply with index (0, 1, 2...)"},
        {"type": "image_url", "image_url": {"url": f"data:image/jpeg;base64,{frame_b64}"}}
    ]
    for i, crop in enumerate(crops_b64):
        content.append({"type": "text", "text": f"Option {i}:"})
        content.append({"type": "image_url", "image_url": {"url": f"data:image/jpeg;base64,{crop}"}})

    try:
        res = openai.chat.completions.create(
            model="gpt-4o",
            messages=[
                {"role": "system", "content": "You are an expert at selecting specific objects visually."},
                {"role": "user", "content": content}
            ],
            max_tokens=10
        )
        reply = res.choices[0].message.content.strip()
        print("🧠 GPT chose:", reply)
        return int(reply[0]) if reply[0].isdigit() else None
    except Exception as e:
        print("⚠️ GPT error:", e)
        return None

# === CAMERA SETUP ===
cap = cv2.VideoCapture(CAMERA_INDEX)
if not cap.isOpened():
    print("❌ Camera not accessible.")
    exit()

target_desc = input("🔍 Enter object to follow: ").lower()

try:
    while True:
        # Rotate right in a burst
        send_command("right")
        time.sleep(ROTATE_DELAY)
        send_command("stop")

        # Capture frame after rotation
        ret, frame = cap.read()
        if not ret:
            continue

        h, w = frame.shape[:2]
        grid_l, grid_r = w // 3, 2 * w // 3

        results = model(frame, verbose=False)[0]
        candidates = []

        for box in results.boxes:
            cls = int(box.cls[0])
            label = CLASSES[cls].lower()
            conf = float(box.conf[0])
            if conf > CONFIDENCE_THRESHOLD and target_desc.split()[-1] in label:
                x1, y1, x2, y2 = map(int, box.xyxy[0])
                candidates.append((label, conf, (x1, y1, x2, y2)))

        selected = None
        if len(candidates) == 1:
            selected = candidates[0][2]
        elif len(candidates) > 1:
            print("🤖 Multiple objects — asking GPT")
            crops_b64 = [image_to_base64(frame[y1:y2, x1:x2]) for (_, _, (x1, y1, x2, y2)) in candidates]
            frame_b64 = image_to_base64(frame)
            idx = ask_gpt_to_select_target(frame_b64, crops_b64, target_desc)
            if idx is not None and idx < len(candidates):
                selected = candidates[idx][2]

        if selected:
            x1, y1, x2, y2 = selected
            cx = (x1 + x2) // 2
            cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
            cv2.putText(frame, target_desc, (x1, y1 - 5),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 255, 0), 2)

            if grid_l <= cx <= grid_r:
                send_command("stop")
                time.sleep(0.1)
                send_command("forward")
                print("🎯 Target centered — moving forward")
                continue
            else:
                print("↪️ Target not centered — continue scanning")

        else:
            print("🔄 Target not detected — rotate again")

        # Show feed
        cv2.imshow("Live Feed", frame)
        if cv2.waitKey(1) == 27:
            break

except KeyboardInterrupt:
    print("🛑 Interrupted by user.")
finally:
    send_command("stop")
    cap.release()
    cv2.destroyAllWindows()

