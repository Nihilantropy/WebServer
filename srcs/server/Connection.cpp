#include "Connection.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string.h>
#include "../http/StatusCodes.hpp"
#include "../cgi/CGIHandler.hpp"
#include "../utils/StringUtils.hpp"

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
        DebugLogger::log("Connection is closed, not reading");
        return false;
    }
    
    // Make sure we only read when we should
    if (_state != READING_HEADERS && _state != READING_BODY) {
        std::stringstream ss;
        ss << _state;
        DebugLogger::log("Connection state doesn't allow reading: " + ss.str());
        return true;
    }
    
    char buffer[4096];
    ssize_t bytesRead;
    
    // Read data from socket using non-blocking I/O
    bytesRead = recv(_clientFd, buffer, sizeof(buffer), 0);
    
    if (bytesRead > 0) {
        // Append read data to input buffer
        std::string newData(buffer, bytesRead);
        _inputBuffer.append(newData);
        _updateLastActivity();
        
        std::cout << "Read " << bytesRead << " bytes from " << _clientIp << std::endl;
        DebugLogger::hexDump("Raw request data", newData);
        
        // Process data based on current state
        if (_state == READING_HEADERS) {
            DebugLogger::log("Parsing headers...");
            if (_request.parseHeaders(_inputBuffer)) {
                DebugLogger::log("Headers parsed successfully");
                DebugLogger::logRequest(_clientIp, _request.getMethodStr(), 
                                     _request.getPath(), _request.getHeaders().toString());
                
                // Headers complete, check if we need to read the body
                if (_request.isComplete()) {
                    // No body expected, move to processing
                    DebugLogger::log("Request is complete (no body required), moving to PROCESSING state");
                    _state = PROCESSING;
                    process();
                } else {
                    // Body expected, move to reading body
                    DebugLogger::log("Body expected, moving to READING_BODY state");
                    
                    std::stringstream ss;
                    ss << _request.getHeaders().getContentLength();
                    DebugLogger::log("Content-Length: " + ss.str());
                    
                    _state = READING_BODY;
                }
            } else {
                DebugLogger::log("Headers not complete yet, continuing to read");
            }
        } else if (_state == READING_BODY) {
            DebugLogger::log("Parsing body...");
            
            std::stringstream ss;
            ss << _inputBuffer.size();
            DebugLogger::log("Body buffer size: " + ss.str());
            
            if (_request.parseBody(_inputBuffer)) {
                // Body complete, move to processing
                DebugLogger::log("Body parsed successfully, moving to PROCESSING state");
                
                std::stringstream bodySizeStr;
                bodySizeStr << _request.getBody().size();
                DebugLogger::log("Body size: " + bodySizeStr.str());
                
                _state = PROCESSING;
                process();
            } else {
                DebugLogger::log("Body not complete yet, continuing to read");
                DebugLogger::log("Current body content: " + _request.getBody().substr(0, 100) + 
                             (_request.getBody().size() > 100 ? "..." : ""));
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
            DebugLogger::log("No data available (EAGAIN/EWOULDBLOCK)");
            return true;
        } else {
            // Error
            std::cerr << "Error reading from socket " << _clientFd << ": " << strerror(errno) << std::endl;
            DebugLogger::logError("Socket read error: " + std::string(strerror(errno)));
            _state = CLOSED;
            return false;
        }
    }
}

