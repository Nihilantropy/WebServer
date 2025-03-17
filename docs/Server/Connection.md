# Detailed Explanation of the Connection Class

## 1. Connection Class Purpose and Overview

The Connection class represents a client connection to our web server and manages the entire lifecycle of an HTTP transaction. This class is responsible for:

- Reading HTTP requests from clients using non-blocking I/O
- Processing these requests according to HTTP protocol rules
- Generating appropriate HTTP responses
- Writing responses back to clients
- Managing connection state and timeouts
- Properly closing connections when needed

The Connection class implements a state machine that transitions through various stages of an HTTP transaction, ensuring that all I/O operations are performed in a non-blocking manner through the IOMultiplexer.

## 2. Class Attributes Explained

```cpp
private:
    int _clientFd;                  // Client socket file descriptor
    struct sockaddr_in _clientAddr; // Client address information
    std::string _clientIp;          // Client IP address (for logging)
    ServerConfig* _serverConfig;    // Server configuration to use
    
    std::string _inputBuffer;       // Buffer for incoming data
    std::string _outputBuffer;      // Buffer for outgoing data
    
    time_t _lastActivity;           // Time of last activity (for timeout)
    ConnectionState _state;         // Current connection state
    
    // Connection timeout in seconds
    static const time_t CONNECTION_TIMEOUT = 60;
```

- **_clientFd**: File descriptor for the client socket, representing the network connection
- **_clientAddr**: Structure containing the client's network address information
- **_clientIp**: Human-readable string of the client's IP address (used for logging)
- **_serverConfig**: Pointer to the server configuration that should handle this connection
- **_inputBuffer**: String buffer that accumulates data read from the client
- **_outputBuffer**: String buffer that holds data to be written to the client
- **_lastActivity**: Timestamp of the most recent activity on this connection (used for timeout detection)
- **_state**: Current state in the connection's lifecycle state machine
- **CONNECTION_TIMEOUT**: Constant defining how long a connection can be idle before being closed (60 seconds)

## 3. Connection State Machine

The Connection class implements a state machine with the following states:

```cpp
enum ConnectionState {
    READING_HEADERS,  // Reading HTTP headers
    READING_BODY,     // Reading HTTP body
    PROCESSING,       // Processing the request
    SENDING_RESPONSE, // Sending response
    CLOSED            // Connection closed
};
```

This state machine manages the progression of an HTTP transaction:

1. **READING_HEADERS**: Initial state, reading HTTP request headers until the headers are complete
2. **READING_BODY**: Reading the HTTP request body (if present)
3. **PROCESSING**: Processing the complete HTTP request
4. **SENDING_RESPONSE**: Sending the HTTP response back to the client
5. **CLOSED**: Connection has been closed or encountered an error

The state transitions happen based on events like:
- Complete headers being received
- Complete body being received
- Request processing being completed
- Response being fully sent
- Errors or timeouts occurring

## 4. Constructor and Destructor

```cpp
Connection::Connection(int clientFd, struct sockaddr_in clientAddr, ServerConfig* config)
    : _clientFd(clientFd), _clientAddr(clientAddr), _serverConfig(config),
      _inputBuffer(), _outputBuffer(), _state(READING_HEADERS)
{
    // Convert binary address to string for logging
    char ipBuffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &_clientAddr.sin_addr, ipBuffer, INET_ADDRSTRLEN);
    _clientIp = ipBuffer;
    
    _updateLastActivity();
    
    std::cout << "New connection from " << _clientIp << " (fd: " << _clientFd << ")" << std::endl;
}

Connection::~Connection()
{
    close();
}
```

- **Constructor**:
  - Initializes the connection with the client socket and server configuration
  - Converts the binary IP address to a string for logging
  - Sets the initial state to READING_HEADERS
  - Updates the last activity timestamp
  - Logs information about the new connection
  
- **Destructor**:
  - Calls close() to ensure the socket is properly closed
  - Prevents resource leaks by always cleaning up the file descriptor

## 5. Core I/O Methods

### readData()

