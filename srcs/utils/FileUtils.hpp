#pragma once

#include <string>
#include <map>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include "../config/parser/LocationConfig.hpp"

/**
 * @brief Class containing file operation utilities
 */
class FileUtils
{
public:
    /**
     * @brief File type enumeration
     */
    enum FileType
    {
        TYPE_UNKNOWN,
        TYPE_REGULAR,
        TYPE_DIRECTORY,
        TYPE_SYMLINK,
        TYPE_SPECIAL
    };

    /**
     * @brief File entry information structure
     */
    struct FileInfo
    {
        std::string name;      // File name
        std::string path;      // Full path
        size_t size;           // File size in bytes
        time_t modTime;        // Last modification time
        FileType type;         // File type
        std::string extension; // File extension
        std::string mimeType;  // MIME type
    };

    /**
     * @brief Check if a file exists and is readable
     * 
     * @param path File path
     * @return bool True if file exists and is readable
     */
    static bool fileExists(const std::string& path);
    
    /**
     * @brief Check if a path is a directory
     * 
     * @param path Directory path
     * @return bool True if path is a directory
     */
    static bool isDirectory(const std::string& path);
    
    /**
     * @brief Get the type of a file
     * 
     * @param path File path
     * @return FileType File type enumeration value
     */
    static FileType getFileType(const std::string& path);
    
    /**
     * @brief Check if a file is readable
     * 
     * @param path File path
     * @return bool True if file exists and is readable
     */
    static bool isReadable(const std::string& path);
    
    /**
     * @brief Check if a file is writable
     * 
     * @param path File path
     * @return bool True if file exists and is writable
     */
    static bool isWritable(const std::string& path);
    
    /**
     * @brief Create a directory if it doesn't exist
     * 
     * @param path Directory path
     * @param mode Directory permissions (default: 0755)
     * @return bool True if directory exists or was created successfully
     */
    static bool createDirectory(const std::string& path, mode_t mode = 0755);
    
    /**
     * @brief Check if a path is within a parent directory
     * 
     * @param path Path to check
     * @param parentDir Parent directory
     * @return bool True if path is within parentDir
     */
    static bool isPathWithinDirectory(const std::string& path, const std::string& parentDir);
    
    /**
     * @brief Get the contents of a file
     * 
     * @param path File path
     * @return std::string File contents, empty string if error
     */
    static std::string getFileContents(const std::string& path);
    
    /**
     * @brief Write contents to a file
     * 
     * @param path File path
     * @param contents Content to write
     * @return bool True if successful
     */
    static bool writeFileContents(const std::string& path, const std::string& contents);
    
    /**
     * @brief Get detailed information about a file
     * 
     * @param path File path
     * @return FileInfo Structure containing file information
     */
    static FileInfo getFileInfo(const std::string& path);
    
    /**
     * @brief Get the size of a file
     * 
     * @param path File path
     * @return size_t File size, 0 if error
     */
    static size_t getFileSize(const std::string& path);
    
    /**
     * @brief Get the last modification time of a file
     * 
     * @param path File path
     * @return time_t Modification time, 0 if error
     */
    static time_t getFileModTime(const std::string& path);
    
    /**
     * @brief Format a file size in a human-readable format
     * 
     * @param size File size in bytes
     * @return std::string Formatted size (e.g., "1.2 MB")
     */
    static std::string formatFileSize(size_t size);
    
    /**
     * @brief Get the MIME type for a file extension
     * 
     * @param extension File extension (with or without leading dot)
     * @return std::string MIME type, "application/octet-stream" if unknown
     */
    static std::string getMimeType(const std::string& extension);
    
    /**
     * @brief Get the MIME type for a file based on its path
     * 
     * @param path File path
     * @return std::string MIME type, "application/octet-stream" if unknown
     */
    static std::string getMimeTypeFromPath(const std::string& path);
    
    /**
     * @brief List files in a directory
     * 
     * @param dirPath Directory path
     * @return std::vector<FileInfo> List of file information structures
     */
    static std::vector<FileInfo> listDirectory(const std::string& dirPath);
    
    /**
     * @brief Generate an HTML directory listing for a directory
     * 
     * @param dirPath Directory path
     * @param requestPath Request path (for URLs)
     * @return std::string HTML listing, empty string if error
     */
    static std::string generateDirectoryListing(const std::string& dirPath, const std::string& requestPath);
    
    /**
     * @brief Join path components safely
     * 
     * @param base Base directory path
     * @param path Additional path component
     * @return std::string Combined path
     */
    static std::string joinPath(const std::string& base, const std::string& path);
    
    /**
     * @brief Normalize a path (remove redundant slashes, resolve ./ and ../)
     * 
     * @param path Path to normalize
     * @return std::string Normalized path
     */
    static std::string normalizePath(const std::string& path);
    
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
    
    /**
     * @brief Check if a file extension is in a list of extensions
     * 
     * @param filepath File path
     * @param extensions List of extensions (with or without leading dot)
     * @return bool True if file has one of the extensions
     */
    static bool hasExtension(const std::string& filepath, const std::vector<std::string>& extensions);
    
    /**
     * @brief Create a temporary file
     * 
     * @param prefix Prefix for the temporary file name
     * @param contents Optional contents to write to the file
     * @return std::string Path to the created temporary file, empty on failure
     */
    static std::string createTempFile(const std::string& prefix, const std::string& contents = "");
    
    /**
     * @brief Delete a file
     * 
     * @param path File path
     * @return bool True if file was deleted successfully
     */
    static bool deleteFile(const std::string& path);
    
private:
    // Private constructor to prevent instantiation
    FileUtils();
    // Private destructor
    ~FileUtils();
    // Private copy constructor and assignment operator to prevent copying
    FileUtils(const FileUtils& other);
    FileUtils& operator=(const FileUtils& other);
};