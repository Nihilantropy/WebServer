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

/*** READ & PROCESS REQUESTS DATA ***/
bool Connection::readData()
{
    if (!_isValidStateForReading())
        return _state != CLOSED;
        
    ssize_t bytesRead = _readFromSocket();
    
    if (bytesRead > 0) {
        _processReadData(bytesRead);
        return true;
    } else if (bytesRead == 0) {
        return _handleConnectionClosed();
    } else {
        return _handleSocketError();
    }
}

bool Connection::_isValidStateForReading() const
{
    if (_state == CLOSED) {
        DebugLogger::log("Connection is closed, not reading");
        return false;
    }
    
    // Only read in appropriate states
    if (_state != READING_HEADERS && _state != READING_BODY) {
        std::stringstream ss;
        ss << _state;
        DebugLogger::log("Connection state doesn't allow reading: " + ss.str());
        return false;
    }
    
    // Log pre-read state and buffer size
    std::stringstream preReadLog;
    preReadLog << "[DEBUG-UPLOAD] Pre-read state: " << _state 
              << ", Input buffer size: " << _inputBuffer.size();
    DebugLogger::log(preReadLog.str());
    
    return true;
}

ssize_t Connection::_readFromSocket()
{
    char buffer[4096];
    ssize_t bytesRead = recv(_clientFd, buffer, sizeof(buffer), 0);
    
    if (bytesRead > 0) {
        // Append read data to input buffer
        _inputBuffer.append(buffer, bytesRead);
        _updateLastActivity();
        
        // Log data received
        _logReadOperation(bytesRead, buffer);
    }
    
    return bytesRead;
}

void Connection::_logReadOperation(ssize_t bytesRead, const char* buffer)
{
    std::stringstream dataLog;
    dataLog << "Read " << bytesRead << " bytes, total buffer: " << _inputBuffer.size();
    DebugLogger::log(dataLog.str());
    
    std::cout << "Read " << bytesRead << " bytes from " << _clientIp << std::endl;
    DebugLogger::hexDump("Raw request data", std::string(buffer, bytesRead));
}

bool Connection::_handleConnectionClosed()
{
    std::cout << "Connection closed by client: " << _clientIp << std::endl;
    _state = CLOSED;
    return false;
}

bool Connection::_handleSocketError()
{
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // No data available, try again later
        std::stringstream eagainLog;
        eagainLog << "EAGAIN/EWOULDBLOCK - No data available, state: " << _state;
        DebugLogger::log(eagainLog.str());
        return true;
    } else {
        // Error
        std::cerr << "Error reading from socket " << _clientFd << ": " << strerror(errno) << std::endl;
        DebugLogger::logError("Socket read error: " + std::string(strerror(errno)));
        _state = CLOSED;
        return false;
    }
}

void Connection::_processReadData(ssize_t bytesRead)
{
    (void)bytesRead;

    if (_state == READING_HEADERS) {
        _processHeaderData();
    } else if (_state == READING_BODY) {
        _processBodyData();
    }
}

void Connection::_processHeaderData()
{
    DebugLogger::log("Parsing headers...");
    if (_request.parseHeaders(_inputBuffer)) {
        DebugLogger::log("Headers parsed successfully");
        
        // Log important headers
        _logHeaderInfo();

        // Get Content-Length and check against client_max_body_size limit
        size_t contentLength = _request.getHeaders().getContentLength();
        if (contentLength > 0) {
            size_t maxBodySize = _getEffectiveMaxBodySize(_request.getPath());
            if (maxBodySize > 0 && contentLength > maxBodySize) {
                DebugLogger::logError("Content-Length exceeds client_max_body_size limit");
                _handleError(HTTP_STATUS_PAYLOAD_TOO_LARGE);
                return;
            }
        }
        
        // Check for Expect: 100-continue header
        if (_request.getHeaders().get("expect") == "100-continue") {
            _handle100Continue();
        }
        
        // Headers complete, check if we need to read the body
        if (_request.isComplete()) {
            _transitionToProcessing();
        } else {
            _handleBodyAfterHeaders();
        }
    } else {
        // Headers parsing failed
        if (_request.getMethod() == Request::UNKNOWN) {
            _handleUnknownMethod();
        } else {
            DebugLogger::log("Headers not complete yet, continuing to read");
        }
    }
}

