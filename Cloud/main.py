import threading
from datetime import datetime
from flask import Flask, render_template_string, jsonify
import paho.mqtt.client as mqtt

MQTT_BROKER = "broker.hivemq.com"
MQTT_PORT = 1883
TOPIC_DATA_UP = "assignment/orange_pi/sensor_data"
TOPIC_CMD_DOWN = "assignment/orange_pi/commands"

app = Flask(__name__)

# Store the last 50 readings for the graph
data_history = []

def on_connect(client, userdata, flags, rc):
    print(f"[Cloud] Connected to MQTT Broker. Listening on: {TOPIC_DATA_UP}")
    client.subscribe(TOPIC_DATA_UP)

def on_message(client, userdata, msg):
    global data_history
    payload = msg.payload.decode("utf-8")
    print(f"[Cloud] Received Data: {payload}")
    
    try:
        # Check if the Gateway sent a timestamp (e.g., "41.25,1727751045")
        if ',' in payload:
            val_str, timestamp_str = payload.split(',')
            val = float(val_str)
            # Convert UNIX timestamp to HH:MM:SS
            dt = datetime.fromtimestamp(int(timestamp_str))
            ts = dt.strftime("%H:%M:%S")
        else:
            # Fallback if Gateway only sends "41.25"
            val = float(payload)
            ts = datetime.now().strftime("%H:%M:%S")

        data_history.append({"timestamp": ts, "value": val})
        
        # Keep only the latest 50 points to prevent memory issues
        if len(data_history) > 50:
            data_history.pop(0)
            
    except ValueError as e:
        print(f"[Cloud] Error parsing data: {e}")

mqtt_client = mqtt.Client()
mqtt_client.on_connect = on_connect
mqtt_client.on_message = on_message

mqtt_client.connect(MQTT_BROKER, MQTT_PORT, 60)
mqtt_client.loop_start()

# --- Frontend HTML with Chart.js ---
HTML_TEMPLATE = """
<!DOCTYPE html>
<html>
<head>
    <title>Cloud IoT Dashboard</title>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; margin-top: 20px; background: #f4f4f9; }
        .card { background: white; padding: 20px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: inline-block; width: 80%; max-width: 800px; }
        .data-value { font-size: 2.5em; font-weight: bold; color: #007bff; margin: 10px 0; }
        button { background: #dc3545; color: white; border: none; padding: 10px 20px; font-size: 1.1em; border-radius: 5px; cursor: pointer; margin-bottom: 20px;}
        button:hover { background: #c82333; }
        button:disabled { background: #ccc; cursor: not-allowed; }
        canvas { max-width: 100%; margin-top: 20px; }
    </style>
</head>
<body>
    <div class="card">
        <h2>Edge Device Status</h2>
        <div class="data-value" id="val">--</div>
        <button id="btn" onclick="measureNow()">Measure Now</button>
        
        <canvas id="sensorChart"></canvas>
    </div>

    <script>
        // Setup Chart.js
        const ctx = document.getElementById('sensorChart').getContext('2d');
        const sensorChart = new Chart(ctx, {
            type: 'line',
            data: {
                labels: [],
                datasets: [{
                    label: 'Sensor Value',
                    data: [],
                    borderColor: '#007bff',
                    backgroundColor: 'rgba(0, 123, 255, 0.1)',
                    borderWidth: 2,
                    fill: true,
                    tension: 0.3
                }]
            },
            options: {
                responsive: true,
                scales: {
                    y: { beginAtZero: false }
                },
                animation: { duration: 0 } // Disable animation for instant updates
            }
        });

        // Fetch the latest history every 1 second
        setInterval(() => {
            fetch('/api/history')
                .then(res => res.json())
                .then(history => {
                    if (history.length > 0) {
                        // Update the large number text
                        document.getElementById('val').innerText = history[history.length - 1].value;
                        
                        // Update the chart arrays
                        sensorChart.data.labels = history.map(point => point.timestamp);
                        sensorChart.data.datasets[0].data = history.map(point => point.value);
                        sensorChart.update();
                    }
                });
        }, 1000);

        function measureNow() {
            let btn = document.getElementById('btn');
            btn.innerText = "Sending...";
            btn.disabled = true;

            fetch('/api/measure_now', { method: 'POST' })
                .then(() => {
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

@app.route('/')
def index():
    return render_template_string(HTML_TEMPLATE)

@app.route('/api/history')
def get_history():
    return jsonify(data_history)

@app.route('/api/measure_now', methods=['POST'])
def trigger_measurement():
    print(f"[Cloud] Sending 'Measure Now' command to {TOPIC_CMD_DOWN}")
    mqtt_client.publish(TOPIC_CMD_DOWN, '{"command": "measure_now"}', qos=1)
    return jsonify({"status": "Command sent"})

if __name__ == '__main__':
    print("[System] Starting Cloud Dashboard on http://127.0.0.1:5000")
    app.run(host='0.0.0.0', port=5000, debug=False, use_reloader=False)