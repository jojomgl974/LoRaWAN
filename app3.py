from flask import Flask, render_template, jsonify, request
import paho.mqtt.client as mqtt
import json
import base64

# ======================================================
# CONFIG TTN (ADAPTE SI BESOIN)
# ======================================================
APP_ID = "my-new-la66"          # Application TTN
API_KEY = "NNSXS.A5YLTCBQH5OJANY3ZIM2TXYQ5EDNUPQQ4XNXTOQ.JUVLQJEQKERAULL6N2A7GA5V2O5QINVALOIBCPWJMK2RFLTPYCHA"               # API KEY
REGION = "eu1"
DEVICE_ID = "la66-noeud-1"

broker = f"{REGION}.cloud.thethings.network"
port = 8883

topic_uplink = f"v3/{APP_ID}@ttn/devices/{DEVICE_ID}/up"
topic_downlink = f"v3/{APP_ID}@ttn/devices/{DEVICE_ID}/down/push"

# ======================================================
# VARIABLES GLOBALES
# ======================================================
last_payload = {
    "raw_bytes": [],
    "ascii": ""
}

# ======================================================
# MQTT CALLBACKS
# ======================================================
def on_connect(client, userdata, flags, rc):
    print("Connecté à TTN")
    client.subscribe(topic_uplink)

def on_message(client, userdata, msg):
    global last_payload
    try:
        data = json.loads(msg.payload.decode())
        uplink = data.get("uplink_message", {})

        raw_bytes = uplink.get("decoded_payload", {}).get("raw_bytes", [])

        if not raw_bytes:
            frm = uplink.get("frm_payload", "")
            raw_bytes = list(base64.b64decode(frm)) if frm else []

        ascii_text = "".join(chr(b) for b in raw_bytes if 32 <= b <= 126)
        
        hex_text = " ".join(f"{b:02X}" for b in raw_bytes)

        last_payload = {
            "raw_bytes": raw_bytes,   # décimal
            "hex": hex_text,          # hexadécimal
            "ascii": ascii_text       # texte lisible

        }

        print("Uplink reçu :", last_payload)

    except Exception as e:
        print("Erreur uplink :", e)

# ======================================================
# MQTT SETUP
# ======================================================
mqtt_client = mqtt.Client()
mqtt_client.username_pw_set(APP_ID, API_KEY)
mqtt_client.tls_set()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message
mqtt_client.connect(broker, port)
mqtt_client.loop_start()

# ======================================================
# FONCTION DOWNLINK
# ======================================================
def send_downlink(raw_bytes, fport):
    payload = {
        "downlinks": [
            {
                "f_port": fport,
                "frm_payload": base64.b64encode(bytes(raw_bytes)).decode(),
                "confirmed": False,
                "priority": "NORMAL"
            }
        ]
    }
    mqtt_client.publish(topic_downlink, json.dumps(payload))
    print("Downlink envoyé :", payload)

# ======================================================
# FLASK SERVER
# ======================================================
app = Flask(__name__)

@app.route("/")
def index():
    return render_template("index5.html")

@app.route("/uplink")
def uplink():
    return jsonify(last_payload)

@app.route("/downlink", methods=["POST"])
    
def downlink():
    data = request.json

    # Texte ASCII reçu depuis la page HTML
    ascii_text = data.get("ascii", "")

    # FPort LoRaWAN
    fport = int(data.get("f_port", 10))

    # Conversion ASCII → bytes décimaux
    raw_bytes = [ord(c) for c in ascii_text]

    # Envoi du downlink
    send_downlink(raw_bytes, fport)

    return jsonify({
        "status": "OK",
        "ascii_sent": ascii_text,
        "raw_bytes": raw_bytes
    })

if __name__ == "__main__":
    app.run(host="0.0.0.0", port=5000)
