# Detailed Explanation of the Socket Class

## 1. Socket Class Purpose and Overview

The Socket class is a C++ wrapper around the POSIX socket API that encapsulates the functionality needed for network communication using TCP/IP. In our web server, this class handles the low-level details of creating server sockets that:

- Listen for incoming connections
- Accept those connections
- Manage socket properties like non-blocking mode

## 2. Class Attributes Explained

```cpp
private:
    int             _socketFd;         // Socket file descriptor
    std::string     _host;             // IP address to bind to
    int             _port;             // Port to bind to
    struct sockaddr_in _address;       // Socket address structure
    bool            _isNonBlocking;    // Flag for non-blocking mode
```

- **_socketFd**: A file descriptor (integer) that represents the socket. In UNIX/Linux, sockets are treated like files and are identified by these descriptors.
- **_host**: The IP address the socket will bind to (e.g., "127.0.0.1" for localhost or "0.0.0.0" for all interfaces).
- **_port**: The port number the socket will listen on (e.g., 8080).
- **_address**: A structure that combines IP address and port information for binding.
- **_isNonBlocking**: A flag indicating if the socket operates in non-blocking mode.

## 3. Constructor and Destructor

```cpp
Socket::Socket(const std::string& host, int port)
    : _socketFd(-1), _host(host), _port(port), _isNonBlocking(false)
{
    std::memset(&_address, 0, sizeof(_address));
}

Socket::~Socket()
{
    close();
}
```

- **Constructor**: 
  - Initializes basic properties (_host, _port)
  - Sets _socketFd to -1 (invalid) to indicate the socket hasn't been created yet
  - Zeroes the address structure with memset to ensure it's properly initialized
  
- **Destructor**:
  - Calls close() to ensure the socket is properly closed when the object is destroyed
  - This prevents resource leaks by always cleaning up open file descriptors

## 4. Core Socket Functions

### create()

```cpp
void Socket::create()
{
    // Create a TCP socket
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFd < 0)
        throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
    
    // Allow reuse of local addresses
    int opt = 1;
    if (setsockopt(_socketFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        close();
        throw std::runtime_error("Failed to set socket options: " + std::string(strerror(errno)));
    }
}
```

- **socket(AF_INET, SOCK_STREAM, 0)**:
  - **AF_INET**: Specifies the IPv4 address family
  - **SOCK_STREAM**: Creates a TCP socket (reliable, connection-oriented)
  - **0**: Uses the default protocol for SOCK_STREAM (TCP)
  
- **setsockopt with SO_REUSEADDR**:
  - Allows the socket to bind to an address that was recently used
  - Prevents "Address already in use" errors when restarting the server
  - Critical for development when you need to restart the server frequently

### setNonBlocking()

```cpp
void Socket::setNonBlocking()
{
    // Set socket to non-blocking mode using fcntl
    int flags = fcntl(_socketFd, F_GETFL, 0);
    if (flags < 0)
    {
        close();
        throw std::runtime_error("Failed to get socket flags: " + std::string(strerror(errno)));
    }
    
    if (fcntl(_socketFd, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        close();
        throw std::runtime_error("Failed to set socket to non-blocking mode: " + std::string(strerror(errno)));
    }
    
    _isNonBlocking = true;
}
```

- **fcntl(F_GETFL)**: Gets the current socket flags
- **fcntl(F_SETFL)**: Sets new flags for the socket
- **O_NONBLOCK**: Flag that puts socket operations in non-blocking mode

**Why Non-Blocking Mode is Critical:**
- In blocking mode, operations like accept(), read(), and write() can block indefinitely
- Non-blocking mode makes these operations return immediately, even if they can't complete
- Operations return -1 with errno set to EAGAIN/EWOULDBLOCK when they would block
- This allows us to use poll() to efficiently monitor multiple sockets

### bind()

