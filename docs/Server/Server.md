# Detailed Explanation of the Server Class

## 1. Server Class Purpose and Overview

The Server class is the central orchestrator of the entire web server application. It acts as the glue that ties together all the other components: Socket, IOMultiplexer, Connection, and the configuration system. The Server class is responsible for:

- Initializing server sockets based on configuration
- Implementing the main event loop using IOMultiplexer
- Accepting new client connections
- Managing the lifecycle of connections
- Handling timeouts and cleanup
- Providing graceful shutdown capabilities
- Routing requests to the appropriate virtual hosts

This class implements the core server logic required by the WebServer project, ensuring all operations are performed in a non-blocking manner through a single poll() call, as specified in the project requirements.

## 2. Class Attributes Explained

```cpp
private:
    std::vector<Socket*>                      _listenSockets;    // Sockets for each host:port
    std::vector<ServerConfig*>                _serverConfigs;    // Server configurations
    std::map<std::string, ServerConfig*>      _defaultServers;   // Default server for each host:port
    
    IOMultiplexer                             _multiplexer;      // I/O multiplexer
    std::map<int, Connection*>                _connections;      // Active connections
    
    bool                                      _running;          // Server running state
    
    static bool _signalReceived;                                // Signal flag for clean shutdown
```

- **_listenSockets**: Vector of Socket objects for each unique host:port combination.
- **_serverConfigs**: Vector of server configurations from the parsed config file.
- **_defaultServers**: Map of host:port strings to their default server configurations (for routing).
- **_multiplexer**: IOMultiplexer instance that handles all I/O multiplexing operations.
- **_connections**: Map of file descriptors to Connection objects for active client connections.
- **_running**: Flag indicating if the server is currently running.
- **_signalReceived**: Static flag set by the signal handler when a shutdown signal is received.

## 3. Constructor and Destructor

```cpp
Server::Server(const std::vector<ServerConfig*>& configs)
    : _listenSockets(), _serverConfigs(configs), _defaultServers(), 
      _multiplexer(), _connections(), _running(false)
{
    if (_serverConfigs.empty()) {
        throw std::runtime_error("No server configurations provided");
    }
}

Server::~Server()
{
    shutdown();
}
```

- **Constructor**:
  - Initializes member variables
  - Validates that at least one server configuration exists
  - Throws an exception if no configurations were provided
  
- **Destructor**:
  - Calls shutdown() to ensure all resources are properly released
  - Prevents resource leaks by cleaning up sockets and connections

## 4. Initialization Methods

### initialize()

```cpp
void Server::initialize()
{
    _setupListenSockets();
    _setupDefaultServers();
    
    // Set up signal handlers for clean shutdown
    setupSignalHandlers();
    
    std::cout << "Server initialized successfully." << std::endl;
}
```

- **Purpose**: Main initialization method for the server
- **Behavior**:
  - Sets up listening sockets based on configuration
  - Establishes default servers for each host:port
  - Sets up signal handlers for clean shutdown
- **Usage**: Called after server construction and before starting the event loop

### _setupListenSockets()

```cpp
void Server::_setupListenSockets()
{
    // Track which host:port combinations we've already set up
    std::map<std::string, bool> createdSockets;
    
    for (std::vector<ServerConfig*>::const_iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        std::stringstream ss;
        ss << (*it)->getHost() << ":" << (*it)->getPort();
        std::string hostPort = ss.str();
        
        // Skip if we already created a socket for this host:port
        if (createdSockets.find(hostPort) != createdSockets.end()) {
            continue;
        }
        
        try {
            // Create and initialize a socket for this host:port
            Socket* socket = new Socket((*it)->getHost(), (*it)->getPort());
            socket->create();
            socket->setNonBlocking();  // Required by subject
            socket->bind();
            socket->listen();
            
            _listenSockets.push_back(socket);
            createdSockets[hostPort] = true;
            
            // Add to multiplexer
            _multiplexer.addFd(socket->getSocketFd(), IOMultiplexer::EVENT_READ, socket);
            
            std::cout << "Listening on " << (*it)->getHost() << ":" << (*it)->getPort() << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Failed to set up socket for " << hostPort << ": " << e.what() << std::endl;
            // Continue with other sockets instead of failing completely
        }
    }
    
    if (_listenSockets.empty()) {
        throw std::runtime_error("Failed to set up any listening sockets");
    }
}
```