bool Connection::writeData()
{
    if (_state == CLOSED) {
        DebugLogger::log("Connection is closed, not writing");
        return false;
    }
    
    // Make sure we only write when we should
    if (_state != SENDING_RESPONSE || _outputBuffer.empty()) {
        std::stringstream ss;
        ss << _state;
        std::stringstream ss2;
        ss2 << _outputBuffer.size();
        DebugLogger::log("Not in writing state or buffer empty, state: " + ss.str() + 
                      ", buffer size: " + ss2.str());
        return true;
    }
    
    std::stringstream ss;
    ss << _outputBuffer.size();
    DebugLogger::log("Writing response data, buffer size: " + ss.str());
    
    ssize_t bytesWritten = send(_clientFd, _outputBuffer.c_str(), _outputBuffer.size(), 0);
    
    if (bytesWritten > 0) {
        _updateLastActivity();
        
        std::cout << "Wrote " << bytesWritten << " bytes to " << _clientIp << std::endl;
        
        std::stringstream bwStr;
        bwStr << bytesWritten;
        DebugLogger::log("Wrote " + bwStr.str() + " bytes to client");
        
        // Remove sent data from buffer
        _outputBuffer.erase(0, bytesWritten);
        
        std::stringstream remStr;
        remStr << _outputBuffer.size();
        DebugLogger::log("Remaining output buffer size: " + remStr.str());
        
        // If all data sent, move to next state
        if (_outputBuffer.empty()) {
            // Mark the response as sent
            _response.markAsSent();
            DebugLogger::log("Response fully sent");
            
            // Check connection header
            bool keepAlive = _request.getHeaders().keepAlive(true);
            DebugLogger::log("Keep-alive: " + std::string(keepAlive ? "yes" : "no"));
            
            if (keepAlive) {
                // Reset for the next request
                DebugLogger::log("Keeping connection alive, resetting for next request");
                _request.reset();
                _state = READING_HEADERS;
                _inputBuffer.clear();
            } else {
                // Close connection after response
                DebugLogger::log("Not keep-alive, closing connection");
                _state = CLOSED;
            }
        }
        
        return true;
    } else if (bytesWritten == 0) {
        // Socket closed
        std::cout << "Connection closed during write: " << _clientIp << std::endl;
        DebugLogger::logError("Connection closed by client during write");
        _state = CLOSED;
        return false;
    } else {
        // Error or would block
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Socket buffer full, try again later
            DebugLogger::log("Write would block (EAGAIN/EWOULDBLOCK), trying again later");
            return true;
        } else {
            // Error
            std::cerr << "Error writing to socket " << _clientFd << ": " << strerror(errno) << std::endl;
            DebugLogger::logError("Socket write error: " + std::string(strerror(errno)));
            _state = CLOSED;
            return false;
        }
    }
}

void Connection::process()
{
    if (_state != PROCESSING) {
        std::stringstream ss;
        ss << _state;
        DebugLogger::log("process() called but state is not PROCESSING, current state: " + ss.str());
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
        DebugLogger::logError("Exception during request processing: " + std::string(e.what()));
        _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
    
    // Build the response and store in output buffer
    _outputBuffer = _response.build();
    
    // Log the response
    DebugLogger::logResponse(_response.getStatusCode(), _response.getHeaders().toString());
    
    std::stringstream ss;
    ss << _response.getBody().size();
    DebugLogger::log("Response body length: " + ss.str());
    
    // Move to sending response state
    _state = SENDING_RESPONSE;
}

void Connection::_processRequest()
{
    DebugLogger::log("Processing request: " + _request.getMethodStr() + " " + _request.getPath());
    
    // Check if method is supported
    Request::Method method = _request.getMethod();
    if (method == Request::UNKNOWN) {
        DebugLogger::logError("Unknown HTTP method");
        _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
        return;
    }
    
    // Find the appropriate location for this path
    LocationConfig* location = _findLocation(_request.getPath());
    if (!location) {
        DebugLogger::logError("No location block found for path: " + _request.getPath());
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    DebugLogger::log("Found location block: " + location->getPath() + 
                  " with root: " + location->getRoot());
    
    // Check if the requested method is allowed for this location
    std::string methodStr = _request.getMethodStr();
    bool methodAllowed = false;
    
    const std::vector<std::string>& allowedMethods = location->getAllowedMethods();
    DebugLogger::log("Allowed methods for this location:");
    for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); 
         it != allowedMethods.end(); ++it) {
        DebugLogger::log(" - " + *it);
        if (*it == methodStr) {
            methodAllowed = true;
        }
    }
    
    if (!methodAllowed) {
        DebugLogger::logError("Method " + methodStr + " not allowed for this location");
        _handleError(HTTP_STATUS_METHOD_NOT_ALLOWED);
        return;
    }
    
    // Check if this is a redirection
    if (!location->getRedirection().empty()) {
        DebugLogger::log("This location has a redirection: " + location->getRedirection());
        _handleRedirection(*location);
        return;
    }
    
    // Handle the request based on method
    if (method == Request::GET) {
        DebugLogger::log("Handling GET request");
        _handleStaticFile();
    } else if (method == Request::POST) {
        DebugLogger::log("Handling POST request");
        _handlePostRequest();
    } else if (method == Request::DELETE) {
        DebugLogger::log("Handling DELETE request");
        _handleDeleteRequest();
    } else {
        DebugLogger::logError("Method implementation missing for " + methodStr);
        _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
    }
}

