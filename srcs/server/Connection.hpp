#pragma once

#include <string>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../config/parser/ServerConfig.hpp"

/**
 * @brief Class to manage an individual client connection
 * 
 * This class handles all aspects of a client connection:
 * - Reading HTTP requests
 * - Processing requests
 * - Writing HTTP responses
 * - Managing connection state and timeouts
 */
class Connection {
public:
    /**
     * @brief Connection state machine states
     */
    enum ConnectionState {
        READING_HEADERS,  // Reading HTTP headers
        READING_BODY,     // Reading HTTP body
        PROCESSING,       // Processing the request
        SENDING_RESPONSE, // Sending response
        CLOSED            // Connection closed
    };
    
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
    
public:
    /**
     * @brief Construct a new Connection object
     * 
     * @param clientFd Client socket file descriptor
     * @param clientAddr Client address information
     * @param config Server configuration to use
     */
    Connection(int clientFd, struct sockaddr_in clientAddr, ServerConfig* config);
    
    /**
     * @brief Destroy the Connection object, close connection if open
     */
    ~Connection();
    
    /**
     * @brief Read available data from the socket
     * 
     * @return true if connection is still valid, false if closed
     */
    bool readData();
    
    /**
     * @brief Write pending data to the socket
     * 
     * @return true if connection is still valid, false if closed
     */
    bool writeData();
    
    /**
     * @brief Process the received request
     */
    void process();
    
    /**
     * @brief Check if the connection has timed out
     * 
     * @return true if connection has timed out, false otherwise
     */
    bool isTimeout() const;
    
    /**
     * @brief Close the connection
     */
    void close();
    
    /**
     * @brief Get the client socket file descriptor
     * 
     * @return int Client socket fd
     */
    int getFd() const;
    
    /**
     * @brief Get the client IP address
     * 
     * @return const std::string& Client IP
     */
    const std::string& getClientIp() const;
    
    /**
     * @brief Get the current connection state
     * 
     * @return ConnectionState Current state
     */
    ConnectionState getState() const;
    
    /**
     * @brief Check if connection should be monitored for read events
     * 
     * @return true if should monitor for reads, false otherwise
     */
    bool shouldRead() const;
    
    /**
     * @brief Check if connection should be monitored for write events
     * 
     * @return true if should monitor for writes, false otherwise
     */
    bool shouldWrite() const;
    
private:
    // Prevent copying
    Connection(const Connection& other);
    Connection& operator=(const Connection& other);
    
    // Helper methods
    void _updateLastActivity();
    bool _parseHttpHeaders();
    bool _parseHttpBody();
    void _processRequest();
    void _generateResponse();
};