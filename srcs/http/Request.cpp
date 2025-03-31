#include "Request.hpp"
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cctype>

Request::Request()
    : _method(UNKNOWN), _uri(), _path(), _queryString(), _queryParams(), 
      _version(), _headers(), _body(), _complete(false), 
      _headersParsed(false), _bodyBytesRead(0)
{
}

Request::~Request()
{
}

bool Request::parse(std::string& buffer)
{
    if (!_headersParsed) {
        if (!parseHeaders(buffer)) {
            return false;
        }
    }
    
    if (_headersParsed) {
        if (!parseBody(buffer)) {
            return false;
        }
    }
    
    return _complete;
}

bool Request::parseHeaders(std::string& buffer)
{
    if (_headersParsed) {
        DebugLogger::log("Headers already parsed");
        return true;
    }
    
    // Look for end of headers (double CRLF)
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // Not enough data yet
        std::stringstream ss;
        ss << buffer.size();
        DebugLogger::log("End of headers not found yet, buffer size: " + ss.str());
        return false;
    }
    
    // Extract headers section
    std::string headers = buffer.substr(0, headerEnd + 4);
    buffer.erase(0, headerEnd + 4);
    
    std::stringstream ss;
    ss << headerEnd;
    DebugLogger::log("Found end of headers at position " + ss.str());
    DebugLogger::log("Headers raw data:");
    DebugLogger::log(headers);
    
    // Parse headers section
    std::istringstream stream(headers);
    std::string line;
    
    // Parse request line
    if (!std::getline(stream, line)) {
        DebugLogger::logError("Failed to read request line from headers");
        return false;
    }
    
    // Remove trailing CR if present
    if (!line.empty() && line[line.length() - 1] == '\r') {
        line.erase(line.length() - 1);
    }
    
    DebugLogger::log("Request line: " + line);
    
    if (!_parseRequestLine(line)) {
        DebugLogger::logError("Failed to parse request line: " + line);
        return false;
    }
    
    // Parse headers
    std::string headerBlock;
    while (std::getline(stream, line)) {
        // Skip the empty line after headers
        if (line == "\r" || line.empty()) {
            DebugLogger::log("Found empty line, end of headers");
            break;
        }
        
        // Remove trailing CR if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        DebugLogger::log("Header line: " + line);
        headerBlock += line + "\n";
    }
    
    if (!_headers.parse(headerBlock)) {
        DebugLogger::logError("Failed to parse headers block");
        return false;
    }
    
    // If we got here, headers parsed successfully
    _headersParsed = true;
    
    // Dump parsed headers
    DebugLogger::log("Parsed headers:");
    const std::map<std::string, std::string>& headerMap = _headers.getAll();
    for (std::map<std::string, std::string>::const_iterator it = headerMap.begin(); 
         it != headerMap.end(); ++it) {
        DebugLogger::log(it->first + ": " + it->second);
    }
    
    // If no body expected, request is complete
    if (_method == GET || (_headers.getContentLength() == 0 && !_headers.hasChunkedEncoding())) {
        DebugLogger::log("No body expected, request is complete");
        _complete = true;
    } else if (_headers.hasChunkedEncoding()) {
        DebugLogger::log("Request uses chunked encoding");
    } else {
        std::stringstream lenStr;
        lenStr << _headers.getContentLength();
        DebugLogger::log("Body expected, Content-Length: " + lenStr.str());
    }
    
    return true;
}

bool Request::parseBody(std::string& buffer)
{
    if (!_headersParsed) {
        DebugLogger::logError("Cannot parse body before headers");
        return false;
    }
    
    if (_complete) {
        DebugLogger::log("Request already complete");
        return true;
    }
    
    // Handle chunked encoding
    if (_headers.hasChunkedEncoding()) {
        DebugLogger::log("Processing chunked body");
        return _parseChunkedBody(buffer);
    }
    
    // Handle normal body with Content-Length
    size_t contentLength = _headers.getContentLength();
    if (contentLength > 0) {
        // Check if we have enough data
        size_t remainingBytes = contentLength - _bodyBytesRead;
        size_t bytesToRead = buffer.size() < remainingBytes ? buffer.size() : remainingBytes;
        
        std::stringstream ss;
        ss << contentLength << ", Read so far: " << _bodyBytesRead 
           << ", Remaining: " << remainingBytes
           << ", Available: " << buffer.size()
           << ", Will read: " << bytesToRead;
        DebugLogger::log("Content-Length: " + ss.str());
        
        // Append to body
        _body.append(buffer, 0, bytesToRead);
        _bodyBytesRead += bytesToRead;
        
        // Remove read data from buffer
        buffer.erase(0, bytesToRead);
        
        // Check if body is complete
        if (_bodyBytesRead >= contentLength) {
            std::stringstream sizeStr;
            sizeStr << _body.size();
            DebugLogger::log("Body complete, total size: " + sizeStr.str());
            _complete = true;
        } else {
            std::stringstream remainStr;
            remainStr << (contentLength - _bodyBytesRead);
            DebugLogger::log("Body not complete, need " + remainStr.str() + " more bytes");
        }
    } else {
        // No body expected
        DebugLogger::log("No body expected (Content-Length is 0)");
        _complete = true;
    }
    
    return _complete;
}

