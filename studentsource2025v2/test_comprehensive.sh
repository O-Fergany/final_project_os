#!/bin/bash

# Configuration
PORT=5678
TIMEOUT=5
LOG_FILE="gateway.log"
DATA_FILE="data.csv"

# 1. Clean and Build (Req: strict compilation -Wall -std=c11 -Werror)
echo "--- Step 1: Strict Compilation ---"
make clean
make all
if [ $? -ne 0 ]; then
    echo "[FAIL] Compilation failed under strict rules."
    exit 1
fi
echo "[PASS] Compiled successfully."

# 2. Test Scenario A: 3 Parallel Clients (The Bare Minimum)
echo -e "\n--- Step 2: Testing Scenario A (3 Parallel Clients) ---"
# Start gateway in background
./sensor_gateway $PORT 3 &
GATEWAY_PID=$!

# Launch 3 clients with a few seconds delay (Req 11)
./sensor_node 15 $TIMEOUT 127.0.0.1 $PORT &
sleep 2
./sensor_node 21 $TIMEOUT 127.0.0.1 $PORT &
sleep 2
./sensor_node 37 $TIMEOUT 127.0.0.1 $PORT &

# Wait for gateway to finish (it should shut down after 3rd client disconnects)
wait $GATEWAY_PID
echo "[DONE] Scenario A finished."

# 3. Test Scenario B: 5 Parallel Clients (Rapid Fire)
echo -e "\n--- Step 3: Testing Scenario B (5 Parallel Clients) ---"
./sensor_gateway $PORT 5 &
GATEWAY_PID=$!

# Launch 5 clients immediately after each other (Req 11)
for i in {1..5}
do
   ./sensor_node $((10+i)) $TIMEOUT 127.0.0.1 $PORT &
done

wait $GATEWAY_PID
echo "[DONE] Scenario B finished."

# 4. Final Validation (Acceptance Criteria)
echo -e "\n--- Step 4: Validating Output ---"

if [ -f "$DATA_FILE" ] && [ -s "$DATA_FILE" ]; then
    echo "[PASS] $DATA_FILE exists and contains measurements."
else
    echo "[FAIL] $DATA_FILE is missing or empty."
fi

if [ -f "$LOG_FILE" ] && [ -s "$LOG_FILE" ]; then
    echo "[PASS] $LOG_FILE exists and contains log events."
    # Check for specific required log strings (Req 9)
    grep -q "opened a new connection" $LOG_FILE && echo "  - Found: Connection opened logs"
    grep -q "closed the connection" $LOG_FILE && echo "  - Found: Connection closed logs"
    grep -q "data.csv file has been created" $LOG_FILE && echo "  - Found: Storage manager init logs"
else
    echo "[FAIL] $LOG_FILE is missing or empty."
fi

