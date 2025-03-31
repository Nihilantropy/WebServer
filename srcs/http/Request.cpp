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
        return true;
    }
    
    // Look for end of headers (double CRLF)
    size_t headerEnd = buffer.find("\r\n\r\n");
    if (headerEnd == std::string::npos) {
        // Not enough data yet
        return false;
    }
    
    // Extract headers section
    std::string headers = buffer.substr(0, headerEnd + 4);
    buffer.erase(0, headerEnd + 4);
    
    // Parse headers section
    std::istringstream stream(headers);
    std::string line;
    
    // Parse request line
    if (!std::getline(stream, line)) {
        return false;
    }
    
    // Remove trailing CR if present
    if (!line.empty() && line[line.length() - 1] == '\r') {
        line.erase(line.length() - 1);
    }
    
    if (!_parseRequestLine(line)) {
        return false;
    }
    
    // Parse headers
    std::string headerBlock;
    while (std::getline(stream, line)) {
        // Skip the empty line after headers
        if (line == "\r" || line.empty()) {
            break;
        }
        
        // Remove trailing CR if present
        if (!line.empty() && line[line.length() - 1] == '\r') {
            line.erase(line.length() - 1);
        }
        
        headerBlock += line + "\n";
    }
    
    if (!_headers.parse(headerBlock)) {
        return false;
    }
    
    _headersParsed = true;
    
    // If no body expected, request is complete
    if (_method == GET || _headers.getContentLength() == 0) {
        _complete = true;
    }
    
    return true;
}

bool Request::parseBody(std::string& buffer)
{
    if (!_headersParsed) {
        return false;
    }
    
    if (_complete) {
        return true;
    }
    
    // Handle chunked encoding
    if (_headers.hasChunkedEncoding()) {
        return _parseChunkedBody(buffer);
    }
    
    // Handle normal body with Content-Length
    size_t contentLength = _headers.getContentLength();
    if (contentLength > 0) {
        // Check if we have enough data
        size_t remainingBytes = contentLength - _bodyBytesRead;
        size_t bytesToRead = buffer.size() < remainingBytes ? buffer.size() : remainingBytes;
        
        // Append to body
        _body.append(buffer, 0, bytesToRead);
        _bodyBytesRead += bytesToRead;
        
        // Remove read data from buffer
        buffer.erase(0, bytesToRead);
        
        // Check if body is complete
        if (_bodyBytesRead >= contentLength) {
            _complete = true;
        }
    } else {
        // No body expected
        _complete = true;
    }
    
    return _complete;
}

bool Request::_parseChunkedBody(std::string& buffer)
{
    // Chunked encoding format:
    // [chunk size in hex]\r\n
    // [chunk data]\r\n
    // ...
    // 0\r\n
    // \r\n
    
    while (!buffer.empty()) {
        // Check if we're at the end of the chunked data
        if (_bodyBytesRead > 0 && buffer.find("0\r\n\r\n") == 0) {
            buffer.erase(0, 5); // Remove the end marker
            _complete = true;
            return true;
        }
        
        // Find chunk size line
        size_t crlfPos = buffer.find("\r\n");
        if (crlfPos == std::string::npos) {
            // Not enough data to read chunk size
            return false;
        }
        
        // Parse chunk size
        std::string hexSize = buffer.substr(0, crlfPos);
        buffer.erase(0, crlfPos + 2); // Remove size line and CRLF
        
        // Convert hex to decimal
        unsigned long chunkSize = 0;
        std::istringstream iss(hexSize);
        iss >> std::hex >> chunkSize;
        
        if (chunkSize == 0) {
            // Last chunk
            if (buffer.find("\r\n") == 0) {
                buffer.erase(0, 2); // Remove final CRLF
                _complete = true;
                return true;
            } else {
                // Not enough data to finish
                return false;
            }
        }
        
        // Check if we have the full chunk data plus CRLF
        if (buffer.size() < chunkSize + 2) {
            // Not enough data
            return false;
        }
        
        // Append chunk data to body
        _body.append(buffer, 0, chunkSize);
        _bodyBytesRead += chunkSize;
        
        // Remove chunk data and CRLF from buffer
        buffer.erase(0, chunkSize + 2);
    }
    
    return false; // Need more data
}

bool Request::_parseRequestLine(const std::string& line)
{
    std::istringstream iss(line);
    std::string methodStr, uri, version;
    
    iss >> methodStr >> uri >> version;
    
    // Check if all three parts were read
    if (methodStr.empty() || uri.empty() || version.empty()) {
        return false;
    }
    
    // Parse method
    _method = parseMethod(methodStr);
    if (_method == UNKNOWN) {
        return false;
    }
    
    // Store URI
    _uri = uri;
    
    // Parse URI to get path and query string
    size_t queryPos = uri.find('?');
    if (queryPos != std::string::npos) {
        _path = uri.substr(0, queryPos);
        _queryString = uri.substr(queryPos + 1);
        _parseQueryParams();
    } else {
        _path = uri;
        _queryString = "";
    }
    
    // Check HTTP version
    if (version != "HTTP/1.0" && version != "HTTP/1.1") {
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