void Connection::_logHeaderInfo()
{
    std::stringstream headerLog;
    headerLog << "Content-Length: " << _request.getHeaders().getContentLength()
             << ", Content-Type: " << _request.getHeaders().getContentType();
    DebugLogger::log(headerLog.str());
    
    DebugLogger::logRequest(_clientIp, _request.getMethodStr(), 
                         _request.getPath(), _request.getHeaders().toString());
}

void Connection::_handle100Continue()
{
    DebugLogger::log("Detected Expect: 100-continue header, sending 100 Continue response");
    
    // Send 100 Continue response
    std::string continueResponse = "HTTP/1.1 100 Continue\r\n\r\n";
    send(_clientFd, continueResponse.c_str(), continueResponse.length(), 0);
}

void Connection::_handleBodyAfterHeaders()
{
    // Body expected, check if body data is already in buffer
    if (_inputBuffer.size() > 0) {
        _attemptImmediateBodyParse();
    } else {
        // No body data in buffer yet, switch to READING_BODY state
        DebugLogger::log("Body expected, moving to READING_BODY state");
        
        std::stringstream ss;
        ss << _request.getHeaders().getContentLength();
        DebugLogger::log("Content-Length: " + ss.str());
        
        _transitionToReadingBody();
    }
}

void Connection::_attemptImmediateBodyParse()
{
    DebugLogger::log("Body data already in buffer, attempting to parse immediately");
    std::stringstream bufLog;
    bufLog << "Buffer size after header parsing: " << _inputBuffer.size();
    DebugLogger::log(bufLog.str());

    // Check the content length against the limit
    size_t contentLength = _request.getHeaders().getContentLength();
    size_t maxBodySize = _getEffectiveMaxBodySize(_request.getPath());
    
    if (maxBodySize > 0 && contentLength > maxBodySize) {
        DebugLogger::logError("Content-Length exceeds client_max_body_size limit");
        _handleError(HTTP_STATUS_PAYLOAD_TOO_LARGE);
        return;
    }
    
    // Try parsing body right away
    bool parseResult = _request.parseBody(_inputBuffer);
    
    std::stringstream resultLog;
    resultLog << "Immediate body parse result: " << (parseResult ? "true" : "false")
             << ", isComplete: " << (_request.isComplete() ? "true" : "false");
    DebugLogger::log(resultLog.str());
    
    if (parseResult) {
        // Body complete, move to processing
        _transitionToProcessing();
    } else {
        // Body parsing started but not complete, continue in READING_BODY state
        DebugLogger::log("Body partially parsed, moving to READING_BODY state");
        _transitionToReadingBody();
    }
}

void Connection::_processBodyData()
{
    DebugLogger::log("Parsing body...");

    // Check current body size before processing more
    const std::string& currentBody = _request.getBody();
    size_t maxBodySize = _getEffectiveMaxBodySize(_request.getPath());
    
    // Only perform check if a limit is set (non-zero)
    if (maxBodySize > 0 && currentBody.size() > maxBodySize) {
        DebugLogger::logError("Request body exceeds client_max_body_size limit");
        _handleError(HTTP_STATUS_PAYLOAD_TOO_LARGE);
        return;
    }
    
    _logBodyParseStart();
    
    bool parseResult = _request.parseBody(_inputBuffer);
    
    _logBodyParseResult(parseResult);
    
    // Check again after parsing in case we just exceeded the limit
    if (parseResult && maxBodySize > 0 && _request.getBody().size() > maxBodySize) {
        DebugLogger::logError("Request body exceeds client_max_body_size limit after parsing");
        _handleError(HTTP_STATUS_PAYLOAD_TOO_LARGE);
        return;
    }

    if (parseResult) {
        _transitionToProcessing();
    } else {
        _logBodyParseIncomplete();
    }
}

void Connection::_logBodyParseStart()
{
    std::stringstream beforeLog;
    beforeLog << "Before parseBody call - Buffer size: " << _inputBuffer.size()
             << ", Content-Length: " << _request.getHeaders().getContentLength();
    DebugLogger::log(beforeLog.str());
}

void Connection::_logBodyParseResult(bool parseResult)
{
    std::stringstream afterLog;
    afterLog << "parseBody result: " << (parseResult ? "true" : "false")
            << ", isComplete: " << (_request.isComplete() ? "true" : "false")
            << ", Remaining buffer: " << _inputBuffer.size();
    DebugLogger::log(afterLog.str());
}

void Connection::_logBodyParseIncomplete()
{
    DebugLogger::log("Body not complete yet, continuing to read");
    
    // Print first part of body content for debugging
    if (_request.getBody().size() > 0) {
        const std::string& body = _request.getBody();
        std::string preview = body.substr(0, body.size() > 100 ? 100 : body.size());
        DebugLogger::log("Body preview: " + preview + (body.size() > 100 ? "..." : ""));
    }
}

