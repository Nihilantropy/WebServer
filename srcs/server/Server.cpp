#include "Server.hpp"
#include <algorithm>
#include <sstream>

// Initialize static members
bool Server::_signalReceived = false;

/**
 * Constructor: Initialize server with configuration
 */
Server::Server(const std::vector<ServerConfig*>& configs)
    : _listenSockets(), _serverConfigs(configs), _defaultServers(), 
      _multiplexer(), _connections(), _running(false)
{
    if (_serverConfigs.empty()) {
        throw std::runtime_error("No server configurations provided");
    }
}

/**
 * Destructor: Clean up resources
 */
Server::~Server()
{
    shutdown();
}

/**
 * Initialize server by setting up listening sockets
 */
void Server::initialize()
{
    _setupListenSockets();
    _setupDefaultServers();
    
    // Set up signal handlers for clean shutdown
    setupSignalHandlers();
    
    std::cout << "Server initialized successfully." << std::endl;
}

/**
 * Set up listening sockets for each unique host:port combination
 */
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

/**
 * Set up default servers for each host:port combination
 * The first server defined for a host:port is the default
 */
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

/**
 * Get the server configuration that should handle this request
 * Implements virtual host routing based on the Host header
 */
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

/**
 * Run the main server loop
 */
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

/**
 * Check if the file descriptor belongs to a listening socket
 */
bool Server::_isListenSocket(int fd)
{
    for (std::vector<Socket*>::const_iterator it = _listenSockets.begin(); it != _listenSockets.end(); ++it) {
        if ((*it)->getSocketFd() == fd) {
            return true;
        }
    }
    return false;
}

/**
 * Get the Socket object for a given file descriptor
 */
Socket* Server::_getSocketByFd(int fd)
{
    for (std::vector<Socket*>::const_iterator it = _listenSockets.begin(); it != _listenSockets.end(); ++it) {
        if ((*it)->getSocketFd() == fd) {
            return *it;
        }
    }
    return NULL;
}

/**
 * Accept a new connection on a listening socket
 */
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

/**
 * Handle activity on a client connection
 */
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

/**
 * Check for connection timeouts
 */
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

/**
 * Gracefully shut down the server
 */
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

/**
 * Set up signal handlers
 */
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

/**
 * Signal handler function
 */
void Server::_signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nReceived signal " << signal << ". Shutting down..." << std::endl;
        _signalReceived = true;
    }
}