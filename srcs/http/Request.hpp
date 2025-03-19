#pragma once

#include <string>
#include <map>
#include "Headers.hpp"

/**
 * @brief Class to represent and parse an HTTP request
 * 
 * This class handles parsing of HTTP request lines, headers, and bodies,
 * and provides access to the parsed components.
 */
class Request {
public:
    /**
     * @brief HTTP Methods
     */
    enum Method {
        GET,
        POST,
        DELETE,
        UNKNOWN
    };

private:
    Method _method;                               // HTTP method
    std::string _uri;                             // Request URI
    std::string _path;                            // URI path component
    std::string _queryString;                     // URI query string
    std::map<std::string, std::string> _queryParams; // Parsed query parameters
    std::string _version;                         // HTTP version
    Headers _headers;                             // HTTP headers
    std::string _body;                            // Request body
    bool _complete;                               // Whether the request is complete
    
    // Parsing state
    bool _headersParsed;                          // Whether headers have been parsed
    size_t _bodyBytesRead;                        // Number of body bytes read so far
    
    /**
     * @brief Parse the HTTP request line
     * 
     * @param line Request line to parse
     * @return bool True if parsing was successful
     */
    bool _parseRequestLine(const std::string& line);
    
    /**
     * @brief Parse query parameters from the query string
     */
    void _parseQueryParams();
    
    /**
     * @brief Parse a chunked request body
     * 
     * @param buffer Buffer containing the body data
     * @return bool True if the body is complete
     */
    bool _parseChunkedBody(std::string& buffer);

public:
    /**
     * @brief Construct a new Request object
     */
    Request();
    
    /**
     * @brief Destroy the Request object
     */
    ~Request();

    /**
     * @brief Parse the request from a buffer
     * 
     * @param buffer Buffer containing the request data
     * @return bool True if the request is complete
     */
    bool parse(std::string& buffer);
    
    /**
     * @brief Parse the HTTP headers from a buffer
     * 
     * @param buffer Buffer containing the header data
     * @return bool True if headers are completely parsed
     */
    bool parseHeaders(std::string& buffer);
    
    /**
     * @brief Parse the HTTP body from a buffer
     * 
     * @param buffer Buffer containing the body data
     * @return bool True if the body is complete
     */
    bool parseBody(std::string& buffer);
    
    /**
     * @brief Reset the request to its initial state
     */
    void reset();
    
    /**
     * @brief Get the HTTP method
     * 
     * @return Method The HTTP method
     */
    Method getMethod() const;
    
    /**
     * @brief Get the HTTP method as a string
     * 
     * @return std::string The HTTP method
     */
    std::string getMethodStr() const;
    
    /**
     * @brief Get the request URI
     * 
     * @return const std::string& The request URI
     */
    const std::string& getUri() const;
    
    /**
     * @brief Get the request path
     * 
     * @return const std::string& The request path
     */
    const std::string& getPath() const;
    
    /**
     * @brief Get the query string
     * 
     * @return const std::string& The query string
     */
    const std::string& getQueryString() const;
    
    /**
     * @brief Get a query parameter value
     * 
     * @param name Parameter name
     * @return std::string Parameter value, empty string if not found
     */
    std::string getQueryParam(const std::string& name) const;
    
    /**
     * @brief Get all query parameters
     * 
     * @return const std::map<std::string, std::string>& Map of query parameters
     */
    const std::map<std::string, std::string>& getQueryParams() const;
    
    /**
     * @brief Get the HTTP version
     * 
     * @return const std::string& The HTTP version
     */
    const std::string& getVersion() const;
    
    /**
     * @brief Get the HTTP headers
     * 
     * @return const Headers& The HTTP headers
     */
    const Headers& getHeaders() const;
    
    /**
     * @brief Get mutable HTTP headers
     * 
     * @return Headers& The HTTP headers
     */
    Headers& getHeaders();
    
    /**
     * @brief Get the request body
     * 
     * @return const std::string& The request body
     */
    const std::string& getBody() const;
    
    /**
     * @brief Check if the request is complete
     * 
     * @return bool True if the request is complete
     */
    bool isComplete() const;
    
    /**
     * @brief Get the host header value
     * 
     * @return std::string The host value
     */
    std::string getHost() const;
    
    /**
     * @brief Convert the request to a string
     * 
     * @return std::string The request as a string
     */
    std::string toString() const;
    
    /**
     * @brief Parse a method string to enum
     * 
     * @param method Method string
     * @return Method Method enum
     */
    static Method parseMethod(const std::string& method);
    
    /**
     * @brief Convert a method enum to string
     * 
     * @param method Method enum
     * @return std::string Method string
     */
    static std::string methodToString(Method method);
};