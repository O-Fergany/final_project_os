#!/ marine/bin/bash

# --- CONFIGURATION ---
PORT=1234
MAX_CLIENTS=3
LOG_FILE="gateway.log"
CSV_FILE="data.csv"
MAP_FILE="room_sensor.map"

echo "--- STARTING DIAGNOSTIC TEST ---"

# 1. Check if map file exists
if [ ! -f "$MAP_FILE" ]; then
    echo "[ERROR] $MAP_FILE missing. DataMgr will fail to initialize."
    exit 1
fi

# 2. Clean old files
rm -f $LOG_FILE $CSV_FILE
touch $LOG_FILE $CSV_FILE

# 3. Start the gateway in the background
echo "[STEP 1] Starting Gateway on port $PORT..."
./sensor_gateway $PORT $MAX_CLIENTS &
GATEWAY_PID=$!
sleep 2

# Check if gateway actually stayed alive
if ! ps -p $GATEWAY_PID > /dev/null; then
    echo "[FAIL] Gateway crashed immediately on startup."
    exit 1
fi

# 4. Check for "Gateway started" log
echo "[STEP 2] Checking logs for startup message..."
if grep -q "Gateway system started" "$LOG_FILE"; then
    echo "[PASS] Startup log found."
else
    echo "[FAIL] Startup log NOT found. Check your pipe/logging logic."
fi

# 5. Send one manual data packet using a dummy sensor (ID 15)
# Using a python one-liner to send binary data: ID(uint16), Value(double), TS(long)
echo "[STEP 3] Sending 1 manual data packet (ID 15, Temp 25.0)..."
python3 -c "import socket, struct, time; \
s = socket.socket(); s.connect(('127.0.0.1', $PORT)); \
s.send(struct.pack('<Hdq', 15, 25.0, int(time.time()))); \
time.sleep(1); s.close()"

# 6. Immediate Analysis
echo "[STEP 4] Analyzing reaction to manual packet..."

# Check Connection Log
if grep -q "Sensor node 15 has opened" "$LOG_FILE"; then
    echo "[PASS] ConnMgr detected connection."
else
    echo "[FAIL] ConnMgr did NOT log connection. Data likely never reached the buffer."
fi

# Check CSV for data
if [ -s "$CSV_FILE" ]; then
    echo "[PASS] StorageMgr wrote data to CSV."
else
    echo "[FAIL] CSV is empty. StorageMgr is stuck or sbuffer_read(reader_id=2) failed."
fi

# 7. Check Alerts (Sending 5 high-temp packets to trigger avg)
echo "[STEP 5] Sending 6 high-temp packets to trigger hot alert..."
python3 -c "import socket, struct, time; \
s = socket.socket(); s.connect(('127.0.0.1', $PORT)); \
for _ in range(6): \
    s.send(struct.pack('<Hdq', 15, 50.0, int(time.time()))); \
    time.sleep(0.1); \
s.close()"
sleep 2

if grep -i "too hot" "$LOG_FILE"; then
    echo "[PASS] DataMgr alert found."
else
    echo "[FAIL] No alerts found. DataMgr is either stuck or calculation logic is wrong."
fi

# 8. Shutdown
echo "[STEP 6] Shutting down..."
kill -SIGINT $GATEWAY_PID 2>/dev/null
sleep 2

echo "--- DIAGNOSTIC COMPLETE ---"
