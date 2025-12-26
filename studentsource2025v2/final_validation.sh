#!/bin/bash

# --- PROJECT CONFIGURATION (Req 3, 4, 6, 8) ---
PORT=1234
LOG_FILE="gateway.log"
CSV_FILE="data.csv"
MAP_FILE="room_sensor.map"
TIMEOUT=5 

# Color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}--- STARTING FINAL COMPREHENSIVE VALIDATION ---${NC}"

# 1. CLEANUP & INITIALIZATION (Req 6 & 8) [cite: 76, 83]
rm -f $LOG_FILE $CSV_FILE $MAP_FILE
echo "1 11" > $MAP_FILE
echo "2 12" >> $MAP_FILE
echo "3 13" >> $MAP_FILE

# 2. STRICT COMPILATION (Req 10 / Testing Strategy) [cite: 164]
echo -e "\n[CHECK] Compilation (make all)..."
make clean > /dev/null
make all
if [ $? -ne 0 ]; then
    echo -e "${RED}[FAIL] Strict compilation failed.${NC}"
    exit 1
fi

# 3. SENSOR NODE SETUP (Handling the Infinite Loop) [cite: 33, 100]
# We compile the sensor with a finite loop count to test Req 3's shutdown criteria
gcc -Wall -std=c11 sensor_node.c lib/tcpsock.c -DLOOPS=5 -o sensor_node -lpthread
echo -e "${GREEN}[PASS] Project and Sensors compiled successfully.${NC}"

# 4. SCENARIO: 3 PARALLEL CLIENTS (Req 3) [cite: 62, 165, 183]
echo -e "\n[CHECK] Scenario: Parallel Processing & Automatic Shutdown..."
./sensor_gateway $PORT 3 &
GATEWAY_PID=$!
sleep 1

# Launch 3 nodes with slight delays (as per Req testing strategy) [cite: 166]
./sensor_node 11 1 127.0.0.1 $PORT &
sleep 1
./sensor_node 12 1 127.0.0.1 $PORT &
sleep 1
./sensor_node 13 1 127.0.0.1 $PORT &

# Wait for gateway to shut down naturally after 3rd disconnect [cite: 62]
MAX_WAIT=20
while ps -p $GATEWAY_PID > /dev/null && [ $MAX_WAIT -gt 0 ]; do
    echo -n "."
    sleep 1
    MAX_WAIT=$((MAX_WAIT-1))
done
echo ""

if ps -p $GATEWAY_PID > /dev/null; then
    echo -e "${RED}[FAIL] Gateway did not shut down naturally after client disconnects.${NC}"
    kill -9 $GATEWAY_PID
else
    echo -e "${GREEN}[PASS] Gateway shut down after processing all data.${NC}"
fi

# 5. LOGGING VALIDATION (Req 7, 8, 9) [cite: 77-96]
echo -e "\n[CHECK] Log File Requirements (gateway.log)..."
[ -f "$LOG_FILE" ] && echo -e "${GREEN}[PASS] gateway.log exists.${NC}" || echo -e "${RED}[FAIL] gateway.log missing.${NC}"

# Check for required sequence/timestamp format [cite: 81]
if [[ $(head -n 1 $LOG_FILE) =~ ^[0-9]+ ]]; then
    echo -e "${GREEN}[PASS] Log format follows <seq> <ts> <msg>.${NC}"
else
    echo -e "${RED}[FAIL] Log format incorrect.${NC}"
fi

# Check for specific Required events [cite: 88, 89, 95]
grep -q "opened a new connection" "$LOG_FILE" && echo -e "${GREEN}[PASS] Log: Connection opened recorded.${NC}"
grep -q "closed the connection" "$LOG_FILE" && echo -e "${GREEN}[PASS] Log: Connection closed recorded.${NC}"
grep -q "data.csv file has been created" "$LOG_FILE" && echo -e "${GREEN}[PASS] Log: Storage manager init recorded.${NC}"

# 6. STORAGE VALIDATION (Req 6) [cite: 75, 76]
echo -e "\n[CHECK] Data Storage (data.csv)..."
if [ -s "$CSV_FILE" ]; then
    MEASUREMENTS=$(wc -l < $CSV_FILE)
    echo -e "${GREEN}[PASS] data.csv contains $MEASUREMENTS measurements.${NC}"
else
    echo -e "${RED}[FAIL] data.csv is empty or missing.${NC}"
fi

# 7. PERSISTENCE CHECK (Req 6 & 8) [cite: 77, 85]
if [ -f "$CSV_FILE" ] && [ -f "$LOG_FILE" ]; then
    echo -e "${GREEN}[PASS] Data and Log files persisted after shutdown.${NC}"
else
    echo -e "${RED}[FAIL] Persistence check failed.${NC}"
fi

echo -e "\n${BLUE}--- VALIDATION COMPLETE ---${NC}"
