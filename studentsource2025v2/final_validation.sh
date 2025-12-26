#!/bin/bash

# --- PROJECT CONFIGURATION ---
PORT=1234
LOG_FILE="gateway.log"
CSV_FILE="data.csv"
MAP_FILE="room_sensor.map"
TIMEOUT=5 # Defined at compilation time per Req 4

# Color codes for readability
GREEN='\033[0;32m'
RED='\033[0;31m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${BLUE}--- STARTING FINAL REQUIREMENTS VALIDATION ---${NC}"

# 1. CLEANUP & INITIALIZATION (Req 6 & 8)
# Creates new empty files as required at startup [cite: 76, 83]
rm -f $LOG_FILE $CSV_FILE $MAP_FILE
echo "1 11" > $MAP_FILE
echo "2 12" >> $MAP_FILE
echo "3 13" >> $MAP_FILE

# 2. COMPILATION CHECK (Req 10 / Testing Strategy)
# Strict compilation as defined in Req 10 and testing strategy [cite: 164]
echo -e "\n[CHECK] Strict Compilation (make all)..."
make clean > /dev/null
make all
if [ $? -ne 0 ]; then
    echo -e "${RED}[FAIL] Compilation failed strict flags (-Wall -Werror).${NC}"
    exit 1
fi
echo -e "${GREEN}[PASS] Compiled successfully.${NC}"

# 3. SCENARIO: 3 PARALLEL CLIENTS (Req 3 / Acceptance Criteria)
# Requirement: Handle 3 clients continuously until they disconnect [cite: 62, 63, 183]
echo -e "\n[CHECK] Scenario: 3 Parallel Clients (test3.sh equivalent)..."
./sensor_gateway $PORT 3 &
GATEWAY_PID=$!
sleep 1

# Launch 3 sensor nodes with a few seconds in between [cite: 166]
./sensor_node 11 1 127.0.0.1 $PORT &
sleep 2
./sensor_node 12 2 127.0.0.1 $PORT &
sleep 2
./sensor_node 13 3 127.0.0.1 $PORT &

# Wait for gateway to shut down automatically after 3rd disconnect [cite: 62]
MAX_WAIT=30
while ps -p $GATEWAY_PID > /dev/null && [ $MAX_WAIT -gt 0 ]; do
    sleep 1
    MAX_WAIT=$((MAX_WAIT-1))
done

if ps -p $GATEWAY_PID > /dev/null; then
    echo -e "${RED}[FAIL] Gateway did not shut down automatically.${NC}"
    kill -9 $GATEWAY_PID
else
    echo -e "${GREEN}[PASS] Gateway shut down after processing all data.${NC}"
fi

# 4. DATA VALIDATION (Req 6 & 9)
echo -e "\n[CHECK] Data and Log Requirements..."

# Verify data.csv contents [cite: 75, 182]
if [ -s "$CSV_FILE" ]; then
    echo -e "${GREEN}[PASS] data.csv contains measurements.${NC}"
else
    echo -e "${RED}[FAIL] data.csv is empty or missing.${NC}"
fi

# Verify Log Event: Connection Opened (Req 9.1.a) 
if grep -q "opened a new connection" "$LOG_FILE"; then
    echo -e "${GREEN}[PASS] Connection open log found.${NC}"
else
    echo -e "${RED}[FAIL] Missing 'opened a new connection' log events.${NC}"
fi

# Verify Log Format: <sequence> <timestamp> <message> (Req 8) [cite: 81]
# Simple check if the log line starts with a number (sequence)
FIRST_LINE=$(head -n 1 $LOG_FILE)
if [[ $FIRST_LINE =~ ^[0-9]+ ]]; then
    echo -e "${GREEN}[PASS] Log format appears to follow <sequence> <timestamp> <msg>.${NC}"
else
    echo -e "${RED}[FAIL] Log format incorrect. Current: $FIRST_LINE${NC}"
fi

# 5. PERSISTENCE CHECK (Req 6 & 8)
# Files should not be deleted when server stops [cite: 77, 85]
if [ -f "$CSV_FILE" ] && [ -f "$LOG_FILE" ]; then
    echo -e "${GREEN}[PASS] Files persisted after shutdown.${NC}"
else
    echo -e "${RED}[FAIL] Data or Log files were deleted.${NC}"
fi

echo -e "\n${BLUE}--- VALIDATION COMPLETE ---${NC}"
