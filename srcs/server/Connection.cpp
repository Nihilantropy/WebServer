#include "Connection.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <string.h>

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

void Connection::_updateLastActivity()
{
    _lastActivity = time(NULL);
}

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

bool Connection::_parseHttpBody()
{
    // TODO: Implement body parsing based on Content-Length or chunked encoding
    // This will be implemented in Phase 3 (HTTP Protocol Implementation)
    
    // For now, assume we have the full body
    std::cout << "Received HTTP body from " << _clientIp << std::endl;
    return true;
}

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

bool Connection::isTimeout() const
{
    // Check if the connection has been idle for too long
    time_t now = time(NULL);
    return (now - _lastActivity) > CONNECTION_TIMEOUT;
}

void Connection::close()
{
    if (_clientFd >= 0) {
        ::close(_clientFd);
        _clientFd = -1;
    }
    _state = CLOSED;
}

int Connection::getFd() const
{
    return _clientFd;
}

const std::string& Connection::getClientIp() const
{
    return _clientIp;
}

Connection::ConnectionState Connection::getState() const
{
    return _state;
}

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