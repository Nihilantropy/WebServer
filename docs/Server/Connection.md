# Detailed Explanation of the Connection Class (Updated)

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
    
    // HTTP request and response objects
    Request _request;
    Response _response;
    
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
- **_request**: HTTP Request object that parses and stores the request information
- **_response**: HTTP Response object for generating the response
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
      _inputBuffer(), _outputBuffer(), _state(READING_HEADERS),
      _request(), _response()
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
  - Initializes the Request and Response objects
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
            if (_request.parseHeaders(_inputBuffer)) {
                // Headers complete, check if we need to read the body
                if (_request.isComplete()) {
                    // No body expected, move to processing
                    _state = PROCESSING;
                    process();
                } else {
                    // Body expected, move to reading body
                    _state = READING_BODY;
                }
            }
            else
                std::cout << "EVVIVA!\n";
        } else if (_state == READING_BODY) {
            if (_request.parseBody(_inputBuffer)) {
                // Body complete, move to processing
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
  - Delegates HTTP parsing to the Request object
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
            // Mark the response as sent
            _response.markAsSent();
            
            // Check connection header
            bool keepAlive = _request.getHeaders().keepAlive(true);
            
            if (keepAlive) {
                // Reset for the next request
                _request.reset();
                _state = READING_HEADERS;
                _inputBuffer.clear();
            } else {
                // Close connection after response
                _state = CLOSED;
            }
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
  - Handles keep-alive connection management based on HTTP headers
  - Resets for the next request if keep-alive is enabled
  - Handles connection closure and errors

## 6. HTTP Processing Methods

### process()

```cpp
void Connection::process()
{
    if (_state != PROCESSING) {
        return;
    }
    
    std::cout << "Processing " << _request.getMethodStr() << " request for " 
              << _request.getPath() << " from " << _clientIp << std::endl;
    
    // Create a new response for this request
    _response = Response();
    
    try {
        // Process the request and generate a response
        _processRequest();
    } catch (const std::exception& e) {
        // Handle any exceptions during processing
        std::cerr << "Error processing request: " << e.what() << std::endl;
        _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    
    // Build the response and store in output buffer
    _outputBuffer = _response.build();
    
    // Move to sending response state
    _state = SENDING_RESPONSE;
}
```

- **Purpose**: Main entry point for processing a complete HTTP request
- **Behavior**:
  - Creates a new Response object for this request
  - Calls _processRequest() to process the request
  - Catches any exceptions and generates an error response if needed
  - Builds the response and stores it in the output buffer
  - Transitions to SENDING_RESPONSE state

### _processRequest()

```cpp
void Connection::_processRequest()
{
    // Check if method is supported
    Request::Method method = _request.getMethod();
    if (method == Request::UNKNOWN) {
        _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
        return;
    }
    
    // Check if the requested method is allowed for this location
    std::string methodStr = _request.getMethodStr();
    bool methodAllowed = false;
    
    // TODO: Implement proper route matching based on _serverConfig->getLocations()
    // For now, just check if the method is allowed in the first location
    if (!_serverConfig->getLocations().empty()) {
        const std::vector<std::string>& allowedMethods = _serverConfig->getLocations().front()->getAllowedMethods();
        for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it) {
            if (*it == methodStr) {
                methodAllowed = true;
                break;
            }
        }
    }
    
    if (!methodAllowed) {
        _handleError(HTTP_STATUS_METHOD_NOT_ALLOWED);
        return;
    }
    
    // Handle the request based on method
    if (method == Request::GET) {
        _handleStaticFile();
    } else if (method == Request::POST) {
        // TODO: Implement POST handling
        _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
    } else if (method == Request::DELETE) {
        // TODO: Implement DELETE handling
        _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
    } else {
        _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
    }
}
```

- **Purpose**: Processes the HTTP request and generates an appropriate response
- **Key aspects**:
  - Validates the HTTP method
  - Checks if the method is allowed for the requested location
  - Routes the request to the appropriate handler based on the method
  - Generates error responses for unsupported or disallowed methods

### Error Handling Methods

```cpp
void Connection::_handleError(int statusCode)
{
    // Get error page content
    std::string errorContent = _getErrorPage(statusCode);
    
    // Set response with error status and content
    _response.setStatusCode(statusCode);
    _response.setBody(errorContent, "text/html");
}

std::string Connection::_getErrorPage(int statusCode)
{
    // Check if there is a custom error page configured
    std::map<int, std::string> errorPages = _serverConfig->getErrorPages();
    std::map<int, std::string>::const_iterator it = errorPages.find(statusCode);
    
    if (it != errorPages.end()) {
        // Custom error page configured, try to read it
        std::string path = it->second;
        
        // TODO: Implement proper file reading based on server root
        // For now, just try to read directly
        std::ifstream file(path.c_str());
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }
    }
    
    // Default error page
    std::stringstream errorContent;
    errorContent << "<html>\r\n"
                 << "<head><title>Error " << statusCode << "</title></head>\r\n"
                 << "<body>\r\n"
                 << "  <h1>Error " << statusCode << "</h1>\r\n"
                 << "  <p>" << getReasonPhrase(statusCode) << "</p>\r\n"
                 << "  <hr>\r\n"
                 << "  <p>WebServer</p>\r\n"
                 << "</body>\r\n"
                 << "</html>\r\n";
    
    return errorContent.str();
}
```

- **_handleError**:
  - Sets the response status code and body for an error condition
  - Calls _getErrorPage to get the error page content
  
- **_getErrorPage**:
  - Tries to load a custom error page from the server configuration
  - Falls back to a default error page if no custom page is available
  - Returns HTML content for the error page

## 7. Connection Management Methods

These methods remain largely the same as in the original implementation:

- **_updateLastActivity()**: Updates the timestamp of the last activity
- **isTimeout()**: Checks if the connection has been idle for too long
- **close()**: Closes the client socket and marks the connection as closed
- **shouldRead() / shouldWrite()**: Determine if the connection should be monitored for read/write events

## 8. New Methods for HTTP Access

```cpp
const Request& Connection::getRequest() const
{
    return _request;
}

const Response& Connection::getResponse() const
{
    return _response;
}
```

- **Purpose**: Provide access to the HTTP request and response objects
- **Usage**: Allow external components to inspect the HTTP state

## 9. Key Design Changes from Previous Version

### 1. Integration with HTTP Classes

The main change is the integration with dedicated HTTP classes:

- **Request Class**: Now handles all HTTP request parsing
- **Response Class**: Manages HTTP response generation
- **Headers Class**: Provides HTTP header management

This improves separation of concerns:
- Connection: Handles socket I/O and connection lifecycle
- Request/Response: Handle HTTP protocol semantics
- Server: Manages all connections and routing

### 2. Improved HTTP Protocol Support

The updated implementation provides much better HTTP protocol support:

- **Method Validation**: Proper validation of HTTP methods
- **Header Handling**: Full HTTP header support through Headers class
- **Keep-Alive**: Proper handling of persistent connections
- **Error Pages**: Custom error pages based on server configuration
- **Status Codes**: Comprehensive HTTP status code support

### 3. Enhanced Error Handling

Error handling is more robust:

- **Exception Catching**: Process method catches exceptions from request handlers
- **Error Response Generation**: Proper error responses with custom error pages
- **Method Validation**: Validates allowed methods for requested paths
- **Protocol Compliance**: Better adherence to HTTP protocol standards

## 10. Why These Changes Matter for Our Web Server

- **Modularity**: Cleaner separation of concerns between network, HTTP, and application logic
- **Protocol Compliance**: Better adherence to HTTP standards
- **Maintainability**: More organized code with dedicated classes for each responsibility
- **Extensibility**: Easier to add new HTTP features or methods
- **Resource Management**: Better handling of keep-alive connections and timeouts

These changes transform our networking server into a proper HTTP web server that can handle requests according to the HTTP protocol standards. The implementation now provides a solid foundation for adding the request handlers needed in Phase 4 of the project.