#pragma once

#include <map>
#include <string>
#include <sstream>
#include "../utils/StringUtils.hpp"

/**
 * @brief Class to represent and manipulate HTTP headers
 * 
 * This class provides methods for working with HTTP headers,
 * with case-insensitive header names and utility methods for
 * common header operations.
 */
class Headers {
private:
    std::map<std::string, std::string> _headers;  // Normalized header name to value

    /**
     * @brief Normalize a header name (convert to lowercase)
     * 
     * @param name Header name to normalize
     * @return std::string Normalized header name
     */
    std::string _normalize(const std::string& name) const;

public:
    /**
     * @brief Construct a new Headers object
     */
    Headers();
    
    /**
     * @brief Destroy the Headers object
     */
    ~Headers();

    /**
     * @brief Set a header value
     * 
     * @param name Header name
     * @param value Header value
     */
    void set(const std::string& name, const std::string& value);
    
    /**
     * @brief Check if a header exists
     * 
     * @param name Header name
     * @return true if header exists
     */
    bool contains(const std::string& name) const;
    
    /**
     * @brief Get a header value
     * 
     * @param name Header name
     * @return std::string Header value, empty string if not found
     */
    std::string get(const std::string& name) const;
    
    /**
     * @brief Remove a header
     * 
     * @param name Header name
     */
    void remove(const std::string& name);
    
    /**
     * @brief Get all headers as a map
     * 
     * @return const std::map<std::string, std::string>& Headers map
     */
    const std::map<std::string, std::string>& getAll() const;
    
    /**
     * @brief Convert headers to a string
     * 
     * @return std::string Headers as a string in HTTP format
     */
    std::string toString() const;
    
    /**
     * @brief Parse headers from a string
     * 
     * @param str String containing headers
     * @return bool True if parsing was successful
     */
    bool parse(const std::string& str);
    
    /**
     * @brief Clear all headers
     */
    void clear();
    
    /**
     * @brief Get the Content-Length value
     * 
     * @return size_t Content length, 0 if not set or invalid
     */
    size_t getContentLength() const;
    
    /**
     * @brief Get the Content-Type value
     * 
     * @return std::string Content type, empty string if not set
     */
    std::string getContentType() const;
    
    /**
     * @brief Check if Transfer-Encoding is chunked
     * 
     * @return true if chunked encoding is used
     */
    bool hasChunkedEncoding() const;
    
    /**
     * @brief Check if connection should be kept alive
     * 
     * @param defaultValue Default value if header not present
     * @return true if connection should be kept alive
     */
    bool keepAlive(bool defaultValue = true) const;
};