- **Purpose**: Sets up listening sockets for each unique host:port combination in the configuration
- **Key aspects**:
  - Creates only one socket per unique host:port, even if multiple server blocks use the same host:port
  - Sets all sockets to non-blocking mode as required by the project
  - Adds each socket to the IOMultiplexer for event monitoring
  - Logs information about each listening socket
  - Continues even if some sockets fail to set up
  - Throws an exception only if all sockets fail

### _setupDefaultServers()

```cpp
void Server::_setupDefaultServers()
{
    for (std::vector<ServerConfig*>::const_iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        std::stringstream ss;
        ss << (*it)->getHost() << ":" << (*it)->getPort();
        std::string hostPort = ss.str();
        
        // Set as default server if not already set
        if (_defaultServers.find(hostPort) == _defaultServers.end()) {
            _defaultServers[hostPort] = *it;
        }
    }
}
```

- **Purpose**: Establishes the default server configuration for each host:port combination
- **Behavior**:
  - For each host:port, the first server configuration in the file becomes the default
  - This follows NGINX's behavior where the first server block is the default
- **Importance**: The default server handles requests when no server_name matches

## 5. Main Event Loop

### run()

```cpp
void Server::run()
{
    // Prevent running if not initialized
    if (_listenSockets.empty()) {
        throw std::runtime_error("Server not initialized");
    }
    
    _running = true;
    std::cout << "Server started. Press Ctrl+C to stop." << std::endl;
    
    // Main event loop
    while (_running && !_signalReceived) {
        // Wait for activity with a timeout of 1 second
        // This allows us to periodically check timeouts and _signalReceived
        int activity = _multiplexer.wait(1000);
        
        if (activity < 0) {
            // Poll error (except when interrupted by a signal)
            if (errno != EINTR) {
                std::cerr << "poll() failed: " << strerror(errno) << std::endl;
                break;
            }
            continue;
        }
        
        if (activity == 0) {
            // Timeout - check for connection timeouts
            _checkTimeouts();
            continue;
        }
        
        // Get all file descriptors with activity
        std::vector<int> activeFds = _multiplexer.getActiveFds();
        
        for (std::vector<int>::const_iterator it = activeFds.begin(); it != activeFds.end(); ++it) {
            int fd = *it;
            
            // Check for errors
            if (_multiplexer.hasError(fd)) {
                if (_isListenSocket(fd)) {
                    std::cerr << "Error on listening socket: " << fd << std::endl;
                    // This is serious - we might want to remove this socket
                } else {
                    // Client connection error
                    Connection* connection = static_cast<Connection*>(_multiplexer.getData(fd));
                    if (connection) {
                        connection->close();
                        _multiplexer.removeFd(fd);
                        delete connection;
                        _connections.erase(fd);
                    }
                }
                continue;
            }
            
            // Check for read readiness
            if (_multiplexer.isReadReady(fd)) {
                if (_isListenSocket(fd)) {
                    // Accept new connection on listening socket
                    Socket* socket = static_cast<Socket*>(_multiplexer.getData(fd));
                    if (socket) {
                        _acceptNewConnection(socket);
                    }
                } else {
                    // Handle data from existing connection
                    Connection* connection = static_cast<Connection*>(_multiplexer.getData(fd));
                    if (connection) {
                        _handleConnection(connection);
                    }
                }
            }
            
            // Check for write readiness
            if (_multiplexer.isWriteReady(fd)) {
                // Only for client connections, not listening sockets
                if (!_isListenSocket(fd)) {
                    Connection* connection = static_cast<Connection*>(_multiplexer.getData(fd));
                    if (connection) {
                        _handleConnection(connection);
                    }
                }
            }
        }
        
        // Check for connection timeouts
        _checkTimeouts();
    }
    
    std::cout << "Server event loop terminated." << std::endl;
}
```

