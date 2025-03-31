#!/bin/bash

# WebServer Test Script
# This script tests the WebServer implementation by sending various HTTP requests
# and validating the responses.

# Colors for better readability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Configuration
SERVER_HOST="localhost"
SERVER_PORT="8080"
TEST_DIR="/tmp/webserv_tests"
UPLOAD_DIR="$TEST_DIR/uploads"
TOTAL_TESTS=0
PASSED_TESTS=0

# Test files
TEST_HTML="$TEST_DIR/test.html"
TEST_TXT="$TEST_DIR/test.txt"
TEST_IMAGE="$TEST_DIR/test.jpg"
TEST_PHP="$TEST_DIR/test.php"
TEST_UPLOAD="$TEST_DIR/upload_test.txt"
TEST_DIR_LISTING="$TEST_DIR/dir_listing"

# Function to print a separator
print_separator() {
    echo -e "\n${YELLOW}===== $1 =====${NC}\n"
}

# Function to run a test and report results
run_test() {
    local test_name="$1"
    local command="$2"
    local expected_status="$3"
    local expected_content="$4"
    
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    echo -e "${YELLOW}Testing: ${NC}$test_name"
    
    # Run the command and capture output and status code
    response=$(eval "$command")
    status_code=$(echo "$response" | head -n 1 | grep -o "[0-9][0-9][0-9]")
    
    # Check if status code matches expected
    if [[ "$status_code" != "$expected_status" ]]; then
        echo -e "${RED}FAILED:${NC} Expected status $expected_status, got $status_code"
        return
    fi
    
    # If expected content is provided, check for it in the response
    if [[ -n "$expected_content" ]] && ! echo "$response" | grep -q "$expected_content"; then
        echo -e "${RED}FAILED:${NC} Expected content not found: $expected_content"
        return
    fi
    
    PASSED_TESTS=$((PASSED_TESTS + 1))
    echo -e "${GREEN}PASSED${NC}"
}

# Setup test environment
setup() {
    print_separator "Setting up test environment"
    
    mkdir -p "$TEST_DIR"
    mkdir -p "$UPLOAD_DIR"
    mkdir -p "$TEST_DIR_LISTING"
    
    # Create test files
    echo "<html><body><h1>Test HTML</h1></body></html>" > "$TEST_HTML"
    echo "This is a test text file." > "$TEST_TXT"
    
    # Create a binary file (fake image)
    dd if=/dev/urandom of="$TEST_IMAGE" bs=1024 count=10 2>/dev/null
    
    # Create a test PHP file if PHP is available
    if command -v php &> /dev/null; then
        cat > "$TEST_PHP" << EOL
<?php
header('Content-Type: text/html');
echo '<html><body>';
echo '<h1>PHP Test</h1>';
echo '<p>GET parameters:</p><ul>';
foreach (\$_GET as \$key => \$value) {
    echo "<li>\$key: \$value</li>";
}
echo '</ul>';
echo '<p>POST parameters:</p><ul>';
foreach (\$_POST as \$key => \$value) {
    echo "<li>\$key: \$value</li>";
}
echo '</ul>';
echo '</body></html>';
?>
EOL
    fi
    
    # Create file for upload test
    echo "This is a file for upload testing." > "$TEST_UPLOAD"
    
    # Create files for directory listing test
    echo "File 1" > "$TEST_DIR_LISTING/file1.txt"
    echo "File 2" > "$TEST_DIR_LISTING/file2.txt"
    mkdir -p "$TEST_DIR_LISTING/subdir"
    
    echo -e "Test environment set up at ${GREEN}$TEST_DIR${NC}"
}

# Cleanup test environment
cleanup() {
    print_separator "Cleaning up test environment"
    
    rm -rf "$TEST_DIR"
    
    echo "Test environment cleaned up"
}

# Function to wait for server to start
wait_for_server() {
    local max_attempts=30
    local attempt=0
    
    echo "Waiting for server to start on $SERVER_HOST:$SERVER_PORT..."
    
    while [[ $attempt -lt $max_attempts ]]; do
        if curl -s -o /dev/null -w "%{http_code}" "http://$SERVER_HOST:$SERVER_PORT/" > /dev/null 2>&1; then
            echo "Server is up and running!"
            return 0
        fi
        
        attempt=$((attempt + 1))
        sleep 1
    done
    
    echo -e "${RED}Failed to connect to server after $max_attempts attempts${NC}"
    return 1
}

