#pragma once

#include <string>
#include "Headers.hpp"
#include "StatusCodes.hpp"

/**
 * @brief Class to represent and generate an HTTP response
 * 
 * This class provides methods for building HTTP responses,
 * including status lines, headers, and body content.
 */
class Response {
private:
    int _statusCode;              // HTTP status code
    std::string _statusMessage;   // HTTP status message
    std::string _version;         // HTTP version
    Headers _headers;             // HTTP headers
    std::string _body;            // Response body
    bool _sent;                   // Whether the response has been sent

public:
    /**
     * @brief Construct a new Response object with default values
     */
    Response();
    
    /**
     * @brief Construct a new Response object with a status code
     * 
     * @param statusCode HTTP status code
     */
    Response(int statusCode);
    
    /**
     * @brief Destroy the Response object
     */
    ~Response();

    /**
     * @brief Set the HTTP status code
     * 
     * @param statusCode HTTP status code
     */
    void setStatusCode(int statusCode);
    
    /**
     * @brief Set the HTTP version
     * 
     * @param version HTTP version
     */
    void setVersion(const std::string& version);
    
    /**
     * @brief Set the response body
     * 
     * @param body Body content
     * @param contentType Content type (default: text/html)
     */
    void setBody(const std::string& body, const std::string& contentType = "text/html");
    
    /**
     * @brief Set a response header
     * 
     * @param name Header name
     * @param value Header value
     */
    void setHeader(const std::string& name, const std::string& value);
    
    /**
     * @brief Get all headers
     * 
     * @return Headers& Headers object
     */
    Headers& getHeaders();
    
    /**
     * @brief Build the complete HTTP response
     * 
     * @return std::string The response as a string
     */
    std::string build();
    
    /**
     * @brief Create a redirect response
     * 
     * @param location Redirect location
     * @param code HTTP status code (default: 302 Found)
     */
    void redirect(const std::string& location, int code = HTTP_STATUS_FOUND);
    
    /**
     * @brief Set the Content-Type header
     * 
     * @param contentType Content type
     */
    void setContentType(const std::string& contentType);
    
    /**
     * @brief Set the Content-Length header
     * 
     * @param length Content length
     */
    void setContentLength(size_t length);
    
    /**
     * @brief Get the HTTP status code
     * 
     * @return int HTTP status code
     */
    int getStatusCode() const;
    
    /**
     * @brief Get the HTTP status message
     * 
     * @return const std::string& HTTP status message
     */
    const std::string& getStatusMessage() const;
    
    /**
     * @brief Get the HTTP version
     * 
     * @return const std::string& HTTP version
     */
    const std::string& getVersion() const;
    
    /**
     * @brief Get the response body
     * 
     * @return const std::string& Response body
     */
    const std::string& getBody() const;
    
    /**
     * @brief Check if the response has been sent
     * 
     * @return bool True if the response has been sent
     */
    bool isSent() const;
    
    /**
     * @brief Mark the response as sent
     */
    void markAsSent();
};