- **Purpose**: Implements the main server event loop
- **Key aspects**:
  - Uses IOMultiplexer to wait for events with a 1-second timeout
  - Handles poll errors and interrupts gracefully
  - Processes all active file descriptors returned by poll()
  - Checks for errors on each file descriptor
  - Processes read events (accepting new connections or reading from existing ones)
  - Processes write events (writing to connections)
  - Periodically checks for connection timeouts
  - Continues until _running is false or a signal is received
- **Implementation details**:
  - The 1-second timeout allows for periodic tasks without blocking indefinitely
  - Each active file descriptor is processed according to its type (socket or connection)
  - All I/O operations are performed through the non-blocking mechanisms
  - Single poll() call for all I/O operations as required by the project

## 6. Connection Handling Methods

### _acceptNewConnection()

```cpp
void Server::_acceptNewConnection(Socket* socket)
{
    int clientFd = socket->accept();
    
    if (clientFd < 0) {
        // EAGAIN/EWOULDBLOCK means no connections ready to be accepted
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return;
        }
        std::cerr << "Error accepting connection: " << strerror(errno) << std::endl;
        return;
    }
    
    // Set the new socket to non-blocking mode
    int flags = fcntl(clientFd, F_GETFL, 0);
    if (flags < 0 || fcntl(clientFd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "Failed to set client socket to non-blocking mode: " << strerror(errno) << std::endl;
        ::close(clientFd);
        return;
    }
    
    // Get client address information
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    getpeername(clientFd, (struct sockaddr*)&clientAddr, &addrLen);
    
    // TODO: Determine which server config to use based on Host header
    // For now, use the default server for this listening socket
    std::stringstream ss;
    ss << socket->getHost() << ":" << socket->getPort();
    std::string hostPort = ss.str();
    
    ServerConfig* config = _defaultServers[hostPort];
    
    // Create a new Connection object
    Connection* connection = new Connection(clientFd, clientAddr, config);
    _connections[clientFd] = connection;
    
    // Add to multiplexer - initially only interested in reading
    _multiplexer.addFd(clientFd, IOMultiplexer::EVENT_READ, connection);
}
```

- **Purpose**: Accepts a new client connection on a listening socket
- **Parameters**: Socket pointer from which to accept the connection
- **Behavior**:
  - Calls accept() on the socket to get a new client file descriptor
  - Handles non-blocking accept() cases (EAGAIN/EWOULDBLOCK)
  - Sets the new client socket to non-blocking mode
  - Gets client address information for logging and connection creation
  - Creates a new Connection object with the appropriate server configuration
  - Adds the connection to the connection map and the multiplexer
- **Key aspects**:
  - Proper error handling for all operations
  - All sockets are set to non-blocking mode
  - Client socket is associated with the default server for its host:port
  - Only monitors for read events initially (will add write events when needed)

### _handleConnection()

```cpp
void Server::_handleConnection(Connection* connection)
{
    int fd = connection->getFd();
    bool connectionValid = true;
    
    // Handle read events
    if (_multiplexer.isReadReady(fd) && connection->shouldRead()) {
        connectionValid = connection->readData();
    }
    
    // Handle write events
    if (connectionValid && _multiplexer.isWriteReady(fd) && connection->shouldWrite()) {
        connectionValid = connection->writeData();
    }
    
    // If connection is closed, clean up
    if (!connectionValid || connection->getState() == Connection::CLOSED) {
        std::cout << "Cleaning up connection " << fd << std::endl;
        _multiplexer.removeFd(fd);
        delete connection;
        _connections.erase(fd);
        return;
    }
    
    // Update the events we're interested in based on connection state
    short events = 0;
    if (connection->shouldRead()) {
        events |= IOMultiplexer::EVENT_READ;
    }
    if (connection->shouldWrite()) {
        events |= IOMultiplexer::EVENT_WRITE;
    }
    
    _multiplexer.modifyFd(fd, events);
}
```

- **Purpose**: Handles I/O events on an existing client connection
- **Parameters**: Connection pointer to handle
- **Behavior**:
  - Calls readData() if the connection is ready for reading and should read
  - Calls writeData() if the connection is ready for writing and should write
  - Cleans up the connection if it's closed or invalid
  - Updates the events to monitor based on the connection's current state
