#include "Connection.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include "../http/StatusCodes.hpp"

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

void Connection::_handleStaticFile()
{
    // TODO: Implement proper file serving based on location configuration
    // For now, just return a simple response
    _handleDefault();
}

void Connection::_handleDefault()
{
    // Simple default response for initial implementation
    std::string responseBody = "<html>\r\n"
                               "<head><title>WebServer</title></head>\r\n"
                               "<body>\r\n"
                               "  <h1>Welcome to WebServer!</h1>\r\n"
                               "  <p>Your request has been processed successfully.</p>\r\n"
                               "  <hr>\r\n"
                               "  <p>Request details:</p>\r\n"
                               "  <ul>\r\n"
                               "    <li>Method: " + _request.getMethodStr() + "</li>\r\n"
                               "    <li>Path: " + _request.getPath() + "</li>\r\n"
                               "    <li>Client IP: " + _clientIp + "</li>\r\n"
                               "  </ul>\r\n"
                               "</body>\r\n"
                               "</html>\r\n";
    
    _response.setStatusCode(HTTP_STATUS_OK);
    _response.setBody(responseBody, "text/html");
}

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

const Request& Connection::getRequest() const
{
    return _request;
}

const Response& Connection::getResponse() const
{
    return _response;
}