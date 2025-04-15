#pragma once

#include <vector>
#include <map>
#include <string>
#include <signal.h>
#include <iostream>
#include "Socket.hpp"
#include "IOMultiplexer.hpp"
#include "Connection.hpp"
#include "../config/parser/ServerConfig.hpp"
#include "../exceptions/exceptions.hpp"

/**
 * @brief Main server class that manages sockets and connections
 * 
 * This class is responsible for:
 * - Creating and managing server sockets based on configuration
 * - Implementing the main event loop using IOMultiplexer
 * - Accepting new connections and handling data transfer
 * - Managing signal handling for clean shutdown
 */
class Server {
private:
    std::vector<Socket*>                      _listenSockets;    // Sockets for each host:port
    std::vector<ServerConfig*>                _serverConfigs;    // Server configurations
    std::map<std::string, ServerConfig*>      _defaultServers;   // Default server for each host:port

    IOMultiplexer                             _multiplexer;      // I/O multiplexer
    std::map<int, Connection*>                _connections;      // Active connections
    
    bool                                      _running;          // Server running state
    
    // Helper methods
    void _setupListenSockets();
    void _setupDefaultServers();
    ServerConfig* _getServerConfig(const std::string& host, int port, const std::string& serverName);
    void _acceptNewConnection(Socket* socket);
    void _handleConnection(Connection* connection);
    bool _isListenSocket(int fd);
    Socket* _getSocketByFd(int fd);
    void _checkTimeouts();
    
    // Signal handling
    static bool _signalReceived;
    static void _signalHandler(int signal);
    
public:
    /**
     * @brief Construct a new Server object
     * 
     * @param configs Vector of server configurations
     */
    Server(const std::vector<ServerConfig*>& configs);
    
    /**
     * @brief Destroy the Server object, clean up resources
     */
    ~Server();
    
    /**
     * @brief Initialize the server by setting up sockets
     */
    void initialize();
    
    /**
     * @brief Run the main server loop
     */
    void run();
    
    /**
     * @brief Shutdown the server gracefully
     */
    void shutdown();
    
    /**
     * @brief Set up signal handlers for clean shutdown
     */
    static void setupSignalHandlers();
    
private:
    // Prevent copying
    Server(const Server& other);
    Server& operator=(const Server& other);
};