bool Request::_parseChunkedBody(std::string& buffer)
{
    std::stringstream ss;
    ss << buffer.size();
    DebugLogger::log("Parsing chunked body, buffer size: " + ss.str());
    
    // Dump the first 100 bytes of the buffer for debugging
    size_t dumpSize = buffer.size() < 100 ? buffer.size() : 100;
    DebugLogger::hexDump("Chunk buffer", buffer.substr(0, dumpSize));
    
    // Chunked encoding format:
    // [chunk size in hex]\r\n
    // [chunk data]\r\n
    // ...
    // 0\r\n
    // \r\n
    
    while (!buffer.empty()) {
        // Check if we're at the end of the chunked data
        if (buffer.find("0\r\n\r\n") == 0 || buffer.find("0\r\n") == 0) {
            DebugLogger::log("Found end chunk marker");
            
            // Check for the terminating sequence
            if (buffer.find("0\r\n\r\n") == 0) {
                buffer.erase(0, 5); // Remove "0\r\n\r\n"
            } else if (buffer.find("0\r\n") == 0) {
                buffer.erase(0, 3); // Remove "0\r\n"
                
                // Check if there's another \r\n after this
                if (buffer.find("\r\n") == 0) {
                    buffer.erase(0, 2); // Remove additional "\r\n"
                }
            }
            
            _complete = true;
            
            std::stringstream sizeStr;
            sizeStr << _body.size();
            DebugLogger::log("Chunked body complete, total size: " + sizeStr.str());
            return true;
        }
        
        // Find chunk size line
        size_t crlfPos = buffer.find("\r\n");
        if (crlfPos == std::string::npos) {
            // Not enough data to read chunk size
            DebugLogger::log("Chunk size line not complete yet");
            return false;
        }
        
        // Parse chunk size
        std::string hexSize = buffer.substr(0, crlfPos);
        DebugLogger::log("Chunk size (hex): " + hexSize);
        
        // Skip any chunk extensions
        size_t semicolonPos = hexSize.find(';');
        if (semicolonPos != std::string::npos) {
            hexSize = hexSize.substr(0, semicolonPos);
            DebugLogger::log("Trimmed chunk size (hex): " + hexSize);
        }
        
        // Convert hex to decimal
        unsigned long chunkSize = 0;
        std::istringstream iss(hexSize);
        iss >> std::hex >> chunkSize;
        
        if (!iss) {
            // Invalid hex format
            DebugLogger::logError("Invalid chunk size format: " + hexSize);
            return false;
        }
        
        std::stringstream chunkSizeStr;
        chunkSizeStr << chunkSize;
        DebugLogger::log("Chunk size (decimal): " + chunkSizeStr.str());
        
        // Remove size line and CRLF from buffer
        buffer.erase(0, crlfPos + 2);
        
        if (chunkSize == 0) {
            // Last chunk
            if (buffer.find("\r\n") == 0) {
                buffer.erase(0, 2); // Remove final CRLF
                _complete = true;
                
                std::stringstream bodySizeStr;
                bodySizeStr << _body.size();
                DebugLogger::log("Chunked body complete (zero-size chunk), total size: " + 
                             bodySizeStr.str());
                return true;
            } else {
                // Not enough data to finish
                DebugLogger::log("Waiting for final CRLF after zero-size chunk");
                return false;
            }
        }
        
        // Check if we have the full chunk data plus CRLF
        if (buffer.size() < chunkSize + 2) {
            // Not enough data
            std::stringstream ss;
            ss << buffer.size() << " bytes, need " << (chunkSize + 2) << " bytes";
            DebugLogger::log("Not enough data for complete chunk, have " + ss.str());
            return false;
        }
        
        // Append chunk data to body
        _body.append(buffer, 0, chunkSize);
        _bodyBytesRead += chunkSize;
        
        // Check if the chunk is properly terminated with CRLF
        if (buffer.substr(chunkSize, 2) != "\r\n") {
            DebugLogger::logError("Chunk not terminated with CRLF");
            DebugLogger::hexDump("Chunk termination", buffer.substr(chunkSize, 4));
            return false;
        }
        
        // Remove chunk data and CRLF from buffer
        buffer.erase(0, chunkSize + 2);
        
        std::stringstream processedStr;
        processedStr << chunkSize << " bytes, remaining buffer size: " << buffer.size();
        DebugLogger::log("Processed chunk of " + processedStr.str());
    }
    
    DebugLogger::log("Need more data for chunked body");
    return false; // Need more data
}