void Connection::_handleUnknownMethod()
{
    DebugLogger::logError("Unknown HTTP method: " + _request.getMethodStr());
    
    // Create a new response
    _response = Response();
    
    // Handle the 405 error
    _handleError(HTTP_STATUS_METHOD_NOT_ALLOWED);
}

void Connection::_transitionToProcessing()
{
    DebugLogger::log("Request is complete, moving to PROCESSING state");
    _state = PROCESSING;
    
    std::stringstream bodySizeStr;
    bodySizeStr << _request.getBody().size();
    DebugLogger::log("Body size: " + bodySizeStr.str());
    
    process();
}

void Connection::_transitionToReadingBody()
{
    _state = READING_BODY;
}

void Connection::_transitionToSendingResponse()
{
    _state = SENDING_RESPONSE;
}


/*** WRITE & PROCESS RESPONSE ***/
bool Connection::writeData()
{
    if (!_isValidStateForWriting())
        return _state != CLOSED;
        
    ssize_t bytesWritten = _writeToSocket();
    
    if (bytesWritten > 0) {
        return _handleSuccessfulWrite(bytesWritten);
    } else if (bytesWritten == 0) {
        return _handleWriteSocketClosure();
    } else {
        return _handleWriteSocketError();
    }
}

bool Connection::_isValidStateForWriting() const
{
    if (_state == CLOSED) {
        DebugLogger::log("Connection is closed, not writing");
        return false;
    }
    
    // Make sure we only write when we should
    if (_state != SENDING_RESPONSE || _outputBuffer.empty()) {
        std::stringstream ss;
        ss << _state;
        ss << ", buffer size: " << _outputBuffer.size();
        DebugLogger::log("Not in writing state or buffer empty, state: " + ss.str());
        return false;
    }
    
    std::stringstream ss;
    ss << _outputBuffer.size();
    DebugLogger::log("Writing response data, buffer size: " + ss.str());
    return true;
}

ssize_t Connection::_writeToSocket()
{
    return send(_clientFd, _outputBuffer.c_str(), _outputBuffer.size(), 0);
}

bool Connection::_handleSuccessfulWrite(ssize_t bytesWritten)
{
    _updateLastActivity();
    
    _logWriteOperation(bytesWritten);
    
    // Remove sent data from buffer
    _outputBuffer.erase(0, bytesWritten);
    
    // If all data sent, move to next state
    if (_outputBuffer.empty()) {
        _handleWriteComplete();
    }
    
    return true;
}

void Connection::_logWriteOperation(ssize_t bytesWritten)
{
    std::cout << "Wrote " << bytesWritten << " bytes to " << _clientIp << std::endl;
    
    std::stringstream ss;
    ss << bytesWritten << " bytes to client, remaining: " << _outputBuffer.size();
    DebugLogger::log("Wrote " + ss.str());
}

void Connection::_handleWriteComplete()
{
    // Mark the response as sent
    _response.markAsSent();
    DebugLogger::log("Response fully sent");
    
    // Check connection header
    bool keepAlive = _request.getHeaders().keepAlive(true);
    DebugLogger::log("Keep-alive: " + std::string(keepAlive ? "yes" : "no"));
    
    if (keepAlive) {
        _prepareForNextRequest();
    } else {
        _closeAfterResponse();
    }
}

void Connection::_prepareForNextRequest()
{
    DebugLogger::log("Keeping connection alive, resetting for next request");
    _request.reset();
    _state = READING_HEADERS;
    _inputBuffer.clear();
}

void Connection::_closeAfterResponse()
{
    DebugLogger::log("Not keep-alive, closing connection");
    _state = CLOSED;
}

bool Connection::_handleWriteSocketClosure()
{
    std::cout << "Connection closed during write: " << _clientIp << std::endl;
    DebugLogger::logError("Connection closed by client during write");
    _state = CLOSED;
    return false;
}