- **Key aspects**:
  - Only performs operations that are appropriate for the connection's state
  - Updates the IOMultiplexer with the events to monitor
  - Properly cleans up resources when a connection is closed
  - Follows the correct sequence of operations (read then write)

### _checkTimeouts()

```cpp
void Server::_checkTimeouts()
{
    std::vector<int> timeoutFds;
    
    // Find all connections that have timed out
    for (std::map<int, Connection*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        if (it->second->isTimeout()) {
            std::cout << "Connection timeout: " << it->second->getClientIp() << std::endl;
            timeoutFds.push_back(it->first);
        }
    }
    
    // Close and clean up timed out connections
    for (std::vector<int>::iterator it = timeoutFds.begin(); it != timeoutFds.end(); ++it) {
        int fd = *it;
        Connection* connection = _connections[fd];
        
        _multiplexer.removeFd(fd);
        connection->close();
        delete connection;
        _connections.erase(fd);
    }
}
```

- **Purpose**: Checks for and cleans up timed-out connections
- **Behavior**:
  - Identifies connections that have been idle for too long
  - Collects their file descriptors in a separate vector
  - Closes and cleans up each timed-out connection
- **Importance**:
  - Prevents resource exhaustion from abandoned connections
  - Ensures the server can handle a high number of clients
  - Periodically run from the main event loop

## 7. Utility Methods

### _isListenSocket() and _getSocketByFd()

```cpp
bool Server::_isListenSocket(int fd)
{
    for (std::vector<Socket*>::const_iterator it = _listenSockets.begin(); it != _listenSockets.end(); ++it) {
        if ((*it)->getSocketFd() == fd) {
            return true;
        }
    }
    return false;
}

Socket* Server::_getSocketByFd(int fd)
{
    for (std::vector<Socket*>::const_iterator it = _listenSockets.begin(); it != _listenSockets.end(); ++it) {
        if ((*it)->getSocketFd() == fd) {
            return *it;
        }
    }
    return NULL;
}
```

- **_isListenSocket**:
  - Checks if a file descriptor belongs to a listening socket
  - Used to determine how to handle events on a file descriptor
  
- **_getSocketByFd**:
  - Retrieves the Socket object associated with a file descriptor
  - Returns NULL if not found
  - Used when accepting new connections

### _getServerConfig()

```cpp
ServerConfig* Server::_getServerConfig(const std::string& host, int port, const std::string& serverName)
{
    // Combine host and port for lookup
    std::stringstream ss;
    ss << host << ":" << port;
    std::string hostPort = ss.str();
    
    // First, try to find a server config that matches serverName
    for (std::vector<ServerConfig*>::const_iterator it = _serverConfigs.begin(); it != _serverConfigs.end(); ++it) {
        if ((*it)->getHost() == host && (*it)->getPort() == port) {
            const std::vector<std::string>& serverNames = (*it)->getServerNames();
            if (std::find(serverNames.begin(), serverNames.end(), serverName) != serverNames.end()) {
                return *it;
            }
        }
    }
    
    // If no match, return the default server for this host:port
    if (_defaultServers.find(hostPort) != _defaultServers.end()) {
        return _defaultServers[hostPort];
    }
    
    // This should never happen if _setupDefaultServers is called correctly
    throw std::runtime_error("No server configuration found for " + hostPort);
}
```

- **Purpose**: Determines which server configuration should handle a request
- **Parameters**:
  - host: The listening socket's host
  - port: The listening socket's port
  - serverName: The server name from the HTTP Host header
- **Behavior**:
  - First tries to find a server config that matches the host:port and server name
  - If no match is found, returns the default server for the host:port
  - Throws an exception if no default server exists (should never happen)
- **Virtual hosting**:
  - This method implements "virtual hosting" based on the Host header
  - Allows multiple websites to be served from the same IP address and port
  - Follows NGINX's behavior for server name matching

## 8. Shutdown and Signal Handling

### shutdown()