```cpp
bool Connection::readData()
{
    if (_state == CLOSED) {
        return false;
    }
    
    // Make sure we only read when we should
    if (_state != READING_HEADERS && _state != READING_BODY) {
        return true;
    }
    
    char buffer[4096];
    ssize_t bytesRead;
    
    // Read data from socket using non-blocking I/O
    bytesRead = recv(_clientFd, buffer, sizeof(buffer), 0);
    
    if (bytesRead > 0) {
        // Append read data to input buffer
        _inputBuffer.append(buffer, bytesRead);
        _updateLastActivity();
        
        std::cout << "Read " << bytesRead << " bytes from " << _clientIp << std::endl;
        
        // Process data based on current state
        if (_state == READING_HEADERS) {
            if (_parseHttpHeaders()) {
                if (_state == READING_BODY) {
                    // Continue reading body if needed
                    return true;
                } else {
                    // Headers complete, no body expected
                    _state = PROCESSING;
                    process();
                }
            }
        } else if (_state == READING_BODY) {
            if (_parseHttpBody()) {
                // Body complete
                _state = PROCESSING;
                process();
            }
        }
        
        return true;
    } else if (bytesRead == 0) {
        // Connection closed by client
        std::cout << "Connection closed by client: " << _clientIp << std::endl;
        _state = CLOSED;
        return false;
    } else {
        // Error or would block
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, try again later
            return true;
        } else {
            // Error
            std::cerr << "Error reading from socket " << _clientFd << ": " << strerror(errno) << std::endl;
            _state = CLOSED;
            return false;
        }
    }
}
```

- **Purpose**: Reads available data from the client socket
- **Return value**: Boolean indicating if the connection is still valid
- **Key aspects**:
  - Only reads in appropriate states (READING_HEADERS, READING_BODY)
  - Uses non-blocking I/O with proper handling of EAGAIN/EWOULDBLOCK
  - Accumulates read data in the input buffer
  - Updates last activity timestamp
  - Processes headers and body as they become complete
  - Transitions to the next state when appropriate
  - Handles connection closure and errors

### writeData()

```cpp
bool Connection::writeData()
{
    if (_state == CLOSED) {
        return false;
    }
    
    // Make sure we only write when we should
    if (_state != SENDING_RESPONSE || _outputBuffer.empty()) {
        return true;
    }
    
    ssize_t bytesWritten = send(_clientFd, _outputBuffer.c_str(), _outputBuffer.size(), 0);
    
    if (bytesWritten > 0) {
        _updateLastActivity();
        
        std::cout << "Wrote " << bytesWritten << " bytes to " << _clientIp << std::endl;
        
        // Remove sent data from buffer
        _outputBuffer.erase(0, bytesWritten);
        
        // If all data sent, move to next state
        if (_outputBuffer.empty()) {
            // For HTTP/1.0 or "Connection: close", close after response
            // For HTTP/1.1 with "Connection: keep-alive", go back to reading headers
            
            // TODO: Implement proper HTTP connection handling based on headers
            // For now, we'll keep the connection open and go back to reading headers
            _state = READING_HEADERS;
            _inputBuffer.clear();
        }
        
        return true;
    } else if (bytesWritten == 0) {
        // Socket closed
        std::cout << "Connection closed during write: " << _clientIp << std::endl;
        _state = CLOSED;
        return false;
    } else {
        // Error or would block
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Socket buffer full, try again later
            return true;
        } else {
            // Error
            std::cerr << "Error writing to socket " << _clientFd << ": " << strerror(errno) << std::endl;
            _state = CLOSED;
            return false;
        }
    }
}
```

- **Purpose**: Writes pending data to the client socket
- **Return value**: Boolean indicating if the connection is still valid
- **Key aspects**:
  - Only writes in appropriate state (SENDING_RESPONSE) and when there's data to send
  - Uses non-blocking I/O with proper handling of EAGAIN/EWOULDBLOCK
  - Removes sent data from the output buffer
  - Updates last activity timestamp
  - Transitions back to READING_HEADERS when response is fully sent (for keep-alive connections)
  - Handles connection closure and errors

