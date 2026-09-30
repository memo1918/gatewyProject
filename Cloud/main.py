import json
import threading
from datetime import datetime
from flask import Flask, render_template_string, jsonify
import paho.mqtt.client as mqtt

# --- Configuration ---
MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
TOPIC_DATA_UP = "assignment/orange_pi/sensor_data"
TOPIC_CMD_DOWN = "assignment/orange_pi/commands"

app = Flask(__name__)

# Global variable to store the latest data received from the Gateway
latest_data = {"timestamp": "No data yet", "value": "--"}

# --- MQTT Setup ---
def on_connect(client, userdata, flags, rc):
    print(f"[Cloud] Connected to MQTT Broker. Listening on: {TOPIC_DATA_UP}")
    client.subscribe(TOPIC_DATA_UP)

def on_message(client, userdata, msg):
    global latest_data
    payload = msg.payload.decode("utf-8")
    print(f"[Cloud] Received Data: {payload}")
    try:
        # Assuming Gateway sends JSON like: {"sensor_value": 42.5}
        data = json.loads(payload)
        latest_data = {
            "timestamp": datetime.now().strftime("%H:%M:%S"),
            "value": data.get("sensor_value", payload)
        }
    except json.JSONDecodeError:
        latest_data = {
            "timestamp": datetime.now().strftime("%H:%M:%S"),
            "value": payload
        }

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

# Connect and start the MQTT loop in a background thread
mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

# --- Frontend HTML ---
HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Cloud IoT Dashboard</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 50px; background: #f4f4f9; }
        .card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; width: 300px; }
        .data-value { font-size: 2em; font-weight: bold; color: #007bff; margin: 10px 0; }
        .timestamp { color: #888; font-size: 0.9em; margin-bottom: 20px; }
        button { background: #dc3545; color: white; border: none; padding: 10px 20px; font-size: 1.1em; border-radius: 5px; cursor: pointer; }
        button:hover { background: #c82333; }
        button:disabled { background: #ccc; cursor: not-allowed; }
    </style>
</head>
<body>
    <div class="card">
        <h2>Edge Device Status</h2>
        <div class="data-value" id="val">--</div>
        <div class="timestamp">Last Updated: <span id="time">No data yet</span></div>
        <button id="btn" onclick="measureNow()">Measure Now</button>
    </div>

    <script>
        // Fetch the latest data from the Python backend every 1 second
        setInterval(() => {
            fetch('/api/data')
                .then(res => res.json())
                .then(data => {
                    document.getElementById('val').innerText = data.value;
                    document.getElementById('time').innerText = data.timestamp;
                });
        }, 1000);

        // Send the Measure Now command to the backend
        function measureNow() {
            let btn = document.getElementById('btn');
            btn.innerText = "Sending...";
            btn.disabled = true;

            fetch('/api/measure_now', { method: 'POST' })
                .then(res => res.json())
                .then(data => {
                    setTimeout(() => {
                        btn.innerText = "Measure Now";
                        btn.disabled = false;
                    }, 1000);
                });
        }
    </script>
</body>
</html>
"""

# --- Flask Routes ---
@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/api/data')
def get_data():
    return jsonify(latest_data)

@app.route('/api/measure_now', methods=['POST'])
def trigger_measurement():
    print(f"[Cloud] Sending 'Measure Now' command to {TOPIC_CMD_DOWN}")
    # Publish the command to the Gateway
    mqtt_client.publish(TOPIC_CMD_DOWN, json.dumps({"command": "measure_now"}), qos=1)
    return jsonify({"status": "Command sent"})

if __name__ == '__main__':
    print("[System] Starting Cloud Dashboard on http://127.0.0.1:5000")
    # Run the web server
    app.run(host='0.0.0.0', port=5000, debug=False, use_reloader=False)