#!/bin/bash

# --- SETTINGS ---
ITERATIONS=100
PORT=1234
MAP_FILE="room_sensor.map"
LOG_FILE="gateway.log"
CSV_FILE="data.csv"

echo "--- STARTING 100-ITERATION STRESS TEST ---"
echo "1 10" > $MAP_FILE
echo "2 20" >> $MAP_FILE

# Create a robust python sender script
cat << 'EOF' > sender.py
import socket, struct, time, threading, sys

def send_data(sid, val, port):
    try:
        s = socket.socket()
        s.connect(('127.0.0.1', port))
        for _ in range(5):
            s.send(struct.pack('<Hdq', sid, val, int(time.time())))
            time.sleep(0.01)
        s.close()
    except Exception as e:
        pass

port = int(sys.argv[1])
t1 = threading.Thread(target=send_data, args=(10, 22.0, port))
t2 = threading.Thread(target=send_data, args=(20, 55.0, port))
t1.start(); t2.start()
t1.join(); t2.join()
EOF

for i in $(seq 1 $ITERATIONS)
do
    echo -ne "Testing Iteration: $i/$ITERATIONS \r"
    rm -f $LOG_FILE $CSV_FILE
    
    ./sensor_gateway $PORT 2 > /dev/null 2>&1 &
    GATEWAY_PID=$!
    sleep 0.3

    # Run the robust sender
    python3 sender.py $PORT

    # Wait for shutdown with a timeout
    MAX_WAIT=5
    while ps -p $GATEWAY_PID > /dev/null && [ $MAX_WAIT -gt 0 ]; do
        sleep 1
        MAX_WAIT=$((MAX_WAIT-1))
    done

    if ps -p $GATEWAY_PID > /dev/null; then
        echo -e "\n[ITERATION $i FAILED] Deadlock Detected."
        kill -9 $GATEWAY_PID
        rm sender.py
        exit 1
    fi
done

echo -e "\n--- ALL 100 TESTS PASSED ---"
rm sender.py