bool Connection::_handleWriteSocketError()
{
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

/*** PROCESS OPERATIONS ***/
void Connection::process()
{
    if (!_isValidStateForProcessing())
        return;
    
    _logProcessingStart();
    _prepareNewResponse();
    
    try {
        _processRequest();
    } catch (const std::exception& e) {
        _handleProcessingException(e);
    }
    
    _buildAndPrepareResponse();
}

bool Connection::_isValidStateForProcessing() const
{
    if (_state != PROCESSING) {
        std::stringstream ss;
        ss << _state;
        DebugLogger::log("process() called but state is not PROCESSING, current state: " + ss.str());
        return false;
    }
    return true;
}

void Connection::_logProcessingStart()
{
    std::cout << "Processing " << _request.getMethodStr() << " request for " 
              << _request.getPath() << " from " << _clientIp << std::endl;
}

void Connection::_prepareNewResponse()
{
    // Create a new response for this request
    _response = Response();
}

void Connection::_handleProcessingException(const std::exception& e)
{
    // Handle any exceptions during processing
    std::cerr << "Error processing request: " << e.what() << std::endl;
    DebugLogger::logError("Exception during request processing: " + std::string(e.what()));
    _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
}

void Connection::_buildAndPrepareResponse()
{
    // Build the response and store in output buffer
    _outputBuffer = _response.build();
    
    // Log the response
    _logResponseDetails();
    
    // Move to sending response state
    _transitionToSendingResponse();
}

void Connection::_logResponseDetails()
{
    DebugLogger::logResponse(_response.getStatusCode(), _response.getHeaders().toString());
    
    std::stringstream ss;
    ss << _response.getBody().size();
    DebugLogger::log("Response body length: " + ss.str());
}

void Connection::_processRequest()
{
    DebugLogger::log("Processing request: " + _request.getMethodStr() + " " + _request.getPath());
    
    LocationConfig* location = _findAndValidateLocation();
    if (!location) {
        return; // Error already handled
    }
    
    if (!_validateRequestMethod(*location)) {
        return; // Error already handled
    }
    
    if (_checkForRedirection(*location)) {
        return; // Redirection already handled
    }
    
    _routeRequestByMethod();
}

LocationConfig* Connection::_findAndValidateLocation()
{
    LocationConfig* location = _findLocation(_request.getPath());
    if (!location) {
        DebugLogger::logError("No location block found for path: " + _request.getPath());
        _handleError(HTTP_STATUS_NOT_FOUND);
        return NULL;
    }
    
    DebugLogger::log("Found location block: " + location->getPath() + 
                   " with root: " + location->getRoot());
    return location;
}

bool Connection::_validateRequestMethod(const LocationConfig& location)
{
    std::string methodStr = _request.getMethodStr();
    const std::vector<std::string>& allowedMethods = location.getAllowedMethods();
    
    _logAllowedMethods(allowedMethods);
    
    for (std::vector<std::string>::const_iterator it = allowedMethods.begin(); 
         it != allowedMethods.end(); ++it) {
        if (*it == methodStr) {
            return true;
        }
    }
    
    DebugLogger::logError("Method " + methodStr + " not allowed for this location");
    _handleError(HTTP_STATUS_METHOD_NOT_ALLOWED);
    return false;
}

void Connection::_logAllowedMethods(const std::vector<std::string>& allowedMethods)
{
    DebugLogger::log("Allowed methods for this location:");
    for (std::vector<std::string>::const_iterator it = allowedMethods.begin();
         it != allowedMethods.end(); ++it) {
        DebugLogger::log(" - " + *it);
    }
}

bool Connection::_checkForRedirection(const LocationConfig& location)
{
    if (!location.getRedirection().empty()) {
        DebugLogger::log("This location has a redirection: " + location.getRedirection());
        _handleRedirection(location);
        return true;
    }
    return false;
}

void Connection::_routeRequestByMethod()
{
    Request::Method method = _request.getMethod();
    std::string methodStr = _request.getMethodStr();
    
    std::cout << "Request method is: " << method << std::endl;
    
    switch (method) {
        case Request::GET:
            DebugLogger::log("Handling GET request");
            _handleStaticFile();
            break;
            
        case Request::POST:
            DebugLogger::log("Handling POST request");
            _handlePostRequest();
            break;
            
        case Request::DELETE:
            DebugLogger::log("Handling DELETE request");
            _handleDeleteRequest();
            break;
            
        default:
            DebugLogger::logError("Unexpected error caused by unknown method: " + methodStr);
            _handleError(HTTP_STATUS_NOT_IMPLEMENTED);
            break;
    }
}

bool Connection::_needsTrailingSlashRedirect(const std::string& fsPath, const std::string& requestPath)
{
    // If path doesn't end with slash
    if (requestPath.empty() || requestPath[requestPath.length() - 1] != '/') {
        // Check if it's a directory
        
        return FileUtils::isDirectory(fsPath);
    }
    return false;
}

void Connection::_redirectToPathWithSlash(const std::string& requestPath)
{
    std::string redirectUrl = requestPath;
    if (!redirectUrl.empty() && redirectUrl[redirectUrl.length() - 1] != '/') {
        redirectUrl += '/';
    }
    
    DebugLogger::log("Redirecting to add trailing slash: " + redirectUrl);
    _response.redirect(redirectUrl, HTTP_STATUS_MOVED_PERMANENTLY);
}

bool Connection::_tryServeIndexFile(const std::string& dirPath, const LocationConfig& location)
{
    std::string indexFile = location.getIndex();
    if (indexFile.empty()) {
        DebugLogger::log("No index file configured for this location");
        return false;
    }
    
    std::string indexPath = FileUtils::ensureTrailingSlash(dirPath) + indexFile;
    DebugLogger::log("Trying index file: " + indexPath);
    
    if (FileUtils::isFile(indexPath)) {
        DebugLogger::log("Index file exists, serving: " + indexPath);
        _serveFile(indexPath);
        return true;
    }
    
    DebugLogger::logError("Index file not found: " + indexPath);
    return false;
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
    
    // Step 1: Check if this needs a trailing slash redirect (directory without slash)
    if (_needsTrailingSlashRedirect(fsPath, requestPath)) {
        _redirectToPathWithSlash(requestPath);
        return;
    }
    
    // Step 2: Check if the path exists
    if (FileUtils::isDirectory(fsPath)) {
        // Path is a directory with proper trailing slash
        DebugLogger::log("Path is a directory: " + fsPath);
        _handleDirectory(fsPath, requestPath, *location);
        return;
    } 
    else if (FileUtils::isFile(fsPath)) {
        // Path is a regular file
        DebugLogger::log("Serving regular file: " + fsPath);
        _serveFile(fsPath);
        return;
    }
    
    // Step 3: If we get here, try to serve index file for root requests
    if (requestPath == "/" || requestPath == "") {
        if (_tryServeIndexFile(fsPath, *location)) {
            return;
        }
    }
    
    // If we get here, file not found
    DebugLogger::logError("File not found: " + fsPath);
    _handleError(HTTP_STATUS_NOT_FOUND);
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

LocationConfig* Connection::_findLocation(const std::string& requestPath) {
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
        
        bool isMatch = false;
        size_t matchLength = 0;
        
        // CASE 1: Location with trailing slash (directory)
        if (locationPath.length() > 0 && locationPath[locationPath.length() - 1] == '/') {
            // Case 1A: Request matches exactly with location without trailing slash
            // e.g., location "/uploads/" and request "/uploads"
            std::string locationNoSlash = locationPath.substr(0, locationPath.length() - 1);
            if (requestPath == locationNoSlash) {
                isMatch = true;
                matchLength = locationPath.length(); // Use full length for prioritization
                DebugLogger::log("Match: Request matches location without trailing slash");
            }
            // Case 1B: Request starts with location (including slash)
            // e.g., location "/uploads/" and request "/uploads/file.txt"
            else if (requestPath.find(locationPath) == 0) {
                isMatch = true;
                matchLength = locationPath.length();
                DebugLogger::log("Match: Request starts with location (including slash)");
            }
        }
        // CASE 2: Location without trailing slash
        else {
            // Case 2A: Exact match
            // e.g., location "/upload" and request "/upload"
            if (requestPath == locationPath) {
                isMatch = true;
                matchLength = locationPath.length();
                DebugLogger::log("Match: Request exactly matches location");
            }
            // Case 2B: Request starts with location followed by a slash
            // e.g., location "/upload" and request "/upload/file.txt"
            else if (requestPath.find(locationPath) == 0 && 
                     requestPath.length() > locationPath.length() && 
                     requestPath[locationPath.length()] == '/') {
                isMatch = true;
                matchLength = locationPath.length();
                DebugLogger::log("Match: Request starts with location followed by slash");
            }
        }
        
        if (isMatch && matchLength > bestMatchLength) {
            bestMatch = *it;
            bestMatchLength = matchLength;
            
            std::stringstream ss;
            ss << matchLength;
            DebugLogger::log("New best match: '" + locationPath + "' (length: " + ss.str() + ")");
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

void Connection::_handleDirectory(const std::string& fsPath, const std::string& requestPath, 
    const LocationConfig& location)
{
    DebugLogger::log("Handling directory: " + fsPath + " for request path: " + requestPath);

    // Step 1: Ensure request path ends with slash
    if (requestPath[requestPath.length() - 1] != '/') {
        _redirectToPathWithSlash(requestPath);
        return;
    }

    // Step 2: Try to serve an index file
    if (_tryServeIndexFile(fsPath, location)) {
        return;
    }

    // Step 3: If no index file or autoindex is off, return 403 Forbidden
    if (!location.getAutoIndex()) {
        DebugLogger::logError("No index file and autoindex is off, returning 403 Forbidden");
        _handleError(HTTP_STATUS_FORBIDDEN);
        return;
    }

    // Step 4: Generate directory listing
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
    // Get file extension
    std::string extension = "";
    size_t dotPos = fsPath.find_last_of('.');
    if (dotPos != std::string::npos) {
        extension = fsPath.substr(dotPos);
    }
    
    // Check if we have an interpreter for this extension
    std::string interpreter = location.getInterpreterForExtension(extension);
    if (interpreter.empty()) {
        // No interpreter found - fall back to legacy behavior
        if (location.getCgiPath().empty()) {
            DebugLogger::logError("No CGI interpreter found for extension: " + extension);
            _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            return;
        } else {
            interpreter = location.getCgiPath();
            DebugLogger::log("Using legacy CGI path: " + interpreter);
        }
    } else {
        DebugLogger::log("Using interpreter from cgi_handler: " + interpreter + " for " + extension);
    }
    
    // Create CGI handler
    CGIHandler cgiHandler;
    
    // Execute CGI script
    if (cgiHandler.executeCGI(_request, fsPath, location, _response)) {
        // CGI execution completed (may have errors but produced a response)
        if (cgiHandler.hasExecutionError()) {
            // CGI executed but with errors - check if we got a response body
            if (_response.getBody().empty()) {
                // No content produced, return 500 error
                DebugLogger::logError("CGI execution error with no content produced: " + fsPath);
                _handleError(HTTP_STATUS_INTERNAL_SERVER_ERROR);
            } else {
                // We got content despite errors, use the response as is
                std::stringstream ss;
                ss << "CGI execution completed with errors (exit code: " 
                   << cgiHandler.getExitStatus() 
                   << ") but produced content: " << fsPath;
                DebugLogger::logError(ss.str());
                
                // The response has already been set by executeCGI
                // Just log that we're using it despite errors
                DebugLogger::log("Using CGI output despite execution errors");
            }
        } else {
            // Successful execution
            DebugLogger::log("CGI execution successful for: " + fsPath);
        }
    } else {
        // CGI execution failed completely
        DebugLogger::logError("CGI execution failed for: " + fsPath);
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
 * @brief Determine the effective client max body size for the current request
 * 
 * This checks the location config first, then falls back to server config if needed.
 * 
 * @param requestPath The request path to find matching location for
 * @return size_t The effective max body size (0 means unlimited)
 */
size_t Connection::_getEffectiveMaxBodySize(const std::string& requestPath)
{
    // Find the location for this request path
    LocationConfig* location = _findLocation(requestPath);
    
    if (!location) {
        // If no location matches, use server default
        return _serverConfig->getClientMaxBodySize();
    }
    
    // Get location value
    size_t maxBodySize = location->getClientMaxBodySize();
    
    // If using default, fall back to server value
    if (maxBodySize == DEFAULT_CLIENT_SIZE) {
        maxBodySize = _serverConfig->getClientMaxBodySize();
    }
    
    return maxBodySize;
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
        
        // Get the appropriate client_max_body_size value for this location
        size_t client_max_body_size = location.getClientMaxBodySize();

        // If not set at the location level, fall back to server's value
        if (client_max_body_size == DEFAULT_CLIENT_SIZE)
            client_max_body_size = _serverConfig->getClientMaxBodySize();

        // Check file size if a limit is set (non-zero)
        if (client_max_body_size != 0 && files[i].content.size() > client_max_body_size) {
            responseBody << "    <li>\r\n"
                        << "      <strong>Error:</strong> File too large: " << originalFilename << " (" 
                        << FileUtils::formatFileSize(files[i].content.size()) << " exceeds limit of "
                        << FileUtils::formatFileSize(client_max_body_size) << ")<br>\r\n"
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
    
    // Build the response and store in output buffer
    _outputBuffer = _response.build();
    
    // Log the response
    DebugLogger::logResponse(_response.getStatusCode(), _response.getHeaders().toString());
    
    // Transition to sending response state
    _transitionToSendingResponse();

    DebugLogger::log("Error response prepared and ready to send");
    
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