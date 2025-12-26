#!/bin/bash

PORT=1234
MAX_CLIENTS=3
LOG_FILE="gateway.log"
CSV_FILE="data.csv"
MAP_FILE="room_sensor.map"

echo "--- STARTING COMPREHENSIVE DEBUG ---"
killall -9 sensor_gateway 2>/dev/null
rm -f $LOG_FILE $CSV_FILE $MAP_FILE
echo "1 15" > $MAP_FILE
echo "2 20" >> $MAP_FILE

# Start Gateway
./sensor_gateway $PORT $MAX_CLIENTS &
GATEWAY_PID=$!
sleep 2

# CRITICAL CHECK: Is it actually running?
if ! ps -p $GATEWAY_PID > /dev/null; then
    echo "[CRITICAL] Gateway crashed on startup! Check your main() and logger fork."
    exit 1
fi

echo "[DEBUG] Sending valid data (Sensor 15)..."
python3 -c "import socket, struct, time; s = socket.socket(); s.connect(('127.0.0.1', $PORT)); s.send(struct.pack('<Hdq', 15, 22.5, int(time.time()))); s.close()"

echo "[DEBUG] Sending 6 high-temp values (Sensor 20)..."
# Fixed Python syntax for the loop
python3 -c "import socket, struct, time; s = socket.socket(); s.connect(('127.0.0.1', $PORT)); [ (s.send(struct.pack('<Hdq', 20, 50.0, int(time.time()))), time.sleep(0.1)) for _ in range(6) ]; s.close()"

echo "[DEBUG] Sending invalid sensor data (ID 99)..."
python3 -c "import socket, struct, time; s = socket.socket(); s.connect(('127.0.0.1', $PORT)); s.send(struct.pack('<Hdq', 99, 20.0, int(time.time()))); s.close()"

sleep 2
kill -SIGINT $GATEWAY_PID 2>/dev/null
sleep 1

echo "--------------------------------------"
echo "PROBE RESULTS:"
[ -f $LOG_FILE ] && echo "1. Connection Logs: $(grep -c "opened a new connection" $LOG_FILE)" || echo "1. Log file missing"
[ -f $CSV_FILE ] && echo "2. CSV Entries: $(wc -l < $CSV_FILE)" || echo "2. CSV file missing"
[ -f $LOG_FILE ] && echo "3. Hot Alerts: $(grep -c "too hot" $LOG_FILE)" || echo "3. Alerts missing"
[ -f $LOG_FILE ] && echo "4. Invalid IDs: $(grep -c "invalid sensor node ID 99" $LOG_FILE)" || echo "4. Invalid logs missing"