## 6. HTTP Processing Methods

### process()

```cpp
void Connection::process()
{
    if (_state != PROCESSING) {
        return;
    }
    
    std::cout << "Processing request from " << _clientIp << std::endl;
    
    _processRequest();
    _generateResponse();
    
    _state = SENDING_RESPONSE;
}
```

- **Purpose**: Main entry point for processing a complete HTTP request
- **Behavior**:
  - Calls _processRequest() to interpret the HTTP request
  - Calls _generateResponse() to create an appropriate HTTP response
  - Transitions to SENDING_RESPONSE state

### _parseHttpHeaders()

```cpp
bool Connection::_parseHttpHeaders()
{
    // Look for end of headers (double CRLF)
    size_t headerEnd = _inputBuffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // Not enough data yet
        return false;
    }
    
    // We have complete headers
    std::string headers = _inputBuffer.substr(0, headerEnd + 4);
    _inputBuffer.erase(0, headerEnd + 4);
    
    std::cout << "Received complete HTTP headers from " << _clientIp << std::endl;
    
    // TODO: Actually parse the headers to extract method, URI, version, etc.
    // This will be implemented in Phase 3 (HTTP Protocol Implementation)
    
    // For now, check if we expect a body (based on presence of Content-Length or Transfer-Encoding)
    // This is a simplified check - HTTP parsing will be more complex in the real implementation
    if (headers.find("Content-Length:") != std::string::npos || 
        headers.find("Transfer-Encoding: chunked") != std::string::npos) {
        _state = READING_BODY;
        return true;
    }
    
    // No body expected
    return true;
}
```

- **Purpose**: Parses HTTP headers from the input buffer
- **Return value**: Boolean indicating if headers are complete
- **Key aspects**:
  - Identifies header boundary using double CRLF
  - Extracts and removes headers from the input buffer
  - Determines if a request body is expected
  - Sets state to READING_BODY if a body is expected
  - Contains placeholders for full HTTP header parsing (to be implemented)

### _parseHttpBody()

```cpp
bool Connection::_parseHttpBody()
{
    // TODO: Implement body parsing based on Content-Length or chunked encoding
    // This will be implemented in Phase 3 (HTTP Protocol Implementation)
    
    // For now, assume we have the full body
    std::cout << "Received HTTP body from " << _clientIp << std::endl;
    return true;
}
```

- **Purpose**: Parses HTTP request body from the input buffer
- **Return value**: Boolean indicating if body parsing is complete
- **Behavior**:
  - Currently a placeholder for future implementation
  - Will handle both Content-Length and chunked transfer encoding
  - Currently assumes body is complete for prototype purposes

### _processRequest() and _generateResponse()

```cpp
void Connection::_processRequest()
{
    // TODO: Implement actual request processing based on the HTTP method, URI, etc.
    // This will be implemented in Phase 4 (Request Handlers)
    
    // For now, we'll just prepare a simple response
    _generateResponse();
}

void Connection::_generateResponse()
{
    // TODO: Generate proper HTTP response based on request
    // This will be implemented in Phase 3 (HTTP Protocol Implementation)
    
    // For now, just generate a simple "Hello World" response
    std::string responseBody = "<h1>Hello from WebServer!</h1>\n"
                              "<p>Your IP address is: " + _clientIp + "</p>\n";
    
    std::stringstream lengthStr;
    lengthStr << responseBody.size();
    
    _outputBuffer = "HTTP/1.1 200 OK\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-Length: " + lengthStr.str() + "\r\n"
                    "Connection: keep-alive\r\n"
                    "\r\n" + 
                    responseBody;
}
```

- **_processRequest()**:
  - Placeholder for future implementation of HTTP request processing
  - Will handle different HTTP methods (GET, POST, DELETE)
  - Will route requests to appropriate handlers
  
- **_generateResponse()**:
  - Generates an HTTP response and stores it in the output buffer
  - Currently creates a simple "Hello World" response
  - Will be expanded to generate proper responses based on request

## 7. Connection Management Methods