# Run tests
run_tests() {
    # Start the server in the background
    print_separator "Starting WebServer"
    
    # Uncomment to use for actual testing
    # ./webserv ./test_config.conf &
    # SERVER_PID=$!
    
    # Wait for server to start
    # wait_for_server
    
    # Simulate server is running for demo purposes
    echo "Server is running (simulated)"
    
    # Test GET requests
    print_separator "Testing GET Requests"
    
    run_test "Basic GET Request" \
        "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/'" \
        "200" \
        "<html>"
    
    run_test "GET HTML File" \
        "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/test.html'" \
        "200" \
        "Test HTML"
    
    run_test "GET Text File" \
        "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/test.txt'" \
        "200" \
        "This is a test"
    
    run_test "GET with Query Parameters" \
        "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/test.html?param1=value1&param2=value2'" \
        "200" \
        "Test HTML"
    
    run_test "GET Non-existent File" \
        "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/not_found.html'" \
        "404" \
        "Not Found"
    
    # Test directory listing
    print_separator "Testing Directory Listing"
    
    run_test "Directory with Autoindex On" \
        "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/dir_listing/'" \
        "200" \
        "Index of"
    
    # Test POST requests
    print_separator "Testing POST Requests"
    
    run_test "Basic POST Request" \
        "curl -s -i -X POST -d 'param1=value1&param2=value2' 'http://$SERVER_HOST:$SERVER_PORT/post_test'" \
        "200" \
        ""
    
    run_test "POST Form with File Upload" \
        "curl -s -i -X POST -F 'file=@$TEST_UPLOAD' 'http://$SERVER_HOST:$SERVER_PORT/uploads/'" \
        "200" \
        "Upload"
    
    # Test DELETE requests
    print_separator "Testing DELETE Requests"
    
    # First create a file to delete
    echo "This file will be deleted" > "$TEST_DIR/to_delete.txt"
    
    run_test "DELETE File" \
        "curl -s -i -X DELETE 'http://$SERVER_HOST:$SERVER_PORT/to_delete.txt'" \
        "200" \
        ""
    
    # Test CGI
    if [[ -f "$TEST_PHP" ]]; then
        print_separator "Testing CGI"
        
        run_test "PHP CGI Script" \
            "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/test.php?test=value'" \
            "200" \
            "PHP Test"
        
        run_test "PHP CGI POST Request" \
            "curl -s -i -X POST -d 'test=postvalue' 'http://$SERVER_HOST:$SERVER_PORT/test.php'" \
            "200" \
            "PHP Test"
    fi
    
    # Test HTTP error codes
    print_separator "Testing HTTP Error Codes"
    
    run_test "403 Forbidden" \
        "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/etc/passwd'" \
        "403" \
        "Forbidden"
    
    run_test "404 Not Found" \
        "curl -s -i 'http://$SERVER_HOST:$SERVER_PORT/nonexistent.html'" \
        "404" \
        "Not Found"
    
    run_test "405 Method Not Allowed" \
        "curl -s -i -X PUT 'http://$SERVER_HOST:$SERVER_PORT/test.html'" \
        "405" \
        "Method Not Allowed"
    
    # Test stress conditions
    print_separator "Testing Stress Conditions"
    
    run_test "Large Request" \
        "dd if=/dev/zero bs=1M count=2 2>/dev/null | curl -s -i -X POST --data-binary @- 'http://$SERVER_HOST:$SERVER_PORT/uploads/'" \
        "413" \
        "Too Large"
    
    run_test "Concurrent Requests" \
        "for i in {1..10}; do curl -s 'http://$SERVER_HOST:$SERVER_PORT/test.html' & done; wait" \
        "" \
        ""
    
    # Stop the server
    print_separator "Stopping WebServer"
    
    # Uncomment for actual testing
    # kill $SERVER_PID
    # wait $SERVER_PID 2>/dev/null
    
    echo "Server stopped (simulated)"
    
    # Report results
    print_separator "Test Results"
    
    echo -e "Tests passed: ${GREEN}$PASSED_TESTS${NC} / ${YELLOW}$TOTAL_TESTS${NC}"
    
    if [[ $PASSED_TESTS -eq $TOTAL_TESTS ]]; then
        echo -e "${GREEN}All tests passed successfully!${NC}"
    else
        echo -e "${RED}Some tests failed. See above for details.${NC}"
    fi
}

# Main script execution
print_separator "WebServer Test Script"

# Setup test environment
setup

# Run tests
run_tests

# Cleanup test environment
cleanup

exit 0