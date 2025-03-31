#include "Response.hpp"
#include <sstream>

Response::Response()
    : _statusCode(HTTP_STATUS_OK), _statusMessage(getReasonPhrase(HTTP_STATUS_OK)),
      _version("HTTP/1.1"), _headers(), _body(), _sent(false)
{
    // Set default headers
    _headers.set("Server", "WebServer/1.0");
    _headers.set("Connection", "keep-alive");
}

Response::Response(int statusCode)
    : _statusCode(statusCode), _statusMessage(getReasonPhrase(statusCode)),
      _version("HTTP/1.1"), _headers(), _body(), _sent(false)
{
    // Set default headers
    _headers.set("Server", "WebServer/1.0");
    _headers.set("Connection", "keep-alive");
}

Response::~Response()
{
}

void Response::setStatusCode(int statusCode)
{
    _statusCode = statusCode;
    _statusMessage = getReasonPhrase(statusCode);
}

void Response::setVersion(const std::string& version)
{
    _version = version;
}

void Response::setBody(const std::string& body, const std::string& contentType)
{
    _body = body;
    setContentType(contentType);
    setContentLength(_body.size());
}

void Response::setHeader(const std::string& name, const std::string& value)
{
    _headers.set(name, value);
}

Headers& Response::getHeaders()
{
    return _headers;
}

std::string Response::build()
{
    std::stringstream statusStr;
    statusStr << _statusCode;
    DebugLogger::log("Building response with status code: " + statusStr.str());
    
    std::stringstream ss;
    
    // Status line
    ss << _version << " " << _statusCode << " " << _statusMessage << "\r\n";
    
    // Make sure Content-Length is set if we have a body
    if (!_body.empty() && !_headers.contains("content-length")) {
        setContentLength(_body.size());
        
        std::stringstream lenStr;
        lenStr << _body.size();
        DebugLogger::log("Added Content-Length: " + lenStr.str());
    }
    
    // Headers
    ss << _headers.toString();
    DebugLogger::log("Added response headers");
    
    // Empty line
    ss << "\r\n";
    
    // Body
    if (!_body.empty()) {
        ss << _body;
        
        std::stringstream sizeStr;
        sizeStr << _body.size();
        DebugLogger::log("Added response body, size: " + sizeStr.str());
    }
    
    std::string result = ss.str();
    
    std::stringstream resultSizeStr;
    resultSizeStr << result.size();
    DebugLogger::log("Complete response size: " + resultSizeStr.str());
    
    return result;
}

void Response::redirect(const std::string& location, int code)
{
    setStatusCode(code);
    setHeader("Location", location);
    setBody("<html><head><title>Redirect</title></head><body><h1>Redirect</h1><p>Redirecting to <a href=\"" + 
            location + "\">" + location + "</a></p></body></html>");
}

void Response::setContentType(const std::string& contentType)
{
    setHeader("Content-Type", contentType);
}

void Response::setContentLength(size_t length)
{
    std::stringstream ss;
    ss << length;
    setHeader("Content-Length", ss.str());
}

int Response::getStatusCode() const
{
    return _statusCode;
}

const std::string& Response::getStatusMessage() const
{
    return _statusMessage;
}

const std::string& Response::getVersion() const
{
    return _version;
}

const std::string& Response::getBody() const
{
    return _body;
}

bool Response::isSent() const
{
    return _sent;
}

void Response::markAsSent()
{
    _sent = true;
}