### _updateLastActivity()

```cpp
void Connection::_updateLastActivity()
{
    _lastActivity = time(NULL);
}
```

- **Purpose**: Updates the timestamp of the last activity on this connection
- **Usage**: Called whenever reading or writing occurs
- **Importance**: Used to detect idle connections for timeout management

### isTimeout()

```cpp
bool Connection::isTimeout() const
{
    // Check if the connection has been idle for too long
    time_t now = time(NULL);
    return (now - _lastActivity) > CONNECTION_TIMEOUT;
}
```

- **Purpose**: Checks if the connection has been idle for too long
- **Return value**: Boolean indicating if the connection has timed out
- **Behavior**: Compares current time with last activity time against timeout threshold
- **Usage**: Called periodically by the server to close idle connections

### close()

```cpp
void Connection::close()
{
    if (_clientFd >= 0) {
        ::close(_clientFd);
        _clientFd = -1;
    }
    _state = CLOSED;
}
```

- **Purpose**: Closes the client socket and marks the connection as closed
- **Behavior**:
  - Checks if socket is valid before closing
  - Closes the socket using the system close() function
  - Sets file descriptor to -1 to prevent double-closing
  - Sets state to CLOSED
- **Usage**: Called when connection is finished, encounters an error, or times out

## 8. Helper Methods for Event Management

### shouldRead() and shouldWrite()

```cpp
bool Connection::shouldRead() const
{
    // We should monitor for read events when:
    // - Reading headers
    // - Reading body
    return (_state == READING_HEADERS || _state == READING_BODY) && _state != CLOSED;
}

bool Connection::shouldWrite() const
{
    // We should monitor for write events when:
    // - In SENDING_RESPONSE state
    // - Have data in the output buffer
    return _state == SENDING_RESPONSE && !_outputBuffer.empty() && _state != CLOSED;
}
```

- **Purpose**: Determine if the connection should be monitored for read/write events
- **Return value**: Boolean indicating if monitoring is needed
- **Behavior**:
  - shouldRead() returns true in states where we need to read from the client
  - shouldWrite() returns true when we have data to send and are in the right state
  - Both return false if the connection is closed
- **Usage**: Called by the Server class to decide which events to monitor with poll()

## 9. Key Design Decisions

### 1. State Machine Architecture

The Connection class uses a state machine to track progress through the HTTP request/response cycle. This approach:
- Keeps track of where we are in the HTTP transaction
- Ensures we only perform appropriate operations in each state
- Allows for clean transitions between different phases
- Makes it easy to expand with more states if needed

### 2. Non-blocking I/O

All I/O operations are designed to be non-blocking:
- Never blocks waiting for data to be available
- Handles EAGAIN/EWOULDBLOCK appropriately
- Returns control to the main event loop when operations would block
- Updates the IOMultiplexer with current read/write needs

### 3. Buffer Management

The class uses string buffers for incoming and outgoing data:
- Accumulates partial reads until complete headers/body are available
- Tracks how much of the output buffer has been sent
- Properly handles partial writes when socket buffers are full
- Clears buffers when appropriate for connection reuse

### 4. Connection Reuse

The implementation supports HTTP keep-alive:
- Transitions back to READING_HEADERS after sending a response
- Clears input buffer for the next request
- Maintains the same socket for multiple HTTP transactions
- Updates activity timestamp to prevent premature timeouts

## 10. Why This Design Matters for Our Web Server

- **Non-blocking I/O**: Satisfies the project requirement that the server never blocks
- **State Machine**: Enables clear, maintainable code for HTTP transaction processing
- **Buffer Management**: Handles partial reads/writes properly in a non-blocking environment
- **Connection Reuse**: Improves performance by avoiding repeated connection setup
- **Timeout Handling**: Prevents resource exhaustion from idle connections
- **Clean Separation**: Isolates HTTP protocol handling from server management

This design ensures our web server can efficiently handle many concurrent connections while maintaining clean, maintainable code. The Connection class forms the core of our HTTP processing pipeline, with clear hooks for future implementation of HTTP parsing and request handling.