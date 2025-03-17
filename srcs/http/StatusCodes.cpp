#include "StatusCodes.hpp"

// Static map of status codes to reason phrases
const std::map<int, std::string>& getStatusCodesMap()
{
    static std::map<int, std::string> statusCodes;
    
    // Initialize on first call
    if (statusCodes.empty()) {
        // 2xx Success
        statusCodes[HTTP_STATUS_OK] = "OK";
        statusCodes[HTTP_STATUS_CREATED] = "Created";
        statusCodes[HTTP_STATUS_ACCEPTED] = "Accepted";
        statusCodes[HTTP_STATUS_NO_CONTENT] = "No Content";
        
        // 3xx Redirection
        statusCodes[HTTP_STATUS_MOVED_PERMANENTLY] = "Moved Permanently";
        statusCodes[HTTP_STATUS_FOUND] = "Found";
        statusCodes[HTTP_STATUS_SEE_OTHER] = "See Other";
        statusCodes[HTTP_STATUS_NOT_MODIFIED] = "Not Modified";
        statusCodes[HTTP_STATUS_TEMPORARY_REDIRECT] = "Temporary Redirect";
        statusCodes[HTTP_STATUS_PERMANENT_REDIRECT] = "Permanent Redirect";
        
        // 4xx Client Error
        statusCodes[HTTP_STATUS_BAD_REQUEST] = "Bad Request";
        statusCodes[HTTP_STATUS_UNAUTHORIZED] = "Unauthorized";
        statusCodes[HTTP_STATUS_FORBIDDEN] = "Forbidden";
        statusCodes[HTTP_STATUS_NOT_FOUND] = "Not Found";
        statusCodes[HTTP_STATUS_METHOD_NOT_ALLOWED] = "Method Not Allowed";
        statusCodes[HTTP_STATUS_REQUEST_TIMEOUT] = "Request Timeout";
        statusCodes[HTTP_STATUS_LENGTH_REQUIRED] = "Length Required";
        statusCodes[HTTP_STATUS_PAYLOAD_TOO_LARGE] = "Payload Too Large";
        
        // 5xx Server Error
        statusCodes[HTTP_STATUS_INTERNAL_SERVER_ERROR] = "Internal Server Error";
        statusCodes[HTTP_STATUS_NOT_IMPLEMENTED] = "Not Implemented";
        statusCodes[HTTP_STATUS_BAD_GATEWAY] = "Bad Gateway";
        statusCodes[HTTP_STATUS_SERVICE_UNAVAILABLE] = "Service Unavailable";
        statusCodes[HTTP_STATUS_GATEWAY_TIMEOUT] = "Gateway Timeout";
        statusCodes[HTTP_STATUS_HTTP_VERSION_NOT_SUPPORTED] = "HTTP Version Not Supported";
    }
    
    return statusCodes;
}

std::string getReasonPhrase(int statusCode)
{
    const std::map<int, std::string>& statusCodes = getStatusCodesMap();
    
    std::map<int, std::string>::const_iterator it = statusCodes.find(statusCode);
    if (it != statusCodes.end()) {
        return it->second;
    }
    
    return "Unknown Status";
}