#pragma once

#include <string>
#include <map>
#include "../config/parser/LocationConfig.hpp"

/**
 * @brief Utility class for file operations
 * 
 * This class provides functions for:
 * - Checking if a file exists
 * - Reading file contents
 * - Getting MIME types
 * - Generating directory listings
 * - Resolving file paths based on server configuration
 */
class FileUtils {
public:
    /**
     * @brief Check if a file exists and is readable
     * 
     * @param path File path
     * @return true if file exists and is readable
     */
    static bool fileExists(const std::string& path);
    
    /**
     * @brief Check if a path is a directory
     * 
     * @param path Directory path
     * @return true if path is a directory
     */
    static bool isDirectory(const std::string& path);
    
    /**
     * @brief Get the contents of a file
     * 
     * @param path File path
     * @return std::string File contents, empty string if error
     */
    static std::string getFileContents(const std::string& path);
    
    /**
     * @brief Get the size of a file
     * 
     * @param path File path
     * @return size_t File size, 0 if error
     */
    static size_t getFileSize(const std::string& path);
    
    /**
     * @brief Get the MIME type for a file extension
     * 
     * @param extension File extension (with or without leading dot)
     * @return std::string MIME type, "application/octet-stream" if unknown
     */
    static std::string getMimeType(const std::string& extension);
    
    /**
     * @brief Generate an HTML directory listing for a directory
     * 
     * @param dirPath Directory path
     * @param requestPath Request path (for URLs)
     * @return std::string HTML listing, empty string if error
     */
    static std::string generateDirectoryListing(const std::string& dirPath, const std::string& requestPath);
    
    /**
     * @brief Resolve a URI path to a file system path
     * 
     * @param uriPath URI path from request
     * @param location Location configuration
     * @return std::string Resolved file system path
     */
    static std::string resolvePath(const std::string& uriPath, const LocationConfig& location);
    
    /**
     * @brief Get the file extension from a path
     * 
     * @param path File path
     * @return std::string File extension (without dot), empty if none
     */
    static std::string getFileExtension(const std::string& path);
    
    /**
     * @brief Get a map of common MIME types
     * 
     * @return const std::map<std::string, std::string>& Map of extension to MIME type
     */
    static const std::map<std::string, std::string>& getMimeTypes();
};