```cpp
void Server::shutdown()
{
    _running = false;
    
    // Close all connections
    for (std::map<int, Connection*>::iterator it = _connections.begin(); it != _connections.end(); ++it) {
        it->second->close();
        delete it->second;
    }
    _connections.clear();
    
    // Close and delete all listen sockets
    for (std::vector<Socket*>::iterator it = _listenSockets.begin(); it != _listenSockets.end(); ++it) {
        (*it)->close();
        delete *it;
    }
    _listenSockets.clear();
    
    std::cout << "Server shut down." << std::endl;
}
```

- **Purpose**: Gracefully shuts down the server and cleans up all resources
- **Behavior**:
  - Sets the running flag to false to stop the event loop
  - Closes and deletes all active connections
  - Closes and deletes all listening sockets
  - Logs server shutdown
- **Usage**: Called from the destructor or when a shutdown signal is received

### setupSignalHandlers() and _signalHandler()

```cpp
void Server::setupSignalHandlers()
{
    struct sigaction sa;
    sa.sa_handler = &Server::_signalHandler;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    
    if (sigaction(SIGINT, &sa, NULL) == -1 || sigaction(SIGTERM, &sa, NULL) == -1) {
        std::cerr << "Failed to set up signal handlers: " << strerror(errno) << std::endl;
    }
}

void Server::_signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived signal " << signal << ". Shutting down..." << std::endl;
        _signalReceived = true;
    }
}
```

- **setupSignalHandlers**:
  - Sets up handlers for SIGINT (Ctrl+C) and SIGTERM signals
  - Uses sigaction for reliable signal handling
  - Logs an error if setup fails but continues execution
  
- **_signalHandler**:
  - Static method called when a signal is received
  - Sets the _signalReceived flag to true
  - Logs information about the received signal
  - Allows the server to shut down gracefully

## 9. Key Design Decisions

### 1. Single Poll() for All I/O

The Server class centralizes all I/O multiplexing through the IOMultiplexer class, ensuring that:
- Only one poll() call is used for all I/O operations
- Both read and write readiness are checked in the same poll() call
- No I/O operations are performed without going through poll()
- The server never blocks on I/O operations

### 2. Non-blocking Architecture

Every socket and I/O operation is non-blocking:
- All listening sockets are set to non-blocking mode
- All client connections are set to non-blocking mode
- All read and write operations handle EAGAIN/EWOULDBLOCK correctly
- The event loop uses a timeout to prevent indefinite blocking

### 3. Error Handling

The server implements robust error handling:
- Socket setup errors don't crash the server
- Errors on individual connections don't affect other connections
- Signal handling allows for graceful shutdown
- Resource cleanup ensures no leaks even in error conditions

### 4. Virtual Hosting

The server supports multiple virtual hosts:
- Multiple server blocks can share the same host:port
- Server selection is based on the Host header (like NGINX)
- Default servers handle requests when no server_name matches
- This allows serving multiple websites from the same IP address

### 5. Connection Management

Connection lifecycle is carefully managed:
- New connections are properly accepted and initialized
- Active connections are monitored for the appropriate events
- Idle connections are detected and closed after timeout
- All resources are properly cleaned up when connections close

### 6. Scalability

The design is built for scalability:
- Efficient event-driven architecture instead of thread-per-connection
- Single poll() call monitors all file descriptors
- Connection timeout prevents resource exhaustion
- Each connection only consumes resources when active

## 10. Why This Design Matters for Our Web Server

- **Compliance with Requirements**: Meets all the specific requirements of the WebServer project, including:
  - Non-blocking I/O
  - Single poll() for all I/O operations
  - Check for both read and write in the same poll() call
  - No I/O operations without going through poll()
  - Server never blocks

- **Scalability**: Can handle a large number of concurrent connections efficiently

- **Robustness**: Properly handles errors, signals, and resource cleanup

- **Extensibility**: Provides a clean architecture for implementing HTTP protocol features

- **Maintainability**: Well-organized code with clear separation of concerns

This design sets up a solid foundation for implementing the HTTP protocol in the next phase of the project, with all the core server infrastructure in place to handle client connections efficiently.