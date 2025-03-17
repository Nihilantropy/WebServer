# WebServer Project Roadmap

## Current Progress Summary

- ✅ Configuration file parsing system
- ✅ Server and Location configuration classes
- ✅ Basic exception handling
- ✅ Utility functions for string manipulation
- ✅ Well-structured directory and class organization
- ✅ Makefile and build system

## Development Pathway

### Phase 1: Configuration Completion (1-2 days)

- [ ] **Complete Configuration Validation**
  - Implement `ServerConfigValidator::_validate()` fully
  - Validate port numbers
  - Set default error pages when not provided
  - Validate client body size limits
  - Ensure default index files are set for directories
  - Check for valid CGI configurations

- [ ] **Configuration Error Reporting**
  - Improve error messages for configuration issues
  - Add validation for route conflicts

### Phase 2: HTTP Server Core (3-5 days)

- [ ] **Socket Management**
  - Create Socket class for socket operations
  - Implement non-blocking socket configuration
  - Add support for binding to multiple ports

- [ ] **Server Core**
  - Implement main Server class
  - Create initialization and shutdown procedures
  - Set up signal handling

- [ ] **I/O Multiplexing**
  - Create wrapper for poll()/select()/kqueue()/epoll()
  - Implement non-blocking I/O operations
  - Set up the event loop

- [ ] **Connection Management**
  - Implement Connection class for client connections
  - Handle connection acceptance
  - Manage connection timeouts
  - Implement proper connection cleanup

### Phase 3: HTTP Protocol Implementation (3-4 days)

- [ ] **HTTP Request Parsing**
  - Create HTTP Request class
  - Parse HTTP headers, methods, URIs, and versions
  - Handle chunked transfer encoding
  - Process query parameters

- [ ] **HTTP Response Generation**
  - Implement HTTP Response class
  - Create standard headers
  - Set up status codes and reason phrases
  - Implement response body handling

- [ ] **HTTP Status Codes**
  - Define all required HTTP status codes
  - Implement proper error responses
  - Create default error pages

### Phase 4: Request Handlers (3-4 days)

- [ ] **Handler Architecture**
  - Create base RequestHandler interface
  - Implement handler resolution based on request path
  - Set up handler chaining/middleware

- [ ] **GET Method**
  - Implement static file serving
  - Add support for directory indexes
  - Handle conditional requests (If-Modified-Since, etc.)

- [ ] **POST Method**
  - Implement form data parsing
  - Set up basic request body handling
  - Create extension points for upload handling

- [ ] **DELETE Method**
  - Implement file deletion
  - Add proper permission checking
  - Create success/failure responses

- [ ] **Directory Listing**
  - Create directory listing generator
  - Implement on/off toggle based on config
  - Style the directory listing output

### Phase 5: Advanced Features (4-6 days)

- [ ] **CGI Implementation**
  - Create CGI execution environment
  - Implement environment variable setup
  - Set up process execution and pipe handling
  - Handle CGI output parsing

- [ ] **File Upload Handling**
  - Implement multipart/form-data parsing
  - Create file storage mechanism
  - Add upload size limiting
  - Implement upload directory configuration

- [ ] **HTTP Redirects**
  - Implement redirect handling based on config
  - Support various redirect types (301, 302, 307, etc.)
  - Handle relative and absolute redirects

### Phase 6: Testing and Refinement (3-5 days)

- [ ] **Unit Testing**
  - Create tests for configuration parsing
  - Test HTTP request/response handling
  - Validate CGI functionality

- [ ] **Integration Testing**
  - Test complete request flows
  - Verify configuration scenarios
  - Test with different browsers

- [ ] **Stress Testing**
  - Implement load testing
  - Verify server stability under load
  - Test memory usage and leaks

- [ ] **Browser Compatibility**
  - Test with multiple browsers
  - Ensure consistent behavior
  - Fix any browser-specific issues

### Phase 7: Documentation and Finalization (2-3 days)

- [ ] **Code Documentation**
  - Complete class and method documentation
  - Add usage examples
  - Document configuration options

- [ ] **User Guide**
  - Create configuration guide
  - Document default behaviors
  - Add troubleshooting section

- [ ] **Performance Optimization**
  - Profile and optimize critical paths
  - Reduce memory usage
  - Improve connection handling

## Directory Structure

```
webserv/
├── include/                        # Header files
├── srcs/
│   ├── config/                     # Configuration handling
│   │   ├── parser/
│   │   └── validate/
│   ├── exceptions/                 # Exception classes
│   ├── http/                       # HTTP protocol implementation
│   │   ├── Request.hpp
│   │   ├── Request.cpp
│   │   ├── Response.hpp
│   │   ├── Response.cpp
│   │   ├── Headers.hpp
│   │   └── Headers.cpp
│   ├── server/                     # Server core
│   │   ├── Server.hpp
│   │   ├── Server.cpp
│   │   ├── Socket.hpp
│   │   ├── Socket.cpp
│   │   ├── Connection.hpp
│   │   ├── Connection.cpp
│   │   ├── IOMultiplexer.hpp
│   │   └── IOMultiplexer.cpp
│   ├── handlers/                   # Request handlers
│   │   ├── RequestHandler.hpp
│   │   ├── GetHandler.hpp
│   │   ├── PostHandler.hpp
│   │   ├── DeleteHandler.hpp
│   │   ├── FileHandler.hpp
│   │   └── DirectoryHandler.hpp
│   ├── cgi/                        # CGI handling
│   │   ├── CGIHandler.hpp
│   │   ├── CGIHandler.cpp
│   │   ├── CGIEnvironment.hpp
│   │   └── CGIEnvironment.cpp
│   ├── upload/                     # File upload handling
│   │   ├── UploadHandler.hpp
│   │   └── UploadHandler.cpp
│   ├── utils/                      # Utility functions
│   └── main.cpp                    # Entry point
├── config/                         # Configuration files
│   └── webserv.conf
├── tests/                          # Test files
├── var/www/                        # Web content
├── errors/                         # Error pages
└── Makefile                        # Build system
```

## Key Implementation Notes

### Non-blocking I/O

All file descriptors must be set to non-blocking mode:

```cpp
fcntl(socket_fd, F_SETFL, O_NONBLOCK);
```

Never perform I/O operations without going through poll() (or equivalent).

### Poll/Select Implementation

The main loop should check for both read and write readiness:

```cpp
// Example with poll()
struct pollfd fds[MAX_CONNECTIONS];
// Set up fds array
int ready = poll(fds, nfds, timeout);
if (ready > 0) {
    // Check which descriptors are ready
    for (int i = 0; i < nfds; i++) {
        if (fds[i].revents & POLLIN) {
            // Ready for reading
        }
        if (fds[i].revents & POLLOUT) {
            // Ready for writing
        }
    }
}
```

### HTTP Request Parsing

Break this into stages:
1. Read the request line
2. Parse headers
3. Handle request body (if present)

### CGI Execution

Remember that for CGI:
- The server must set up environment variables
- Use pipes for communication
- The CGI process should get the requested path as the first argument
- Must handle chunked requests by unchunking before passing to CGI

### Error Handling

Never let the server crash:
- Catch all exceptions in the main loop
- Log errors but keep the server running
- Return appropriate HTTP error responses

## RFC References

- HTTP/1.1: [RFC 2616](https://datatracker.ietf.org/doc/html/rfc2616)
- CGI: [RFC 3875](https://datatracker.ietf.org/doc/html/rfc3875)