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
        _handlePostRequest();
    } else if (method == Request::DELETE) {
        _handleDeleteRequest();
    } else {
        _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
    }
}

void Connection::_handleStaticFile()
{
    // Get the request path
    std::string requestPath = _request.getPath();
    
    // Find the appropriate location for this path
    LocationConfig* location = _findLocation(requestPath);
    if (!location) {
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Check if this is a redirection
    if (!location->getRedirection().empty()) {
        _handleRedirection(*location);
        return;
    }
    
    // Resolve the path to a file system path
    std::string fsPath = FileUtils::resolvePath(requestPath, *location);
    
    // Check if the path exists
    if (!FileUtils::fileExists(fsPath)) {
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Handle directory
    if (FileUtils::isDirectory(fsPath)) {
        _handleDirectory(fsPath, requestPath, *location);
        return;
    }
    
    // Handle regular file
    _serveFile(fsPath);
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

LocationConfig* Connection::_findLocation(const std::string& requestPath)
{
    const std::vector<LocationConfig*>& locations = _serverConfig->getLocations();
    
    // First, try to find an exact match
    for (std::vector<LocationConfig*>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
        if ((*it)->getPath() == requestPath) {
            return *it;
        }
    }
    
    // Next, find the longest matching prefix
    LocationConfig* bestMatch = NULL;
    size_t bestMatchLength = 0;
    
    for (std::vector<LocationConfig*>::const_iterator it = locations.begin(); it != locations.end(); ++it) {
        const std::string& locationPath = (*it)->getPath();
        
        // Check if location path is a prefix of the request path
        if (requestPath.find(locationPath) == 0) {
            // Check if this location is a better match than the current best
            if (locationPath.length() > bestMatchLength) {
                bestMatch = *it;
                bestMatchLength = locationPath.length();
            }
        }
    }
    
    return bestMatch;
}

void Connection::_handleRedirection(const LocationConfig& location)
{
    // Parse the redirection directive
    std::string redirection = location.getRedirection();
    std::istringstream iss(redirection);
    
    // Extract status code and URL
    std::string returnKeyword;
    int statusCode;
    std::string redirectUrl;
    
    iss >> returnKeyword >> statusCode >> redirectUrl;
    
    // Set up the redirect response
    _response.redirect(redirectUrl, statusCode);
}

void Connection::_handleDirectory(const std::string& fsPath, const std::string& requestPath, const LocationConfig& location)
{
    // Check if the request path ends with a slash
    bool endsWithSlash = requestPath[requestPath.length() - 1] == '/';
    
    // If the path doesn't end with a slash, redirect to add the slash
    if (!endsWithSlash) {
        std::string redirectUrl = requestPath + "/";
        _response.redirect(redirectUrl, HTTP_STATUS_MOVED_PERMANENTLY);
        return;
    }
    
    // Look for an index file
    std::string indexFile = location.getIndex();
    if (!indexFile.empty()) {
        std::string indexPath = fsPath + "/" + indexFile;
        if (FileUtils::fileExists(indexPath)) {
            _serveFile(indexPath);
            return;
        }
    }
    
    // If no index file or autoindex is off, return 403 Forbidden
    if (!location.getAutoIndex()) {
        _handleError(HTTP_STATUS_FORBIDDEN);
        return;
    }
    
    // Generate directory listing
    std::string listing = FileUtils::generateDirectoryListing(fsPath, requestPath);
    if (listing.empty()) {
        _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return;
    }
    
    // Send directory listing
    _response.setStatusCode(HTTP_STATUS_OK);
    _response.setBody(listing, "text/html");
}

void Connection::_serveFile(const std::string& fsPath)
{
    // Get the file contents
    std::string contents = FileUtils::getFileContents(fsPath);
    if (contents.empty()) {
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Get the file extension and MIME type
    std::string extension = FileUtils::getFileExtension(fsPath);
    std::string mimeType = FileUtils::getMimeType(extension);
    
    // Check if this is a CGI file
    LocationConfig* location = _findLocation(_request.getPath());
    if (location) {
        const std::vector<std::string>& cgiExtensions = location->getCgiExtentions();
        for (std::vector<std::string>::const_iterator it = cgiExtensions.begin(); it != cgiExtensions.end(); ++it) {
            // Check if extension matches (with or without leading dot)
            std::string cgiExt = *it;
            if (cgiExt[0] == '.') {
                cgiExt = cgiExt.substr(1);
            }
            
            if (extension == cgiExt) {
                _handleCgi(fsPath, *location);
                return;
            }
        }
    }
    
    // Set the response
    _response.setStatusCode(HTTP_STATUS_OK);
    _response.setBody(contents, mimeType);
}

void Connection::_handleCgi(const std::string& fsPath, const LocationConfig& location)
{
    // TODO: Implement CGI handling
    // For now, just return a placeholder response
    std::string responseBody = "<html>\r\n"
                               "<head><title>CGI Not Implemented</title></head>\r\n"
                               "<body>\r\n"
                               "  <h1>CGI Support Coming Soon</h1>\r\n"
                               "  <p>The requested file would be handled by CGI:</p>\r\n"
                               "  <ul>\r\n"
                               "    <li>File: " + fsPath + "</li>\r\n"
                               "    <li>CGI Path: " + location.getCgiPath() + "</li>\r\n"
                               "  </ul>\r\n"
                               "</body>\r\n"
                               "</html>\r\n";
    
    _response.setStatusCode(HTTP_STATUS_OK);
    _response.setBody(responseBody, "text/html");
}

void Connection::_handlePostRequest()
{
    // Get the request path
    std::string requestPath = _request.getPath();
    
    // Find the appropriate location for this path
    LocationConfig* location = _findLocation(requestPath);
    if (!location) {
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Check if this location has an upload directory
    std::string uploadDir = location->getUploadDir();
    if (!uploadDir.empty()) {
        _handleFileUpload(*location);
        return;
    }
    
    // Check if this is a CGI request
    std::string extension = FileUtils::getFileExtension(requestPath);
    const std::vector<std::string>& cgiExtensions = location->getCgiExtentions();
    
    for (std::vector<std::string>::const_iterator it = cgiExtensions.begin(); it != cgiExtensions.end(); ++it) {
        std::string cgiExt = *it;
        if (cgiExt[0] == '.') {
            cgiExt = cgiExt.substr(1);
        }
        
        if (extension == cgiExt) {
            std::string fsPath = FileUtils::resolvePath(requestPath, *location);
            _handleCgi(fsPath, *location);
            return;
        }
    }
    
    // If we get here, we don't know how to handle this POST request
    _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
}

void Connection::_handleFileUpload(const LocationConfig& location)
{
    // Get content type to determine how to handle the upload
    std::string contentType = _request.getHeaders().getContentType();
    
    // Check for a proper content type for file uploads
    if (contentType.find("multipart/form-data") == std::string::npos) {
        // This is not a proper file upload
        _handleError(HTTP_STATUS_BAD_REQUEST);
        return;
    }
    
    // Parse the multipart form data
    MultipartParser parser(contentType, _request.getBody());
    if (!parser.parse()) {
        _handleError(HTTP_STATUS_BAD_REQUEST);
        return;
    }
    
    // Get the uploaded files
    const std::vector<UploadedFile>& files = parser.getFiles();
    if (files.empty()) {
        // No files were uploaded
        std::string responseBody = "<html>\r\n"
                                   "<head><title>No Files Uploaded</title></head>\r\n"
                                   "<body>\r\n"
                                   "  <h1>No Files Uploaded</h1>\r\n"
                                   "  <p>No files were found in the upload.</p>\r\n"
                                   "</body>\r\n"
                                   "</html>\r\n";
        
        _response.setStatusCode(HTTP_STATUS_BAD_REQUEST);
        _response.setBody(responseBody, "text/html");
        return;
    }
    
    // TODO: Implement actual file saving
    // For now, just generate a response showing the files
    
    std::stringstream responseBody;
    responseBody << "<html>\r\n"
                 << "<head><title>Upload Successful</title></head>\r\n"
                 << "<body>\r\n"
                 << "  <h1>Upload Successful</h1>\r\n"
                 << "  <p>Received " << files.size() << " file(s).</p>\r\n"
                 << "  <ul>\r\n";
    
    for (size_t i = U0; i < files.size(); ++i) {
        responseBody << "    <li>\r\n"
                     << "      <strong>Field Name:</strong> " << files[i].name << "<br>\r\n"
                     << "      <strong>Filename:</strong> " << files[i].filename << "<br>\r\n"
                     << "      <strong>Content Type:</strong> " << files[i].contentType << "<br>\r\n"
                     << "      <strong>Size:</strong> " << files[i].content.size() << " bytes<br>\r\n"
                     << "    </li>\r\n";
    }
    
    responseBody << "  </ul>\r\n"
                 << "  <p>The files would be saved to: " << location.getUploadDir() << "</p>\r\n"
                 << "  <p>Form Fields:</p>\r\n"
                 << "  <ul>\r\n";
    
    const std::map<std::string, std::string>& fields = parser.getFields();
    for (std::map<std::string, std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it) {
        responseBody << "    <li><strong>" << it->first << ":</strong> " << it->second << "</li>\r\n";
    }
    
    responseBody << "  </ul>\r\n"
                 << "</body>\r\n"
                 << "</html>\r\n";
    
    _response.setStatusCode(HTTP_STATUS_OK);
    _response.setBody(responseBody.str(), "text/html");
}

void Connection::_handleDeleteRequest()
{
    // Get the request path
    std::string requestPath = _request.getPath();
    
    // Find the appropriate location for this path
    LocationConfig* location = _findLocation(requestPath);
    if (!location) {
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Resolve the path to a file system path
    std::string fsPath = FileUtils::resolvePath(requestPath, *location);
    
    // Check if the file exists
    if (!FileUtils::fileExists(fsPath)) {
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Don't allow deleting directories
    if (FileUtils::isDirectory(fsPath)) {
        _handleError(HTTP_STATUS_FORBIDDEN);
        return;
    }
    
    // TODO: Implement actual file deletion
    // For now, just return a successful response
    
    std::string responseBody = "<html>\r\n"
                               "<head><title>Delete Successful</title></head>\r\n"
                               "<body>\r\n"
                               "  <h1>Delete Successful</h1>\r\n"
                               "  <p>File deletion support is coming soon.</p>\r\n"
                               "  <p>File: " + fsPath + "</p>\r\n"
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