#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

PORT=8080
HOST="localhost"
CONCURRENT=100
REQUESTS=1000

echo -e "${YELLOW}=== Webserv Stress Test ===${NC}"
echo "Target: http://$HOST:$PORT"
echo "Concurrent connections: $CONCURRENT"
echo "Total requests: $REQUESTS"
echo ""

# Test 1: GET stress test
echo -e "${YELLOW}[1/5] GET stress test (index.html)${NC}"
for i in $(seq 1 $REQUESTS); do
    curl -s http://$HOST:$PORT/ > /dev/null &
    if [ $((i % $CONCURRENT)) -eq 0 ]; then
        wait
    fi
done
wait
echo -e "${GREEN}✓ Completed${NC}"

# Test 2: Different pages
echo -e "${YELLOW}[2/5] Testing multiple pages${NC}"
for i in {1..200}; do
    curl -s http://$HOST:$PORT/login.html > /dev/null &
    curl -s http://$HOST:$PORT/upload.html > /dev/null &
    curl -s http://$HOST:$PORT/cgi-test.html > /dev/null &
    if [ $((i % 50)) -eq 0 ]; then
        wait
    fi
done
wait
echo -e "${GREEN}✓ Completed${NC}"

# Test 3: 404 errors
echo -e "${YELLOW}[3/5] Testing 404 responses${NC}"
for i in {1..100}; do
    curl -s http://$HOST:$PORT/nonexistent$i.html > /dev/null &
    if [ $((i % 50)) -eq 0 ]; then
        wait
    fi
done
wait
echo -e "${GREEN}✓ Completed${NC}"

# Test 4: CGI stress
echo -e "${YELLOW}[4/5] CGI stress test${NC}"
for i in {1..50}; do
    curl -s "http://$HOST:$PORT/cgi-bin/hello.php?test=$i" > /dev/null &
    curl -s -X POST -d "username=test$i&password=pass$i" http://$HOST:$PORT/cgi-bin/login.php > /dev/null &
done
wait
echo -e "${GREEN}✓ Completed${NC}"

# Test 5: Keep-alive / rapid succession
echo -e "${YELLOW}[5/5] Rapid succession test${NC}"
for i in {1..500}; do
    curl -s http://$HOST:$PORT/ > /dev/null &
done
wait
echo -e "${GREEN}✓ Completed${NC}"

echo ""
echo -e "${GREEN}=== All tests completed ===${NC}"
echo "Check if server is still responsive:"
RESPONSE=$(curl -s -o /dev/null -w "%{http_code}" http://$HOST:$PORT/)
if [ "$RESPONSE" -eq 200 ]; then
    echo -e "${GREEN}✓ Server still responsive (HTTP $RESPONSE)${NC}"
else
    echo -e "${RED}✗ Server may have issues (HTTP $RESPONSE)${NC}"
fi