bool Request::_parseRequestLine(const std::string& line)
{
    std::istringstream iss(line);
    std::string methodStr, uri, version;
    
    iss >> methodStr >> uri >> version;
    
    // Check if all three parts were read
    if (methodStr.empty() || uri.empty() || version.empty()) {
        DebugLogger::logError("Invalid request line format, missing components");
        return false;
    }
    
    DebugLogger::log("Method: " + methodStr + ", URI: " + uri + ", Version: " + version);
    
    // Parse method
    _method = parseMethod(methodStr);
    if (_method == UNKNOWN) {
        DebugLogger::logError("Unknown HTTP method: " + methodStr);
        return false;
    }
    
    // Store URI
    _uri = uri;
    
    // Parse URI to get path and query string
    size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos) {
        _path = uri.substr(0, queryPos);
        _queryString = uri.substr(queryPos + 1);
        DebugLogger::log("Path: " + _path + ", Query string: " + _queryString);
        _parseQueryParams();
    } else {
        _path = uri;
        _queryString = "";
        DebugLogger::log("Path: " + _path + " (no query string)");
    }
    
    // Check HTTP version
    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
        DebugLogger::logError("Unsupported HTTP version: " + version);
        return false;
    }
    
    _version = version;
    
    return true;
}

void Request::_parseQueryParams()
{
    _queryParams.clear();
    
    std::string currParam = _queryString;
    std::string pair;
    
    while (!currParam.empty()) {
        size_t ampPos = currParam.find('&');
        
        if (ampPos != std::string::npos) {
            pair = currParam.substr(0, ampPos);
            currParam = currParam.substr(ampPos + 1);
        } else {
            pair = currParam;
            currParam = "";
        }
        
        size_t eqPos = pair.find('=');
        if (eqPos != std::string::npos) {
            std::string key = pair.substr(0, eqPos);
            std::string value = pair.substr(eqPos + 1);
            
            // URL decode key and value
            _queryParams[StringUtils::urlDecode(key)] = StringUtils::urlDecode(value);
        } else if (!pair.empty()) {
            // Handle parameters with no value
            _queryParams[StringUtils::urlDecode(pair)] = "";
        }
    }
}

void Request::reset()
{
    _method = UNKNOWN;
    _uri.clear();
    _path.clear();
    _queryString.clear();
    _queryParams.clear();
    _version.clear();
    _headers.clear();
    _body.clear();
    _complete = false;
    _headersParsed = false;
    _bodyBytesRead = 0;
}

Request::Method Request::getMethod() const
{
    return _method;
}

std::string Request::getMethodStr() const
{
    return methodToString(_method);
}

const std::string& Request::getUri() const
{
    return _uri;
}

const std::string& Request::getPath() const
{
    return _path;
}

const std::string& Request::getQueryString() const
{
    return _queryString;
}

std::string Request::getQueryParam(const std::string& name) const
{
    std::map<std::string, std::string>::const_iterator it = _queryParams.find(name);
    if (it != _queryParams.end()) {
        return it->second;
    }
    return "";
}

const std::map<std::string, std::string>& Request::getQueryParams() const
{
    return _queryParams;
}

const std::string& Request::getVersion() const
{
    return _version;
}

const Headers& Request::getHeaders() const
{
    return _headers;
}

Headers& Request::getHeaders()
{
    return _headers;
}

const std::string& Request::getBody() const
{
    return _body;
}

bool Request::isComplete() const
{
    return _complete;
}

std::string Request::getHost() const
{
    return _headers.get("host");
}

std::string Request::toString() const
{
    std::stringstream ss;
    
    ss << getMethodStr() << " " << _uri << " " << _version << "\r\n";
    ss << _headers.toString();
    ss << "\r\n";
    
    if (!_body.empty()) {
        ss << _body;
    }
    
    return ss.str();
}

Request::Method Request::parseMethod(const std::string& method)
{
    if (method == "GET") {
        return GET;
    } else if (method == "POST") {
        return POST;
    } else if (method == "DELETE") {
        return DELETE;
    } else {
        return UNKNOWN;
    }
}

std::string Request::methodToString(Method method)
{
    switch (method) {
        case GET:
            return "GET";
        case POST:
            return "POST";
        case DELETE:
            return "DELETE";
        case UNKNOWN:
        default:
            return "UNKNOWN";
    }
}