void Connection::_handleStaticFile()
{
    DebugLogger::log("Handling static file for path: " + _request.getPath());
    
    // Get the request path
    std::string requestPath = _request.getPath();
    
    // Find the appropriate location for this path
    LocationConfig* location = _findLocation(requestPath);
    if (!location) {
        DebugLogger::logError("No location block found for path: " + requestPath);
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Resolve the path to a file system path
    std::string fsPath = FileUtils::resolvePath(requestPath, *location);
    DebugLogger::log("Resolved filesystem path: " + fsPath);
    
    // Check if the path exists
    if (!FileUtils::fileExists(fsPath)) {
        DebugLogger::logError("File not found: " + fsPath);
        
        // Special handling for root path with index
        if (requestPath == "/" || requestPath == "") {
            std::string indexFile = location->getIndex();
            if (!indexFile.empty()) {
                std::string indexPath = fsPath;
                if (indexPath[indexPath.length() - 1] != '/') {
                    indexPath += '/';
                }
                indexPath += indexFile;
                
                DebugLogger::log("Trying index file: " + indexPath);
                
                if (FileUtils::fileExists(indexPath)) {
                    DebugLogger::log("Index file exists, serving: " + indexPath);
                    _serveFile(indexPath);
                    return;
                } else {
                    DebugLogger::logError("Index file not found: " + indexPath);
                }
            } else {
                DebugLogger::log("No index file configured for this location");
            }
        }
        
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Handle directory
    if (FileUtils::isDirectory(fsPath)) {
        DebugLogger::log("Path is a directory: " + fsPath);
        _handleDirectory(fsPath, requestPath, *location);
        return;
    }
    
    // Handle regular file
    DebugLogger::log("Serving regular file: " + fsPath);
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
    DebugLogger::log("Finding location for path: " + requestPath);
    
    const std::vector<LocationConfig*>& locations = _serverConfig->getLocations();
    
    // First, try to find an exact match
    for (std::vector<LocationConfig*>::const_iterator it = locations.begin(); 
         it != locations.end(); ++it) {
        if ((*it)->getPath() == requestPath) {
            DebugLogger::log("Found exact match location: " + (*it)->getPath());
            return *it;
        }
    }
    
    // Next, find the longest matching prefix
    LocationConfig* bestMatch = NULL;
    size_t bestMatchLength = 0;
    
    for (std::vector<LocationConfig*>::const_iterator it = locations.begin(); 
         it != locations.end(); ++it) {
        const std::string& locationPath = (*it)->getPath();
        
        DebugLogger::log("Checking if location '" + locationPath + "' matches request '" + requestPath + "'");
        
        // Special case for root location - lowest priority
        if (locationPath == "/") {
            if (bestMatch == NULL) { // Only use root if we have no other match
                bestMatch = *it;
                bestMatchLength = 1;
                DebugLogger::log("Root location '/' is fallback match");
            }
            continue; // Skip the rest of the loop for root location
        }
        
        // Check if location path is a prefix of the request path
        if (requestPath.find(locationPath) == 0) {
            // For non-root locations, verify it's a proper segment match
            size_t locPathLen = locationPath.length();
            
            // If request path matches exactly OR
            // If location ends with '/' (directory) OR 
            // If next character in request path is '/' (path boundary)
            if (requestPath.length() == locPathLen || 
                locationPath[locPathLen-1] == '/' || 
                (requestPath.length() > locPathLen && requestPath[locPathLen] == '/')) {
                
                // Check if this location is a better match than the current best
                if (locPathLen > bestMatchLength) {
                    bestMatch = *it;
                    bestMatchLength = locPathLen;
                    
                    std::stringstream ss;
                    ss << locPathLen;
                    DebugLogger::log("New best match: '" + locationPath + 
                                  "' (length: " + ss.str() + ")");
                }
            }
        }
    }
    
    if (bestMatch) {
        DebugLogger::log("Final best match location: " + bestMatch->getPath());
    } else {
        DebugLogger::logError("No matching location found for " + requestPath);
    }
    
    return bestMatch;
}

void Connection::_handleRedirection(const LocationConfig& location)
{
    // Parse the redirection directive
    std::string redirection = location.getRedirection();
    std::istringstream iss(redirection);
    
    // Extract status code and URL
    int statusCode;
    std::string redirectUrl;
    
    // Parse the redirection string
    if (!(iss >> statusCode >> redirectUrl)) {
        // Handle parsing failure
        _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return;
    }
    
    // Ensure URL is an absolute path (starts with /)
    if (redirectUrl.length() > 0 && redirectUrl[0] == '.') {
        // Convert relative path starting with ./ to absolute
        if (redirectUrl.length() > 1 && redirectUrl[1] == '/') {
            redirectUrl = redirectUrl.substr(1); // Remove the dot
        } else {
            // Add leading slash
            redirectUrl = "/" + redirectUrl;
        }
    } else if (redirectUrl.length() > 0 && redirectUrl[0] != '/') {
        // Add leading slash to make it an absolute path
        redirectUrl = "/" + redirectUrl;
    }
    
    // Set up the redirect response
    _response.redirect(redirectUrl, statusCode);
}

void Connection::_handleDirectory(const std::string& fsPath, const std::string& requestPath, const LocationConfig& location)
{
    DebugLogger::log("Handling directory: " + fsPath + " for request path: " + requestPath);
    
    // Check if the request path ends with a slash
    bool endsWithSlash = requestPath[requestPath.length() - 1] == '/';
    DebugLogger::log("Path ends with slash: " + std::string(endsWithSlash ? "yes" : "no"));
    
    // If the path doesn't end with a slash, redirect to add the slash
    if (!endsWithSlash) {
        std::string redirectUrl = requestPath + "/";
        DebugLogger::log("Redirecting to add trailing slash: " + redirectUrl);
        _response.redirect(redirectUrl, HTTP_STATUS_MOVED_PERMANENTLY);
        return;
    }
    
    // Look for an index file
    std::string indexFile = location.getIndex();
    if (!indexFile.empty()) {
        std::string indexPath = fsPath + "/" + indexFile;
        DebugLogger::log("Checking for index file: " + indexPath);
        
        if (FileUtils::fileExists(indexPath)) {
            DebugLogger::log("Index file exists, serving: " + indexPath);
            _serveFile(indexPath);
            return;
        } else {
            DebugLogger::logError("Index file not found: " + indexPath);
        }
    }
    
    // If no index file or autoindex is off, return 403 Forbidden
    if (!location.getAutoIndex()) {
        DebugLogger::logError("No index file and autoindex is off, returning 403 Forbidden");
        _handleError(HTTP_STATUS_FORBIDDEN);
        return;
    }
    
    // Generate directory listing
    DebugLogger::log("Generating directory listing for: " + fsPath);
    std::string listing = FileUtils::generateDirectoryListing(fsPath, requestPath);
    if (listing.empty()) {
        DebugLogger::logError("Failed to generate directory listing");
        _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return;
    }
    
    // Send directory listing
    DebugLogger::log("Serving directory listing");
    _response.setStatusCode(HTTP_STATUS_OK);
    _response.setBody(listing, "text/html");
}

void Connection::_serveFile(const std::string& fsPath)
{
    DebugLogger::log("Serving file: " + fsPath);
    
    // Get the file contents
    std::string contents = FileUtils::getFileContents(fsPath);
    if (contents.empty()) {
        DebugLogger::logError("Failed to read file contents: " + fsPath);
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    std::stringstream ss;
    ss << contents.size();
    DebugLogger::log("Read file contents, size: " + ss.str());
    
    // Get the file extension and MIME type
    std::string extension = FileUtils::getFileExtension(fsPath);
    std::string mimeType = FileUtils::getMimeType(extension);
    
    DebugLogger::log("File extension: " + extension + ", MIME type: " + mimeType);
    
    // Check if this is a CGI file
    LocationConfig* location = _findLocation(_request.getPath());
    if (location) {
        const std::vector<std::string>& cgiExtensions = location->getCgiExtentions();
        for (std::vector<std::string>::const_iterator it = cgiExtensions.begin(); 
             it != cgiExtensions.end(); ++it) {
            // Check if extension matches (with or without leading dot)
            std::string cgiExt = *it;
            if (cgiExt[0] == '.') {
                cgiExt = cgiExt.substr(1);
            }
            
            if (extension == cgiExt) {
                DebugLogger::log("File is a CGI script, extension: " + extension);
                _handleCgi(fsPath, *location);
                return;
            }
        }
    }
    
    // Set the response
    DebugLogger::log("Setting response with file contents");
    _response.setStatusCode(HTTP_STATUS_OK);
    _response.setBody(contents, mimeType);
}

void Connection::_handleCgi(const std::string& fsPath, const LocationConfig& location)
{
    // Create CGI handler
    CGIHandler cgiHandler;
    
    // Execute CGI script
    if (cgiHandler.executeCGI(_request, fsPath, location, _response)) {
        // CGI execution successful, response has been set by CGIHandler
        std::cout << "CGI execution successful for: " << fsPath << std::endl;
    } else {
        // CGI execution failed
        std::cerr << "CGI execution failed for: " << fsPath << std::endl;
        _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
    }
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

/**
 * @brief Prepare the upload directory for file storage
 * 
 * @param uploadDir Path to the upload directory
 * @return true if directory exists and is writable, false otherwise
 */
bool Connection::_prepareUploadDirectory(const std::string& uploadDir)
{
    // Check if directory exists
    if (FileUtils::isDirectory(uploadDir)) {
        // Check if directory is writable
        if (!FileUtils::isWritable(uploadDir)) {
            std::cerr << "Upload directory is not writable: " << uploadDir << std::endl;
            return false;
        }
        return true;
    }
    
    // Try to create the directory
    if (!FileUtils::createDirectory(uploadDir)) {
        std::cerr << "Failed to create upload directory: " << uploadDir << " - " << strerror(errno) << std::endl;
        return false;
    }
    
    return true;
}

/**
 * @brief Handle file upload from POST request
 * 
 * @param location The location configuration for the request
 * @return true if upload was successful, false otherwise
 */
bool Connection::_handleFileUpload(const LocationConfig& location)
{
    // Get content type to determine how to handle the upload
    std::string contentType = _request.getHeaders().getContentType();
    
    // Check for a proper content type for file uploads
    if (contentType.find("multipart/form-data") == std::string::npos) {
        // This is not a proper file upload
        _handleError(HTTP_STATUS_BAD_REQUEST);
        return false;
    }
    
    // Check if client body size exceeds server limit
    if (_request.getBody().size() > _serverConfig->getClientMaxBodySize()) {
        _handleError(HTTP_STATUS_PAYLOAD_TOO_LARGE);
        return false;
    }
    
    // Parse the multipart form data
    MultipartParser parser(contentType, _request.getBody());
    if (!parser.parse()) {
        _handleError(HTTP_STATUS_BAD_REQUEST);
        return false;
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
        return false;
    }
    
    // Get the upload directory from the location configuration
    std::string uploadDir = location.getUploadDir();
    
    // Ensure upload directory exists and is writable
    if (!_prepareUploadDirectory(uploadDir)) {
        _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
        return false;
    }
    
    // Ensure upload directory ends with a slash
    if (uploadDir[uploadDir.length() - 1] != '/') {
        uploadDir += '/';
    }
    
    // Save each file to the upload directory
    std::stringstream responseBody;
    responseBody << "<html>\r\n"
                << "<head><title>Upload Result</title></head>\r\n"
                << "<body>\r\n"
                << "  <h1>Upload Result</h1>\r\n"
                << "  <p>Received " << files.size() << " file(s).</p>\r\n"
                << "  <ul>\r\n";
    
    size_t successfulUploads = 0;
    std::vector<std::string> savedPaths;
    
    for (size_t i = 0; i < files.size(); ++i) {
        // ===== FILE VALIDATION =====
        
        // Sanitize filename - replace any problematic characters
        std::string originalFilename = files[i].filename;
        std::string safeFilename = _sanitizeFilename(originalFilename);
        
        // Check if filename is valid after sanitization
        if (safeFilename.empty()) {
            responseBody << "    <li>\r\n"
                        << "      <strong>Error:</strong> Invalid filename: " << originalFilename << "<br>\r\n"
                        << "    </li>\r\n";
            continue;
        }
        
        // Validate file type (if needed)
        if (!_isAllowedFileType(safeFilename)) {
            responseBody << "    <li>\r\n"
                        << "      <strong>Error:</strong> File type not allowed: " << originalFilename << "<br>\r\n"
                        << "    </li>\r\n";
            continue;
        }
        
        // Validate file size (individual file check)
        if (files[i].content.size() > _serverConfig->getClientMaxBodySize()) {
            responseBody << "    <li>\r\n"
                        << "      <strong>Error:</strong> File too large: " << originalFilename << "<br>\r\n"
                        << "    </li>\r\n";
            continue;
        }
        
        // Create a unique filename if a file with this name already exists
        std::string finalFilename = _getUniqueFilename(uploadDir, safeFilename);
        
        // Full path to save the file
        std::string savePath = uploadDir + finalFilename;
        
        // Verify that the final path is still within the upload directory (security check)
        if (!FileUtils::isPathWithinDirectory(savePath, uploadDir)) {
            responseBody << "    <li>\r\n"
                        << "      <strong>Error:</strong> Security violation for file: " << originalFilename << "<br>\r\n"
                        << "    </li>\r\n";
            continue;
        }
        
        // ===== ACTUAL FILE STORAGE =====
        
        // Save the file
        bool saveSuccess = false;
        try {
            std::ofstream fileStream(savePath.c_str(), std::ios::binary);
            if (fileStream.is_open()) {
                fileStream.write(files[i].content.c_str(), files[i].content.size());
                saveSuccess = !fileStream.bad();
                fileStream.close();
                
                // Set appropriate permissions (e.g., 0644)
                chmod(savePath.c_str(), S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            }
        } catch (const std::exception& e) {
            saveSuccess = false;
            std::cerr << "Exception while saving file " << finalFilename << ": " << e.what() << std::endl;
        }
        
        if (saveSuccess) {
            successfulUploads++;
            savedPaths.push_back(savePath);
            
            responseBody << "    <li>\r\n"
                        << "      <strong>Field Name:</strong> " << files[i].name << "<br>\r\n"
                        << "      <strong>Original Filename:</strong> " << originalFilename << "<br>\r\n"
                        << "      <strong>Saved As:</strong> " << finalFilename << "<br>\r\n"
                        << "      <strong>Content Type:</strong> " << files[i].contentType << "<br>\r\n"
                        << "      <strong>Size:</strong> " << FileUtils::formatFileSize(files[i].content.size()) << "<br>\r\n"
                        << "    </li>\r\n";
        } else {
            // Failed to write file
            responseBody << "    <li>\r\n"
                        << "      <strong>Error:</strong> Failed to save file " << originalFilename << "<br>\r\n"
                        << "    </li>\r\n";
            
            // Clean up any partial file that might have been created
            if (FileUtils::fileExists(savePath)) {
                unlink(savePath.c_str());
            }
        }
    }
    
    responseBody << "  </ul>\r\n";
    
    // Add form fields to response
    const std::map<std::string, std::string>& fields = parser.getFields();
    if (!fields.empty()) {
        responseBody << "  <h2>Form Fields:</h2>\r\n"
                    << "  <ul>\r\n";
        
        for (std::map<std::string, std::string>::const_iterator it = fields.begin(); it != fields.end(); ++it) {
            responseBody << "    <li><strong>" << it->first << ":</strong> " << it->second << "</li>\r\n";
        }
        
        responseBody << "  </ul>\r\n";
    }
    
    // Report summary
    responseBody << "  <p><strong>Upload Summary:</strong> " 
                << successfulUploads << " of " << files.size() 
                << " files were uploaded successfully to " << uploadDir << "</p>\r\n";
    
    responseBody << "</body>\r\n"
                << "</html>\r\n";
    
    // Set response status based on success
    if (successfulUploads > 0) {
        _response.setStatusCode(HTTP_STATUS_OK);
    } else if (files.size() > 0) {
        // Files were provided but none could be saved
        _response.setStatusCode(HTTP_STATUS_INTERNAL_SERVER_ERROR);
    } else {
        // No files were provided (should not happen due to earlier check)
        _response.setStatusCode(HTTP_STATUS_BAD_REQUEST);
    }
    
    _response.setBody(responseBody.str(), "text/html");
    return (successfulUploads > 0);
}

// Helper method to sanitize filenames
std::string Connection::_sanitizeFilename(const std::string& filename)
{
    // Remove any path components (directories)
    std::string baseName = filename;
    size_t lastSlash = baseName.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        baseName = baseName.substr(lastSlash + 1);
    }
    
    // Replace potentially dangerous characters
    std::string safeFilename;
    for (size_t i = 0; i < baseName.length(); ++i) {
        char c = baseName[i];
        // Allow alphanumeric characters, dash, underscore, dot
        if (isalnum(c) || c == '-' || c == '_' || c == '.') {
            safeFilename += c;
        } else {
            // Replace other characters with underscore
            safeFilename += '_';
        }
    }
    
    // Don't allow filenames starting with a dot (hidden files)
    if (!safeFilename.empty() && safeFilename[0] == '.') {
        safeFilename = "dot_" + safeFilename.substr(1);
    }
    
    return safeFilename;
}

// Generate a unique filename to avoid overwriting existing files
std::string Connection::_getUniqueFilename(const std::string& directory, const std::string& filename)
{
    // Check if file already exists
    if (!FileUtils::fileExists(directory + filename)) {
        return filename;
    }
    
    // Extract base name and extension
    std::string baseName = filename;
    std::string extension = "";
    
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        baseName = filename.substr(0, dotPos);
        extension = filename.substr(dotPos);
    }
    
    // Try adding numbers to the filename until we find an unused name
    int counter = 1;
    std::string uniqueName;
    
    do {
        std::stringstream ss;
        ss << baseName << "_" << counter << extension;
        uniqueName = ss.str();
        counter++;
    } while (FileUtils::fileExists(directory + uniqueName) && counter < 1000);
    
    // If we couldn't find a unique name after 1000 tries, use timestamp
    if (counter >= 1000) {
        std::stringstream ss;
        ss << baseName << "_" << time(NULL) << extension;
        uniqueName = ss.str();
    }
    
    return uniqueName;
}

// Check if a file type is allowed (based on extension)
bool Connection::_isAllowedFileType(const std::string& filename)
{
    // Get file extension
    std::string extension = "";
    size_t dotPos = filename.find_last_of('.');
    if (dotPos != std::string::npos) {
        extension = filename.substr(dotPos + 1);
        
        // Convert to lowercase for case-insensitive comparison
        for (size_t i = 0; i < extension.length(); ++i) {
            extension[i] = tolower(extension[i]);
        }
    }
    
    // Example: Disallow potentially dangerous file types
    // This is a basic example - you may want to customize this list
    static const char* disallowedExtensions[] = {
        "php", "cgi", "pl", "py", "sh", "exe", "bat", "cmd", "htaccess", NULL
    };
    
    for (int i = 0; disallowedExtensions[i] != NULL; ++i) {
        if (extension == disallowedExtensions[i]) {
            return false;
        }
    }
    
    return true;
}

void Connection::_handleDeleteRequest()
{
    // Get the request path
    std::string requestPath = _request.getPath();
    
    // Find the appropriate location for this path
    LocationConfig* location = _findLocation(requestPath);
    if (!location) {
        std::cout << "DELETE: Location not found for path: " << requestPath << std::endl;
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Check if DELETE method is allowed for this location
    const std::vector<std::string>& allowedMethods = location->getAllowedMethods();
    bool deleteAllowed = false;
    
    for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); it != allowedMethods.end(); ++it) {
        if (*it == "DELETE") {
            deleteAllowed = true;
            break;
        }
    }
    
    if (!deleteAllowed) {
        std::cout << "DELETE: Method not allowed for path: " << requestPath << std::endl;
        _handleError(HTTP_STATUS_METHOD_NOT_ALLOWED);
        return;
    }
    
    // Resolve the path to a file system path
    std::string fsPath = FileUtils::resolvePath(requestPath, *location);
    
    // ===== SECURITY CHECKS =====
    
    // Check if the file exists
    if (!FileUtils::fileExists(fsPath)) {
        std::cout << "DELETE: File not found: " << fsPath << std::endl;
        _handleError(HTTP_STATUS_NOT_FOUND);
        return;
    }
    
    // Check if this is a directory
    if (FileUtils::isDirectory(fsPath)) {
        std::cout << "DELETE: Cannot delete directory: " << fsPath << std::endl;
        _handleError(HTTP_STATUS_FORBIDDEN);
        return;
    }
    
    // Check if the file is outside the location's root directory (security check)
    std::string rootDir = location->getRoot();
    if (!FileUtils::isPathWithinDirectory(fsPath, rootDir)) {
        std::cout << "DELETE: Path outside root directory: " << fsPath << std::endl;
        _handleError(HTTP_STATUS_FORBIDDEN);
        return;
    }
    
    // Check if we have permission to delete the file
    if (!FileUtils::isWritable(fsPath)) {
        std::cout << "DELETE: Permission denied: " << fsPath << std::endl;
        _handleError(HTTP_STATUS_FORBIDDEN);
        return;
    }
    
    // ===== DELETE OPERATION =====
    
    // Attempt to delete the file
    if (remove(fsPath.c_str()) != 0) {
        // Failed to delete the file
        std::cout << "DELETE: Failed to delete file: " << fsPath << " - " << strerror(errno) << std::endl;
        
        // Different error responses based on the reason for failure
        if (errno == EACCES || errno == EPERM) {
            // Permission denied
            _handleError(HTTP_STATUS_FORBIDDEN);
        } else {
            // Other errors
            _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
        }
        return;
    }
    
    // File deleted successfully
    std::cout << "DELETE: Successfully deleted file: " << fsPath << std::endl;
    
    // Create a simple success response
    std::string responseBody = "<html>\r\n"
                              "<head><title>Delete Successful</title></head>\r\n"
                              "<body>\r\n"
                              "  <h1>Delete Successful</h1>\r\n"
                              "  <p>The file has been successfully deleted.</p>\r\n"
                              "  <p><strong>Path:</strong> " + requestPath + "</p>\r\n"
                              "</body>\r\n"
                              "</html>\r\n";
    
    _response.setStatusCode(HTTP_STATUS_OK);
    _response.setBody(responseBody, "text/html");
}

void Connection::_handleError(int statusCode)
{
    std::stringstream ss;
    ss << statusCode;
    DebugLogger::log("Handling error, status code: " + ss.str());
    
    // Get error page content
    std::string errorContent = _getErrorPage(statusCode);
    
    // Set response with error status and content
    _response.setStatusCode(statusCode);
    _response.setBody(errorContent, "text/html");
    
    std::stringstream sizeStr;
    sizeStr << errorContent.size();
    DebugLogger::log("Error page set, content size: " + sizeStr.str());
}

std::string Connection::_getErrorPage(int statusCode)
{
    std::stringstream ss;
    ss << statusCode;
    DebugLogger::log("Getting error page for status code: " + ss.str());
    
    // Check if there is a custom error page configured
    std::map<int, std::string> errorPages = _serverConfig->getErrorPages();
    std::map<int, std::string>::const_iterator it = errorPages.find(statusCode);
    
    if (it != errorPages.end()) {
        // Custom error page configured, try to read it
        std::string path = it->second;
        DebugLogger::log("Custom error page found: " + path);
        
        // Check if the path starts with a slash, if so, it's relative to server root
        if (!path.empty() && path[0] == '/') {
            // Find a suitable location to resolve the path
            LocationConfig* location = _findLocation("/");
            if (location) {
                // Remove leading slash
                std::string relativePath = path.substr(1);
                std::string root = location->getRoot();
                if (root[root.length() - 1] != '/') {
                    root += '/';
                }
                
                // Construct full path
                path = root + relativePath;
                DebugLogger::log("Resolved error page path: " + path);
            }
        }
        
        // Try to read the file
        if (FileUtils::fileExists(path)) {
            DebugLogger::log("Reading custom error page: " + path);
            std::string content = FileUtils::getFileContents(path);
            if (!content.empty()) {
                return content;
            }
            DebugLogger::logError("Failed to read custom error page: " + path);
        } else {
            DebugLogger::logError("Custom error page not found: " + path);
        }
    }
    
    // Default error page
    DebugLogger::log("Using default error page");
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