```cpp
void Socket::bind()
{
    // Set up the address structure
    _address.sin_family = AF_INET;
    _address.sin_port = htons(_port);
    
    // Convert host string to network address
    if (_host == "0.0.0.0" || _host == "localhost")
        _address.sin_addr.s_addr = INADDR_ANY;
    else if (inet_pton(AF_INET, _host.c_str(), &_address.sin_addr) <= 0)
    {
        close();
        throw std::runtime_error("Invalid address: " + _host);
    }
    
    // Bind the socket to the address
    if (::bind(_socketFd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
    {
        close();
        throw std::runtime_error("Failed to bind socket: " + std::string(strerror(errno)));
    }
}
```

- **sockaddr_in structure**:
  - **sin_family = AF_INET**: Sets IPv4 address family
  - **sin_port = htons(_port)**: Converts port to network byte order
  - **sin_addr**: Holds the IP address to bind to

- **INADDR_ANY**: Special value meaning "bind to all available interfaces"
  - Used when _host is "0.0.0.0" or "localhost"
  
- **inet_pton**: Converts an IP address string to binary form
  - "pton" stands for "presentation to network"
  
- **bind**: Associates the socket with a specific address and port
  - The :: prefix is used to call the global bind() function, not a method

### listen()

```cpp
void Socket::listen(int backlog)
{
    // Start listening for connections
    if (::listen(_socketFd, backlog) < 0)
    {
        close();
        throw std::runtime_error("Failed to listen on socket: " + std::string(strerror(errno)));
    }
}
```

- **listen**: Marks the socket as passive (ready to accept connections)
- **backlog**: Maximum length of the queue of pending connections
  - If more clients try to connect when the queue is full, they'll be refused
  - Default value of 10 is reasonable for many scenarios

### accept()

```cpp
int Socket::accept()
{
    // Accept a new connection
    struct sockaddr_in clientAddr;
    socklen_t clientAddrLen = sizeof(clientAddr);
    
    int clientFd = ::accept(_socketFd, (struct sockaddr*)&clientAddr, &clientAddrLen);
    
    // We don't throw on EAGAIN/EWOULDBLOCK because these aren't errors
    // in non-blocking mode, they just mean "no connection available right now"
    return clientFd;
}
```

- **accept**: Creates a new socket for a connection from the queue
  - Returns a new file descriptor for the client connection
  - The original socket continues listening for more connections
  
- **clientAddr**: Will be filled with the client's address information
  - Could be used to get the client's IP address and port
  
- **Non-blocking behavior**:
  - May return -1 with EAGAIN/EWOULDBLOCK if no connections are waiting
  - We don't throw an exception in this case, as it's expected in non-blocking mode

### close()

```cpp
void Socket::close()
{
    if (_socketFd >= 0)
    {
        ::close(_socketFd);
        _socketFd = -1;
    }
}
```

- **close**: Releases the socket file descriptor and resources
- Checks if _socketFd is valid before attempting to close
- Sets _socketFd to -1 after closing to prevent double-closing

## 5. Network Byte Order and Addressing

- **htons()** (Host TO Network Short): Converts port number to network byte order (big-endian)
- **Network byte order**: Standardized byte ordering used in network protocols
  - Different CPUs have different byte ordering (endianness)
  - Network protocols use big-endian ordering
  - Converting ensures consistency across different systems

- **Special IP addresses**:
  - **0.0.0.0**: Bind to all network interfaces on the machine
  - **127.0.0.1**: Loopback interface (localhost), only accessible locally
  - **Specific IP**: Bind to a specific network interface

## 6. Error Handling

- **Consistent approach**: Check return values and throw exceptions with descriptive messages
- **Resource cleanup**: Always close the socket before throwing exceptions to prevent leaks
- **Error details**: Include the specific error message using strerror(errno)

## 7. Why This Design Matters for Our Web Server

- **Encapsulation**: Hides the complexity of socket programming
- **Resource management**: Ensures proper socket cleanup through RAII principles
- **Error handling**: Provides clear error messages for debugging
- **Non-blocking operation**: Essential for meeting the WebServer project requirements
- **Reusability**: Well-designed class can be used for all servers in our configuration
